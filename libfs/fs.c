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
char * resize_buffer(char * buffer, int old_size, int * new_size);
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
	char null[1]={'\0'};
	//initialize empty entries
	for(int i =0; i<128; i++){
		
		strcpy(RD.fname,null);
	}
	fat->f_table =  calloc(SB->nDataBlocks *sizeof(FAT));//initialize all indices to 0
	read_in_FAT();
	//fat->f_table[0]= FAT_EOC;
	dir =  calloc(sizeof(int));
	FS_Mount=1;
	return 0;
	
	/* TODO: Phase 1 */
}


int fs_umount(void)
{
	update_RD();
	update_FAT();
	free(SB);
	free(fat->f_table);
	free(RD)
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
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	int i=0; char null[1]={'\0'};
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

