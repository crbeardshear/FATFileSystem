#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF
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
//phase 3 function prototypes
static int fs_fd_init(int fd, const char *filename);
int return_rd(char * fd_name);
int next_block();
int fd_exists(int fd);
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
	
	uint16_t *f_table;//will dynamically allocate, = nDataBlocks
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
   // int length; //used to keep track of how far we've gone into file (in bytes); depth is basically file size in bytes
	//may need more later

}t4;
int rd_total=0;//need to keep track of creates and deletes later on
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
	//int  retVal;, cmp;
	char signature[8] = {'E','C','S','1','5','0','F','S'};
	//const char *  = malloc(8*sizeof(char));
	//strcpy(signature,temp);
	SB = (struct sBlock*) calloc(1,sizeof(struct sBlock));
	RD = (struct Root_Dir*) calloc(FS_FILE_MAX_COUNT, sizeof(struct Root_Dir));//using calloc to initialize pointers
	fat= (struct FAT*) calloc(1,sizeof(struct FAT));
	//filename cannot be a NULL terminator
	if(diskname[0]=='\0'){
		return -1;
	}
	//fprintf(stderr,"1......");
	//open disk
	if(block_disk_open(diskname)!=0){
		fprintf(stderr,"block_disk_open FAIL\n");
		free(SB);
		free(RD);
		return -1;
	}
	//fprintf(stderr,"2........");
	//read in superblock
	if(block_read(0, (void*)SB)!=0){
		fprintf(stderr,"block_read(0, (void*)SB FAIL\n");
		free(SB);
		free(RD);
		return -1;
	}
	SB = (struct sBlock*)SB;
	//fprintf(stderr,"3........SB->f_block_start%d ", SB->f_block_start);
	//used strncmp instead strcmp becasue signature 
	//is not NULL terminated
	if(strncmp(signature, SB->Sig,8)!=0){
		fprintf(stderr,"strncmp(signature, SB->Sig,8) FAIL\n");
		free(SB);
		free(RD);
		return -1;
	}
	//fprintf(stderr,"3........SB->tNumBlocks: %d",SB->tNumBlocks);
	if(block_disk_count()!=SB->tNumBlocks){
		fprintf(stderr,"block_disk_count()!=SB->tNumBlocks FAIL\n");
		free(SB);
		free(RD);
		return -1;
	}
	//char null[1]={'\0'};
	//initialize empty entries
	for(int i =0; i<128; i++){
		RD[i].fname[0]='\0';//null
		//strcpy(RD[i].fname,null);
	}
	//fprintf(stderr,"4........");
	//create file decriptor table
	filedes = calloc(FS_OPEN_MAX_COUNT, sizeof(struct fs_filedes));//using calloc o initialize pointers
	//initialize name of each file decriptor name to '\0'
	for(int i =0; i<FS_OPEN_MAX_COUNT; i++){
		filedes[i].fd_filename[0] = '\0';
		//strcpy(filedes[i].fd_filename,null);
	}
	
	fat->f_table =  calloc(SB->nDataBlocks, sizeof(struct FAT));//initialize all indices to 0
	//fprintf(stderr,"6........SB->nDataBlocks: %d",SB->nDataBlocks);
	if(read_in_FAT()!=0){
		fprintf(stderr,"READ FAT FAIL\n");
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
			   //fprintf(stderr,"used FAT blocks %d\n",next_block());
           if(read_in_RD()!=0){
		      fprintf(stderr,"READ RD FAIL\n");
		      return -1;
	       }
		      break;
       }
       }
	//fat->f_table[0]= FAT_EOC; First index of FAT should alread have FAT_EOC
	dir =  calloc(FS_FILE_MAX_COUNT, sizeof(int));
	FS_Mount=1;
	//fprintf(stderr,"5........");
	return 0;
	
	/* TODO: Phase 1 */
}


int fs_umount(void)
{
	//check if a virtual disk is open
	//Check if there are open file descriptors
	if(FS_Mount==0||fd_total>0){
		fprintf(stderr,"FS_Mount==0||fd_total>0\n");
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
	if(block_disk_close()){
		fprintf(stderr,"Error block_disk_close\n");
		return -1;
	}
	//fprintf(stderr,"fd_total at time of unmount %d\n",fd_total);
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
	
  /*FS Info:
    total_blk_count=8198
    fat_blk_count=4
    rdir_blk=5
    data_blk=6
    data_blk_count=8192
    fat_free_ratio=8191/8192
    rdir_free_ratio=128/128*/

	fprintf(stderr,"FS Info:\n");
	//fprintf(stderr,"Signature: %s\n",SB->Sig);
	fprintf(stderr,"total_blk_count=%d\n",SB->tNumBlocks);//not sure if sould use %d
	fprintf(stderr,"fat_blk_count%d\n",SB->nFAT_Blocks);
	fprintf(stderr,"rdir_blk=%d\n",SB->rdb_Index);
	fprintf(stderr,"data_blk=%d\n",SB->d_block_start);
	fprintf(stderr,"data_blk_count=%d\n",SB->nDataBlocks);//not sure if sould use %d
	
	
	
	return 0;
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	//check if ptr is valid
	if (filename == NULL)
		return -1;
    if (filename[0] == '\0')
		return -1;
	//check if filename is valid
	int i = 0;
	while (filename[i] != '\0') {
		if (i+1==FS_FILENAME_LEN&&filename[i+1] != '\0'){
			fprintf(stderr,"No NULL Terminator\n");
			return -1;
		}
		i++;
	}
	struct Root_Dir * new_file = create_root(filename);//Root_Dir and dir have same index for this file
	strcpy(new_file->fname,filename);
	new_file->fSize=0;
	new_file->f_index=next_block();//
	fat->f_table[new_file->f_index]=FAT_EOC;
	//fprintf(stderr,"test create %d\n",new_file->f_index);
	return 0;
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	int i=0; char null[1]={'\0'};
	//check to see if filename is open in any file descriptors
	for(int i=0; i<32;i++){
		if(strcmp(filedes[i].fd_filename,filename)==0){
			//fprintf(stderr,"strcmp(filedes[i].fd_filename,filename\n");
		    return -1;
	    }
	}
	if(file_exist(filename)!=0||(strncmp(filename,null,1)==0)){
		//fprintf(stderr,"file_exist(filename)!=0||(strncmp(filename,null,1)==0\n");
		return -1;
	}
	while(i<128){
		if((strcmp(RD[i].fname,filename)==0)){
			delete_file(RD[i].f_index);
			delete_root(filename);
			strcpy(RD[i].fname, null);
	        RD[i].fSize=0;//not necessarry specified as xxx
	        RD[i].f_index=FAT_EOC;//not necessarry specified as xxx
			break;
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
        if(RD[i].fname[0]!='\0'){
            fprintf(stderr,"%s\n",RD[i].fname);
        }
	
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
	//fprintf(stderr,"fd_open: %d\n",fd_total);
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
	//fprintf(stderr,"fd_close: %d",fd_total);
	return 0;
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	
    if((fd_exists(fd))||fd>=32||fd<0){
       return -1;
    }
    else{
		for(int i=0; i<128;i++){
			//make sure file still exists
			if(strcmp(RD[i].fname,filedes[fd].fd_filename)){
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
	
	return 0;
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	return 0;
	/* TODO: Phase 4 */
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
	
	int size=0, y=0;// ret = 0;
	char *new_array =resize_buffer((char *)fat->f_table,SB->nDataBlocks*sizeof(uint16_t),&size);
	int retVals[SB->rdb_Index-1];
	for(int i =1; i<SB->rdb_Index; i++){
		y=i-1;
		retVals[i]=block_read(i,new_array+(y*BLOCK_SIZE));
		if(retVals[i]==-1){
			free(new_array);
			return -1;
		}
	}
	uint16_t * upFAT=(uint16_t*)new_array;
	for(int i =0; i<SB->nDataBlocks; i++){//the indexing may lead to a segfault
		fat->f_table[i]=upFAT[i];
	}
	
	/*for(int i=0; i<SB->nDataBlocks; i++){
		if(fat->f_table[i]!=0){
			fprintf(stderr,"Used Fat block %d: %d\n",i,fat->f_table[i]);
		}
	}*/
	
	free(new_array);
	return 0;
	/* TODO: Phase 1 */
}
int update_FAT(){
	
	int size=0,y=0;// ret = 0;
	char *new_array =resize_buffer((char*)fat->f_table,SB->nDataBlocks*sizeof(uint16_t),&size);
	int retVals[SB->rdb_Index-1];
	//fprintf(stderr,"New size %d",size);
	for(int i=1; i<SB->rdb_Index; i++){
		y=i-1;
		retVals[i]=block_write(i,fat->f_table+(y*BLOCK_SIZE));
		if(retVals[i]==-1){
			free(new_array);
			return -1;
		}
	}
	/*for(int i=0; i<SB->nDataBlocks; i++){
		if(fat->f_table[i]!=0){
			fprintf(stderr,"Used Fat blocks %d\n",i);
		}
	}*/
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
    
    for(int i=0; i<128; i++){
		//dir parallels Root_Directory
        if(strcmp(RD[i].fname,fname)==0){
            RD[i].fname[0]='\0';
          return 0;
        }
    }
    return -1;
}

int file_exist(const char * fname){
    
    for(int i=0; i<128; i++){
        if((strcmp(RD[i].fname,fname)==0)){
          return 0;
        }
    }
    return -1;
}

int delete_file(int fir_block){
    int temp =0; 
	//char * null= calloc(BLOCK_SIZE, sizeof(char));
	//memset( null, '\0', BLOCK_SIZE *sizeof(char) );
    
    //bool not_at_end=false;
    for(int i=0; i<SB->nDataBlocks; i++){
        if(fat->f_table[fir_block]==FAT_EOC){
            fat->f_table[fir_block]=0;
		    //zero out corresponding data block, might not be necesary
            //block_write(SB.f_block_start+fir_block,null);
		
            return 0;
        }
        temp = fat->f_table[fir_block];
        fat->f_table[fir_block]=0;
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

struct Root_Dir * create_root(const char *file_n){
    
    for(int i=0; i<128; i++){
          if(RD[i].fname[0]=='\0'){
			strcpy(RD[i].fname, file_n);
          return RD+i;
          }
        /*if(dir[i]==0){
		  dir[i]=1;
          return RD+i;
        }*/
    }
    return NULL;
}
int next_block(){
    int i =0;
    while(i<SB->nDataBlocks){
        if(fat->f_table[i]==0){
            return i;
        }
        i++;
    }
    //FAT table is full
    return -1;
}

//phase 3 helper functions
static int fs_fd_init(int fd, const char *filename)
{
	//filedes[fd] = malloc(sizeof(fs_filedes));
	//if (filedes[fd] == NULL)
	//	return -1;

	filedes[fd].fd_offset = 0;
	strcpy(filedes[fd].fd_filename, filename);

	return 0;
	/* TODO: Phase 3 */
}

int return_rd(char * fd_name){
    
    for(int i=0; i<128; i++){
		//make sure file
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
