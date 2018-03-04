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
int create_root();
char * resize_buffer(char * buffer, int old_size, int * new_size);
int update_RD();
int read_in_FAT();
int update_FAT();
char * resize_buffer(char * buffer, int old_size, int * new_size);
int delete_root(char * fname);
int file_exist(char * fname);
int delete_file(int fir_block);
struct Root_Dir * create_root();
int next_block();
//phase 3 function prototypes
static int fs_fd_init(int fd, char *filename);
int return_rd(int fd);
int next_block();

typedef struct __attribute__((__packed__)) sBlock{
	
	char Sig[8];
	uint16_t tNumBlocks;
	uint16_t rdb_Index;
	uint16_t f_block_start;
	uint16_t nDataBlocks;
	uint8_t  nFAT_Blocks;
	char padding[4079];
}

typedef struct __attribute__((__packed__)) FAT{
	
	uint16_t *f_table;//will dynamically allocate, = nDataBlocks
}
typedef struct __attribute__((__packed__)) Root_Dir{
	
	char fname[FS_FILENAME_LEN];
	uint32_t fSize;
	uint16_t  f_index;
	char padding[10];
}
typedef struct fs_filedes {

	int fd_offset;
	char *fd_filename;
   // int length; //used to keep track of how far we've gone into file (in bytes); depth is basically file size in bytes
	//may need more later

};

int fd_total=0;
struct fs_filedes *filedes;
int FS_Mount=0;
int * dir; //keeps track of which index in the directory is being used
struct sBlock * SB;
struct Root_Dir * RD;
struct FAT * fat;

/* TODO: Phase 1 */

int fs_mount(const char *diskname)
{
	int cmp, retVal;
	char signature[8] = {'E','C','S','1','5','0','F','S'};
	SB = malloc(sizeof(sBlock));
	RD = malloc(FS_FILE_MAX_COUNT*sizeof(Root_Dir));
	
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
	SB = (sBlock*)SB;
	//used strncmp instead strcmp becasue signature 
	//is not NULL terminated
	if(strncmp(signature, SB.Sig,8)!=0){
		free(SB);
		free(RD);
		return -1;
	}
	if(block_disk_count()!=SB.tNumBlocks){
		free(SB);
		free(RD);
		return -1;
	}
	char null[1]={'\0'};
	//initialize empty entries
	for(int i =0; i<128; i++){
		RD[i].fname[0]='\0';//null
		//strcpy(RD.fname,null);
	}
	//create file decriptor table
	filedes = malloc(FS_OPEN_MAX_COUNT* sizeof(fs_filedes));
	//initialize name of each file decriptor name to '\0'
	for(int i =0; i<128; i++){
		filedes[i].fd_filename[0] = '\0';
		
	}
	fat->f_table =  calloc(SB->nDataBlocks *sizeof(FAT));//initialize all indices to 0
	if(read_in_FAT()){
		fprintf(stderr,"READ FAT FAIL\n");
		return -1;
	}
	//fat->f_table[0]= FAT_EOC; First index of FAT should alread have FAT_EOC
	dir =  calloc(FS_FILE_MAX_COUNT*sizeof(int));
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
		fprintf(stderr,"Update RD FAIL\n");
		return -1;
	}
	if(update_FAT()){
		fprintf(stderr,"Update FAT FAIL\n");
		return -1;
	}
	//do this before calling block_disk_close(), just in case
	//it returns -1
	if(block_disk_close())
		return -1;
	free(SB);
	free(RD);
	free(fat->f_table);
	free(fat);
	free(dir);
	free(filedes);
	return 0;
	/* TODO: Phase 1 */
}

int fs_info(void)
{
	if(FS_Mount==0){
		return -1;
	}
	
	fprintf(stderr,"File System Info\n");
	fprintf(stderr,"Signature: %s",SB.Sig);
	fprintf(stderr,"Number of Total Number of Blocks: %d\n",SB.NumBlocks);//not sure if sould use %d
	fprintf(stderr,"Number of Datablocks: %d\n",SB.nDataBlocks);
	fprintf(stderr,"Number of FAT Blocks: %d\n",SB.nFAT_Blocks);
	
	return 0;
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	struct Root_Dir new_file = create_root();//Root_Dir and dir have same index for this file
	strcpy(new_file->fname,filename);
	new_file->fSize=0;
	new_file->f_index=next_block();//
	fat->f_table[new_file->f_index]=FAT_EOC;
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	int i=0; char null[1]={'\0'};
	//check to see if filename is open in any file descriptors
	for(int i=0; i<32;i++){
		if((filedes[i].fd_filename,filename)==0){
		    return -1;
	    }
	}
	if(file_exist(filename)!=0||(strncmp(filename,null,1)==0)){
		return -1;
	}
	while(i<128){
		if((strcmp(RD[i].fname,filename)!=0){
			delete_file(RD[i].f_index);
			delete_root(filename);
			strcpy(RD[i].fname, null);
	        RD[i].fSize=0;//not necessarry specified as xxx
	        f_index=FAT_EOC;//not necessarry specified as xxx
		}
		i++;
	}
	return 0;
	/* TODO: Phase 2 */
}

int fs_ls(void)
{    
    if(FS_Mount==0){
		return -1;
	}
	
	for(int i=0; i<128; i++){
		//dir parallels Root_Directory
        if(RD[i].fname[0]=='\0'){
            fprintf(stderr,"%s ",RD[i].fname);
        }
		fprintf(stderr,"\n");
    }
	return 0;
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	
	//check if ptr is valid
	if (filename == NULL)
		return -1;

	//check if filename is valid
	int i = 0;
	while (filename[i] != '\0') {
		if (i > FS_FILENAME_LEN)
			return -1;
		i++;
	}
	/*
	 * check if the file exists
	 * not yet implemented:
	 * depends on the implementation of fs_create, etc.
	 * should also check whether the file is valid
	 */

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
	if((filedes[fd].fd_filename[0]=='\0')||32<=fd){
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
	
    if((!fd_exists(fd))||fd>=32||fd<0){
       return -1;
    }
    else{
		for(int i=0; i<128;i++){
			//make sure file still exists
			if(strcmp(RD[i].fname,filedes[fd].fd_filename){
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
	if(!fd_exists(fd){
		return -1;
	} 
	int rd_index=return_rd(fd);//make sure file still exists
	if(offset>RD[rd_index].fSize||rd_index<0){
		return -1;
	}
	//if file exists and its size is equal to or greater
	//than offset
	filedes[fd].fd_offset = offset;
	return 0;
	/* TODO: Phase 3 */
}

//WIP, not final
int fs_write(int fd, void *buf, size_t count)
{
	if (fd > FS_OPEN_MAX_COUNT)
		return -1;

	if (filedes[fd] == NULL)
		return -1;

	void *bounce_buf = malloc(BLOCK_SIZE);
	int buf_index = 0;
	int bytes_remaining = count;
	int file_offset = filedes[fd].offset;
	int block_offset = file_offset % BLOCK_SIZE;
	int write_amt = 0;
	unsigned int end_offset = 0;
	unsigned int start_offset = 0;

	int filesize = rootdir[filedes[fd].rdindex].fSize;

	int target_blocknum = ceilingdiv(filedes[fd].offset, BLOCK_SIZE);
	//current = the first index of the file pointed at by fd
	uint16_t curblock = rootdir[filedes[fd].rdindex].index;

	//Cycles through the fat and finds the index target_block in file des fd
	for (int i = 0; i < target_blocknum; i++)
		curblock = fat[curblock];

	//First Block
	uint16_t firstblock = curblock;

	do {
		if (curblock == firstblock) {
			start_offset = block_offset;
		} else {
			start_offset = 0;
			//copy over read_amt bytes to bounce_buf
		}

		//if it is the first or last block to write, preserve
		//the data outside of the write
		if (curblock == firstblock || bytes_remaining < BLOCK_SIZE) {
			if (block_read(curblock, bounce_buf))
				return -1;
		}

		//if curblock is the last of the chain
		if (fat[curblock] == FAT_EOC || bytes_remaining < BLOCK_SIZE) {
			if (file_offset + count > filesize) {
				//Resize the file
				disk_extend(fd, ext_blocks);
				end_offset = filesize % BLOCK_SIZE;
			} else {
				end_offset = count - buf_index;
			}
		} else {
			end_offset = BLOCK_SIZE;
		}

		write_amt = end_offset - start_offset;

		memcpy(buf + buf_index, bounce_buf + start_offset, write_amt);

		bytes_remaining -= write_amt;
		buf_index += write_amt;

		curblock = fat[curblock];

	} while (bytes_remaining > 0);

	filedes[fd].offset = filedes[fd].offset + buf_index;

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
	if (filedes[fd] == NULL)
		return -1;

	//used for first and last block
	void *bounce_buf = malloc(BLOCK_SIZE);

	//index in *buf which is currently being copied to
	int read_amt = 0;
	int buf_index = 0;
	int bytes_remaining = count;
	int file_offset = filedes[fd].offset;
	int block_offset = file_offset % BLOCK_SIZE;
	unsigned int start_offset = 0;
	unsigned int end_offset = 0;
	

	int filesize = rootdir[filedes[fd].rdindex].fSize;

	//which index of block from FAT to read from: ceiling(offset/BLOCK_SIZE)
	int target_blocknum = ceilingdiv(file_offset, BLOCK_SIZE);

	//the first block in the chain
	uint16_t curblock = rootdir[filedes[fd].rdindex].index;

	//Cycles through the fat and finds the index target_blocknum in fd
	for (int i = 0; i < target_blocknum; i++)
		curblock = fat[curblock];

	//first block to read from
	uint16_t firstblock = curblock;

	do {
		if (curblock == firstblock) {
			start_offset = block_offset;
		} else {
			start_offset = 0;
		}

		//if curblock is the last of the chain
		if (fat[curblock] == FAT_EOC || bytes_remaining < BLOCK_SIZE) {
			if (file_offset + count > filesize) {
				//Read to end of file
				end_offset = filesize % BLOCK_SIZE;
			} else {
				end_offset = count - buf_index;
			}
		} else {
			end_offset = BLOCK_SIZE;
		}

		read_amt = end_offset - start_offset;

		memcpy(buf + buf_index, bounce_buf + start_offset, read_amt);

		bytes_remaining -= read_amt;
		buf_index += read_amt;

		curblock = fat[curblock];

	} while (curblock != FAT_EOC && bytes_remaining > 0);

	//need to use buf_index because count could exceed the size of the file
	filedes[fd].offset = filedes[fd].offset + buf_index;

	free(bounce_buf);

	return buf_index;
}



//phase 1-2 helper functions
int update_RD(){
	//write root directory block to first data block index
	int ret = block_write(SB.f_block_start,RD);
	return ret;
}
int read_in_FAT(){
	
	int size=0, ret = 0;
	char *new_array =resize_buffer(fat.f_table,SB.nDataBlocks*sizeof(uint16_t),&size);
	int retVals[SB.f_block_start-1];
	for(int i =1; i<SB.f_block_start; i++){
		y=i-1;
		retVals[i]=block_read(i,new_array+(y*BLOCK_SIZE));
	}
	uint16_t * upFAT=(uint16_t*)new_array;
	for(int i =0; i<nDataBlocks; i++){//the indexing may lead to a segfault
		fat->f_table[i]=upFAT[i];
	}
	
	for(int i=1; i<SB.f_block_start; i++){
		if(ret!=retVals[i]){
			ret=retVals[i];
		}
	}
	
	free(new_array);
	return ret;
	/* TODO: Phase 1 */
}
int update_FAT(){
	
	int size=0, ret = 0;
	char *new_array =resize_buffer((char*)fat.f_table,SB.nDataBlocks*sizeof(uint16_t),&size);
	int retVals[SB.f_block_start-1];
	for(int i=1; i<SB.f_block_start; i++){
		y=i-1;
		retVals[i]=block_write(i,fat->f_table+(y*BLOCK_SIZE));
	}
	for(int i=1; i<SB.f_block_start; i++){
		if(ret!=retVals[i]){
			ret=retVals[i];
		}
	}
	free(new_array);
	return ret;
	/* TODO: Phase 1 */
}
//old size is in bytes
char * resize_buffer(char * buffer, int old_size, int * new_size){
	char * new_buffer;
    if((old_size%BLOCK_SIZE)==0){
		new_buffer = calloc(old_size* sizeof(char));
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
		new_size=num_bytes;
        new_buffer = calloc(num_bytes * sizeof(char));
		for(int i=0; i<num_bytes; i++){
         new_buffer[i]=buffer[i];
        }
		
	
	}   
	    return new_buffer;
    /* TODO: Phase 1-2 */
}


int delete_root(char * fname){
    
    for(int i=0; i<128; i++){
		//dir parallels Root_Directory
        if(strcmp(RD[i].fname,fname)==0){
            dir[i]=0;
          return 0;
        }
    }
    return NULL;
}

int file_exist(char * fname){
    
    for(int i=0; i<128; i++){
        if((strcmp(RD[i].fname,fname)==0)&&(dir[i]==1)){
          return 0;
        }
    }
    return -1;
}

int delete_file(int fir_block){
    int temp =0; 
	char * null= calloc(BLOCK_SIZE *sizeof(char));
	//memset( null, '\0', BLOCK_SIZE *sizeof(char) );
    
    //bool not_at_end=false;
    for(int i=0; i<nDataBlocks; i++){
        if(fat->f_table[fir_block]==FAT_EOC){
            fat->f_table[fir_block]=0;
		    //zero out corresponding data block, might not be necesary
            //block_write(SB.f_block_start+fir_block,null);
		
            return 0;
        }
        temp = f_table[fir_block];
        fat->f_table[f_block]=0;
	    //zero out corresponding data block, cannot ignore data in
        //data blocks, it may accidentally be read in later on fs_read,
        //might not be necesary, because whole block is overwritten
        //block_write(SB.f_block_start+fir_block,null);
	    //zeroing out corresponding Fat block index may not becasue
	    //necesary because will not be reading from fat table blocks
	    //and will totally overwrite totally these blocks when writing
	    //to fat table
	    //Will need to update FAT blocks and Root directory blocks at end
    
        fir_block = temp;
    }
    //EOC wasn't found
    return -1;
}

struct Root_Dir * create_root(){
    
    for(int i=0; i<128; i++){
        if(dir[i]==0){
		  dir[i]=1;
          return RD+i;
        }
    }
    return NULL;
}
int next_block(){
    int i =0;
    while(i<nDataBlocks){
        if(fat->f_table[i]==0){
            return i;
        }
        i++;
    }
    //FAT table is full
    return -1;
}
bool fd_exists(int fd)
{
	if(filedes[fd].fd_filename[0]=='\0'){
		return false;
	}
	
	return true;
	/* TODO: Phase 3 */
}

//phase 3 helper functions
static int fs_fd_init(int fd, char *filename)
{
	//filedes[fd] = malloc(sizeof(fs_filedes));
	//if (filedes[fd] == NULL)
	//	return -1;

	filedes[fd].fd_offset = 0;
	strcpy(filedes[fd].fd_filename, filename);

	return 0;
	/* TODO: Phase 3 */
}

int return_rd(int fd){
    
    for(int i=0; i<128; i++){
		//make sure file
        if((strcmp(RD[i].fname,fname)==0)){
          return i;
        }
    }
    return -1;
	/* TODO: Phase 3 */
}

bool fd_exists(int fd)
{
	if(filedes[fd].fd_filename[0]=='\0'){
		return false;
	}
	
	return true;
	/* TODO: Phase 3 */
}
