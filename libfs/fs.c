#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF
#define ceilingdiv(x,y) \
	1 + ((x - 1) / y)
//phase 1-2 function prototypes
int bytes_to_block(int y);
int delete_file(int fir_block);
char * resize_buffer(char * buffer, int old_size, int * new_size);
int update_RD();
int read_in_RD();
int read_in_FAT();
int update_FAT();
int delete_root(const char * fname);
int file_exist(const char * fname);
int delete_file(int fir_block);
struct Root_Dir * create_root(const char *file_n);
int next_block();
int free_FAT_blocks();
int free_RD_blocks();
//phase 3 function prototypes
static int fs_fd_init(int fd, const char *filename);
int return_rd(char * fd_name);
int next_block();
int fd_exists(int fd);
int file_exists(const char * fd_name);
uint32_t file_extend(int fd, uint16_t blockcount);

typedef struct __attribute__((__packed__)) sBlock{
	
	char Sig[8];
	uint16_t tNumBlocks;
	uint16_t rdb_Index;
	uint16_t d_block_start;
	uint16_t nDataBlocks;
	uint8_t  nFAT_Blocks;
	char padding[4079];
}t;

typedef struct __attribute__((__packed__)) FAT{
	
	uint16_t *f_table;
}t2;
typedef struct __attribute__((__packed__)) Root_Dir{
	
	char fname[FS_FILENAME_LEN];
	uint32_t fSize;
	uint16_t  f_index;
	char padding[10];
}t3;
typedef struct fs_filedes {

	int fd_offset;
	char fd_filename[FS_FILENAME_LEN];
   

}t4;
int rd_total=0;//need to keep track of creates and deletes later on
int fd_total=0;
struct fs_filedes *filedes;
int FS_Mount=0;
struct sBlock * SB;
struct Root_Dir * RD;
struct FAT * fat;

/* TODO: Phase 1 */

int fs_mount(const char *diskname)
{
	
	char signature[8] = {'E','C','S','1','5','0','F','S'};
	SB = (struct sBlock*) calloc(1,sizeof(struct sBlock));
	RD = (struct Root_Dir*) calloc(FS_FILE_MAX_COUNT, sizeof(struct Root_Dir));//using calloc to initialize pointers
	fat= (struct FAT*) calloc(1,sizeof(struct FAT));
	//filename cannot be a NULL terminator
	if(diskname[0]=='\0'){
		return -1;
	}
	
	//open disk
	if(block_disk_open(diskname)!=0){
		free(SB);
		free(RD);
		return -1;
	}
	
	//read in superblock
	if(block_read(0, (void*)SB)!=0){
		free(SB);
		free(RD);
		return -1;
	}
	SB = (struct sBlock*)SB;
	//used strncmp instead strcmp becasue signature 
	//is not NULL terminated
	if(strncmp(signature, SB->Sig,8)!=0){
		free(SB);
		free(RD);
		return -1;
	}
	
	if(block_disk_count()!=SB->tNumBlocks){
		free(SB);
		free(RD);
		return -1;
	}
	
	//initialize empty entries
	for(int i =0; i<FS_FILE_MAX_COUNT; i++){
		RD[i].fname[0]='\0';
		
	}
	
	//create file decriptor table
	filedes = calloc(FS_OPEN_MAX_COUNT, sizeof(struct fs_filedes));
	//initialize name of each file decriptor name to '\0'
	for(int i =0; i<FS_OPEN_MAX_COUNT; i++){
		filedes[i].fd_filename[0] = '\0';
		
	}
	//initialize FAT table and all indices to 0
	fat->f_table =  calloc(SB->nDataBlocks, sizeof(struct FAT));
	
	if(read_in_FAT()!=0){
		return -1;
	}
	//make sure first FAT block is 
	if(fat->f_table[0]!=FAT_EOC){
		free(SB);
		free(RD);
		free(fat->f_table);
		free(fat);
		free(filedes);
		return -1;
	}
		int numUsedFB=0;
		for(int i=0; i<SB->nDataBlocks;i++){
		   //if data is stored in the fat table, then it is likely root dir
		   //does not contain garbage and sould be read
		   if(fat->f_table[i]!=0){
			numUsedFB++;
		   }
		   if(numUsedFB>1){
		   if(read_in_RD()!=0){
			  fprintf(stderr,"READ RD FAIL\n");
			  return -1;
		   }
			  break;
	   }
	   }
	
	FS_Mount=1;
	
	return 0;
	
	/* TODO: Phase 1 */
}


int fs_umount(void)
{
	//check if a virtual disk is open
	//Check if there are open file descriptors
	if(FS_Mount==0||fd_total>0){
		return -1;
	}
	if(update_RD()){
		return -1;
	}
	if(update_FAT()){
		return -1;
	}
	//must do close disk before calling block_disk_close(),
	//just in case it returns -1
	if(block_disk_close()){
		return -1;
	}
	free(SB);
	free(RD);
	free(fat->f_table);
	free(fat);
	free(filedes);
	return 0;
	/* TODO: Phase 1 */
}

int fs_info(void)
{
	//ensure that mount has been sucessfully called
	if(FS_Mount==0){
		return -1;
	}
	
	fprintf(stdout,"FS Info:\n");
	fprintf(stdout,"total_blk_count=%d\n",SB->tNumBlocks);
	fprintf(stdout,"fat_blk_count=%d\n",SB->nFAT_Blocks);
	fprintf(stdout,"rdir_blk=%d\n",SB->rdb_Index);
	fprintf(stdout,"data_blk=%d\n",SB->d_block_start);
	fprintf(stdout,"data_blk_count=%d\n",SB->nDataBlocks);
	fprintf(stdout,"fat_free_ratio=%d/%d\n",free_FAT_blocks(),SB->nDataBlocks);
	fprintf(stdout,"rdir_free_ratio=%d/%d\n",free_RD_blocks(),FS_FILE_MAX_COUNT);
	
	
	
	return 0;
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	//ensure that mount has been sucessfully called
	if(FS_Mount==0){
		return -1;
	}
	//ensure that the root directory isn't full
	if(free_RD_blocks()<1){
		return -1;
	}
	//ensure that the FAT table isn't full
	/*
	if(free_FAT_blocks()<1){
		return -1;
	}
	*/
	//check if ptr is valid
	if (filename == NULL)
		return -1;
	if (filename[0] == '\0')
		return -1;
	//check if filename is valid
	int i = 0;
	while (filename[i] != '\0') {
		if ((i+1==FS_FILENAME_LEN)&&(filename[i] != '\0')){
			return -1;
		}
		i++;
	}
	//cannot have two files with same name
	if(!file_exists(filename))
		return -1;
	struct Root_Dir * new_file = create_root(filename);
	strcpy(new_file->fname,filename);
	//set the new files size to 0 and its first data block index to FAT_EOC
	new_file->fSize=0;
	new_file->f_index=FAT_EOC;
	return 0;
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	//ensure that mount has been sucessfully called
	if(FS_Mount==0){
		return -1;
	}

	//validate filename
	
	char null[1] = {'\0'};

	if(file_exist(filename)!=0||(strncmp(filename,null,1)==0)){
		return -1;
	}

	//check to see if filename is open in any file descriptors
	for(int z=0; z<FS_OPEN_MAX_COUNT;z++){
		if(strcmp(filedes[z].fd_filename,filename)==0){
			return -1;
		}
	}
	int i=0;
	while(i<FS_FILE_MAX_COUNT){
		if((strcmp(RD[i].fname,filename)==0)&&(RD[i].f_index!=FAT_EOC)){
			delete_file(RD[i].f_index);
			delete_root(filename);
			RD[i].fSize=0;
			RD[i].f_index=FAT_EOC;
			break;
		}
		//if file is empty it shouldn't be sent to delete_file()
		if((strcmp(RD[i].fname,filename)==0)&&(RD[i].f_index==FAT_EOC)){ 
			delete_root(filename);
			RD[i].fSize=0;
			RD[i].f_index=FAT_EOC;
			break;
		}
		i++;
	}
	return 0;
	/* TODO: Phase 2 */
}

int fs_ls(void)
{   //ensure that mount has been sucessfully called
	if(FS_Mount==0){
		return -1;
	}
	
	int num_files=0;
	for(int i=0; i<FS_FILE_MAX_COUNT; i++){
		if(RD[i].fname[0]!='\0'){
			num_files++;
			if(num_files==1){
				fprintf(stdout,"FS Ls:\n");
			}
			fprintf(stdout,"file: %s, size: %d, data_blk: %d\n",RD[i].fname,RD[i].fSize,RD[i].f_index);
		}
	
	}
	return 0;
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	//ensure that mount has been sucessfully called
	if(FS_Mount==0){
		return -1;
	}
	//check if ptr is valid
	if (filename == NULL)
		return -1;

	//check if filename is valid
	int i = 0;
	while (filename[i] != '\0') {
		if (i+1 == FS_FILENAME_LEN)
			return -1;
		i++;
	}
	//make sure fie exists before trying to open it
	if(file_exists(filename)==-1)
		return -1;

	int j = 0;
	//check if there is an available file des spot
	//j will equal the first open spot starting from index 0
	while (filedes[j].fd_filename[0] != '\0') {
		if (j > FS_OPEN_MAX_COUNT)
			return -1;
		j++;
	}
	//at this point, j == the open filedes spot
	//initialize the file descriptor
	fs_fd_init(j, filename);
	fd_total++;
	return j;
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	//ensure that mount has been sucessfully called
	if(FS_Mount==0){
		return -1;
	}
	if(FS_OPEN_MAX_COUNT<=fd||fd<0){
		return -1;
	}
	if((filedes[fd].fd_filename[0]=='\0')){
		return -1;
	}
	//set fd to intial value
	filedes[fd].fd_filename[0]='\0';
	fd_total--;
	return 0;
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	//ensure that mount has been sucessfully called
	if(FS_Mount==0){
		return -1;
	}
	if(FS_OPEN_MAX_COUNT<=fd||fd<0){
		return -1;
	}
	if((fd_exists(fd))){
	   return -1;
	}
	else{
		for(int i=0; i<FS_FILE_MAX_COUNT;i++){
			//make sure file still exists
			if(strcmp(RD[i].fname,filedes[fd].fd_filename)==0){
				return RD[i].fSize;
			}
			
		}
	   
	}	
	//fd does not exist
	return -1;
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	//ensure that mount has been sucessfully called
	if(FS_Mount==0){
		return -1;
	}
	if(fd_exists(fd)){
		return -1;
	} 
	int rd_index=return_rd(filedes[fd].fd_filename);//make sure file still exists
	if(offset>RD[rd_index].fSize||rd_index<0){
		return -1;
	}
	//if file exists and its size is equal to or greater
	//than offset
	filedes[fd].fd_offset = offset;
	return 0;
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	//Error checking before the writes
	if (fd > FS_OPEN_MAX_COUNT)
		return -1;

	if (fd_exists(fd))
		return -1;

	//The bounce buffer for cases where we need to preserve existing data
	void *bounce_buf = malloc(BLOCK_SIZE);

	//These variables reflect the status of the file
	int file_offset = filedes[fd].fd_offset;
	int block_offset = file_offset % BLOCK_SIZE;
	int fsrd = return_rd(filedes[fd].fd_filename);

	//These variables are for coordinating the write itself
	uint16_t end_offset = 0;
	uint16_t start_offset = 0;
	uint32_t ext_blocks = 0;
	int buf_index = 0;
	int write_amt = 0;
	int bytes_remaining = count;

	//Integer division in C returns the floor
	//The block number in the chain which corresponds to the file offset
	int target_blocknum = filedes[fd].fd_offset / BLOCK_SIZE;

	//Current = the first index of the file pointed at by fd
	uint16_t firstblock = RD[fsrd].f_index;


	//If the file needs to be intialized
	if (firstblock == FAT_EOC) {
		int nextb = next_block();
		if (nextb == -1) {
			//No more data blocks to append
			return 0;
		} else {
			RD[fsrd].f_index = nextb;
			fat->f_table[nextb] = FAT_EOC;
			firstblock = nextb;
		}
	}

	//Current block in the chain
	uint16_t curblock = firstblock;

	int filesize = RD[fsrd].fSize;

	//Cycles through the fat and finds the index target_block in file des fd
	for (int i = 0; i < target_blocknum; i++) {
		curblock = fat->f_table[curblock];
	}

	do {
		if (curblock == firstblock) {
			start_offset = block_offset;
		} else {
			start_offset = 0;
		}

		//if it is the first or last block to write, preserve
		//the data outside of the write
		if (curblock == firstblock || bytes_remaining < BLOCK_SIZE) {
			if (block_read(curblock + SB->d_block_start, bounce_buf))
				return -1;
		}

		//if writing less than block, write amt needed
		if (bytes_remaining < BLOCK_SIZE) {
			end_offset = bytes_remaining - 1;
		} else {
			//if writing more than block, write to whole block
			end_offset = BLOCK_SIZE - 1;
		}

		write_amt = end_offset - start_offset + 1;

		memcpy(bounce_buf + start_offset, buf + buf_index, write_amt);

		if (block_write(curblock + SB->d_block_start, bounce_buf))
			return -1;

		//cleanup for next iteration
		bytes_remaining -= write_amt;
		buf_index += write_amt;

		//check if the file needs to be extended
		if (fat->f_table[curblock] == FAT_EOC && bytes_remaining > 0) {
			ext_blocks = file_extend(fd, ceilingdiv(bytes_remaining, BLOCK_SIZE));
			
			//If writing more than is available after extension, write as much
			//as possible
			if (bytes_remaining > ext_blocks * BLOCK_SIZE) {
				bytes_remaining = ext_blocks * BLOCK_SIZE;
			}
		}

		curblock = fat->f_table[curblock];

	} while (bytes_remaining > 0);

	filedes[fd].fd_offset = filedes[fd].fd_offset + buf_index;

	//if we wrote additional blocks to file
	if (filesize < file_offset + buf_index) {
		RD[fsrd].fSize = file_offset + buf_index;
		if (update_RD())
			return -1;
	}

	free(bounce_buf);

	return buf_index;
}

//using generic variables like rootdir. Will refactor when other phases are finalized
//ceilingdiv(a,b) is a symbolic function, defined at the top
//it is simply CEILING(a/b), with integers a and b
//May bug out on testing due to mixing 0-indexed values and 1-indexed values
int fs_read(int fd, void *buf, size_t count)
{
	//fd is out of bounds
	if (fd > FS_OPEN_MAX_COUNT)
		return -1;

	//fd is not currently open
	if (fd_exists(fd))
		return -1;

	//Used for first and last block
	void *bounce_buf = malloc(BLOCK_SIZE);

	//Data about the file
	int file_offset = filedes[fd].fd_offset;
	int block_offset = file_offset % BLOCK_SIZE;
	int fsrd = return_rd(filedes[fd].fd_filename);
	int filesize = RD[fsrd].fSize;

	//Variables manipulated to read the correct portion
	int read_amt = 0;
	int buf_index = 0;
	int bytes_remaining = count;
	uint16_t start_offset = 0;
	uint16_t end_offset = 0;

	//which index of block from FAT to read from: ceiling(offset/BLOCK_SIZE)
	//Division of integers returns floor(a/b)
	int target_blocknum = file_offset/BLOCK_SIZE;

	//the first block in the chain
	uint16_t firstblock = RD[fsrd].f_index;
	uint16_t curblock = firstblock;

	//Cycles through the fat and finds the index target_blocknum in fd
	for (int i = 0; i < target_blocknum; i++) {
		curblock = fat->f_table[curblock];
	}

	do {
		if (curblock == firstblock) {
			start_offset = block_offset;
		} else {
			start_offset = 0;
		}

		//if curblock is the last of the chain
		if (fat->f_table[curblock] == FAT_EOC || bytes_remaining < BLOCK_SIZE) {
			//If desired read is longer than the filesize
			if (file_offset + count - 1 > filesize) {
				//Read to end of file
				end_offset = (filesize % BLOCK_SIZE) - 1;
			} else {
				//else read as many bytes as count
				end_offset = count - buf_index - 1;
			}
		} else {
			end_offset = BLOCK_SIZE - 1;
		}

		read_amt = end_offset - start_offset + 1;

		if (block_read(curblock + SB->d_block_start, bounce_buf))
			return -1;

		memcpy(buf + buf_index, bounce_buf + start_offset, read_amt);

		bytes_remaining -= read_amt;
		buf_index += read_amt;

		curblock = fat->f_table[curblock];

	} while (curblock != FAT_EOC && bytes_remaining > 0);

	//need to use buf_index because count could exceed the size of the file
	filedes[fd].fd_offset = filedes[fd].fd_offset + buf_index;

	free(bounce_buf);

	return buf_index;
}

int read_in_RD(){
	//write root directory block to first data block index
	int ret = block_read(SB->rdb_Index,RD);
	return ret;
}
//phase 1-2 helper functions
int update_RD(){
	//write root directory block to first data block index
	int ret = block_write(SB->rdb_Index,RD);
	return ret;
}
int read_in_FAT(){
	
	int size=0, y=0;
	char *new_array =resize_buffer((char *)fat->f_table,SB->nDataBlocks*sizeof(uint16_t),&size);
	int retVals[SB->rdb_Index-1];
	for(int i =1; i<SB->rdb_Index; i++){
		y=i-1;
		retVals[i]=block_read(i,new_array+(y*BLOCK_SIZE));
		if(retVals[i]==-1) {
			free(new_array);
			return -1;
		}
	}
	uint16_t * upFAT=(uint16_t*)new_array;
	for(int i =0; i<SB->nDataBlocks; i++){
		fat->f_table[i]=upFAT[i];
	}
	
	
	free(new_array);
	return 0;
	/* TODO: Phase 1 */
}

int update_FAT(){
	
	int size=0,y=0;
	//new_array conatains all contents of fat->f_table and is
	//an ammount of bytes that is a multiple of BLOCK_SIZE
	char *new_array =resize_buffer((char*)fat->f_table,SB->nDataBlocks*sizeof(uint16_t),&size);
	int retVals[SB->rdb_Index-1];
	
	//write new_array into disk where FAT table is located
	for(int i=1; i<SB->rdb_Index; i++){
		y=i-1;
		retVals[i]=block_write(i,new_array + (y*BLOCK_SIZE));
		if(retVals[i]==-1){
			free(new_array);
			return -1;
		}
	}
	
	free(new_array);
	return 0;
	/* TODO: Phase 1 */
}

//old size is in bytes
char * resize_buffer(char * buffer, int old_size, int * new_size){
	char * new_buffer;
	if((old_size%BLOCK_SIZE)==0){
		new_buffer = calloc(old_size, sizeof(char));
		for(int i=0; i < old_size; i++){
		 new_buffer[i]=buffer[i];
		}
		*new_size=old_size;
		return new_buffer;
	}
	else{
		//duplicating old buffer into new buffer that is 
		//a multiple of BLOCK_SIZE
		int blocks = old_size/BLOCK_SIZE;
		blocks=blocks+1;
		int num_bytes = blocks*BLOCK_SIZE;
		*new_size=num_bytes;
		new_buffer = calloc(num_bytes, sizeof(char));
		for(int i=0; i<num_bytes; i++){
		 new_buffer[i]=buffer[i];
		}
		
	
	}   
		return new_buffer;
	/* TODO: Phase 1-2 */
}


int delete_root(const char * fname){
	
	for(int i=0; i<FS_FILE_MAX_COUNT; i++){
		if(strcmp(RD[i].fname,fname)==0){
			RD[i].fname[0]='\0';
		  return 0;
		}
	}
	return -1;
}

int file_exist(const char * fname){
	
	for(int i=0; i<FS_FILE_MAX_COUNT; i++){
		if((strcmp(RD[i].fname,fname)==0)){
		  return 0;
		}
	}
	return -1;
}

int delete_file(int fir_block){
	int temp =0; 
	
	
	for(int i=0; i<SB->nDataBlocks; i++){
		if(fat->f_table[fir_block]==FAT_EOC){
			fat->f_table[fir_block]=0;
			return 0;
		}
		temp = fat->f_table[fir_block];
		fat->f_table[fir_block]=0;
		
	
		fir_block = temp;
	}
	//EOC wasn't found
	return -1;
}

struct Root_Dir * create_root(const char *file_n){
	
	for(int i=0; i<FS_FILE_MAX_COUNT; i++){
		  if(RD[i].fname[0]=='\0'){
			strcpy(RD[i].fname, file_n);
		  return RD+i;
		  }
	}
	return NULL;
}
int next_block(){
	
	for(int i =0; i<SB->nDataBlocks; i++){
		if(fat->f_table[i]==0){
			return i;
		}
	}
	//FAT table is full
	return -1;
}

int free_FAT_blocks(){
	
	int fb_count=0;
	for(int i =0;i<SB->nDataBlocks;i++){
		if(fat->f_table[i]==0){
			fb_count++;
		}
	}
	return fb_count;
}
int free_RD_blocks(){
	int rdb_count=0;
	for(int i=0; i<FS_FILE_MAX_COUNT; i++){
		if(RD[i].fname[0]=='\0'){
		  rdb_count++;
		}
	}
	return rdb_count;
}

//phase 3 helper functions
static int fs_fd_init(int fd, const char *filename)
{
	filedes[fd].fd_offset = 0;
	strcpy(filedes[fd].fd_filename, filename);

	return 0;
	/* TODO: Phase 3 */
}

int return_rd(char * fd_name){
	
	for(int i=0; i<FS_FILE_MAX_COUNT; i++){
		//compare input string to filenames in root directory
		if((strcmp(RD[i].fname,fd_name)==0)){
		  return i;
		}
	}
	return -1;
	/* TODO: Phase 3 */
}

int fd_exists(int fd)
{
	if(filedes[fd].fd_filename[0]=='\0'){
		return -1;
	}
	
	return 0;
	/* TODO: Phase 3 */
}
int file_exists(const char * fd_name)
{
	for(int i=0; i<FS_FILE_MAX_COUNT; i++){
		//compare input string to filenames in root directory
		if((strcmp(RD[i].fname,fd_name)==0)){
		  return 0;
		}
	}
	return -1;
	
	/* TODO: Phase 3 */
}

//returns the number of blocks added, which is as many as possible.
uint32_t file_extend(int fd, uint16_t blockcount)
{
	//next available data block in FAT
	int alloc_block;
	uint16_t curblock = 0;
	uint32_t blocks_added = 0;
	//fd's root directory index
	int fsrd = return_rd(filedes[fd].fd_filename);

	//find directory entry for fd
	curblock = RD[fsrd].f_index;
	while (fat->f_table[curblock] != FAT_EOC) {
		curblock = fat->f_table[curblock];
	}
	//curblock now equals the last block of fd's chain

	for (int i = 0; i < blockcount; i++) {
		alloc_block = next_block();

		//no more space in disk
		if (alloc_block == -1)
			break;

		//incorporate the new block to the fd's chain
		fat->f_table[alloc_block] = FAT_EOC;
		fat->f_table[curblock] = (uint16_t) alloc_block;
		curblock = (uint16_t) alloc_block;

		//for updating the file's entry in root dir
		blocks_added++;
	}
	
	return blocks_added;
}
