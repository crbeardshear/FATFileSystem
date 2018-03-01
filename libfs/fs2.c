#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF
int bytes_to_block(int y);
int delete_file(int fir_block);
int create_root();
char * resize_buffer(char * buffer);
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
	fat->f_table =  calloc(SB->nDataBlocks *sizeof(FAT));//initialize all indices to 0
	fat->f_table[0]= FAT_EOC;
	dir =  calloc(sizeof(int));
	FS_Mount=1;
	return 0;
	
	/* TODO: Phase 1 */
}

//for section 4
int bytes_to_block(int y){
	int i=0;
	while((BLOCK_SIZE*i)<y){	
		i++;
	}
	return i;
}
int fs_umount(void)
{
	/* TODO: Phase 1 */
}

int fs_info(void)
{
	if(FS_Mount==0){
		return -1;
	}
	else{
	fprintf(stderr,"File System Info\n");
	fprintf(stderr,"Signature: %s",SB.Sig);
	fprintf(stderr,"Number of Total Number of Blocks: %d\n",SB.NumBlocks);//not sure if sould use %d
	fprintf(stderr,"Number of Datablocks: %d\n",SB.nDataBlocks);
	fprintf(stderr,"Number of FAT Blocks: %d\n",SB.nFAT_Blocks);
	}
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
	
	char * resize_array= resize_buffer(fat->f_table);
	//Fat table starts at second index
	for(int i=0; i<sizeof(resize_array/BLOCK_SIZE); i++){
	block_write(1+i, resize_array+(i*BLOCK_SIZE);// make an update fat table function
	}
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	int i=0;
	if(file_exist(filename)!=0){
		return -1;
	}
	while(i<128){
		if((strcmp(RD[i].fname,filename)!=0){
			delete_file(RD.f_index);
			delete_root(filename);
		}
		i++;
	}
	
	
	
	
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

char * resize_buffer(char * buffer){
	char * new_buffer;
    if((sizeof(buffer)%BLOCK_SIZE)==0){
		new_buffer = calloc(sizeof(buffer)* sizeof(char));
		for(int i=0; i<sizeof(buffer); i++){
         new_buffer[i]=buffer;
        }
		return new_buffer;
	}
	else{
		//duplicating old buffer into new buffer that is 
		//a multiple of BLOCK_SIZE
		int blocks = sizeof(buffer)/BLOCK_SIZE;
		blocks=blocks+1;
        new_buffer = calloc(blocks * sizeof(char));
		for(int i=0; i<blocks; i++){
         new_buffer[i]=buffer;
        }
		return new_buffer;
	
	}
    
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
    
    //bool not_at_end=false;
    for(int i=0; i<nDataBlocks; i++){
    if(fat->f_table[fir_block]==FAT_EOC){
        fat->f_table[fir_block]=0;
        return 0;
    }
    temp = f_table[fir_block];
    fat->f_table[f_block]=0;
    
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