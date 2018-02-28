#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF
int bytes_to_block(int y);
typedef struct __attribute__((__packed__)) sBlock{
	
	char Sig[8];
	uint16_t tNumBlocks;
	uint16_t rdb_Index;
	uint16_t BlockStart;
	uint16_t nDataBlocks;
	uint8_t  nFAT_Blocks;
	char padding[4079];
}

typedef struct __attribute__((__packed__)) FAT{
	
	uint16_t *f_table;//will dynamically allocate, = nDataBlocks
}
typedef struct __attribute__((__packed__)) Root_Dir{
	
	char Filename[FS_FILENAME_LEN];
	uint32_t fSize;
	uint16_t  index;
	char padding[10];
}
int FS_Mount=0;
typedef struct sBlock * SB;
typedef struct Root_Dir * RD;
typedef struct FAT * fat;
/* TODO: Phase 1 */

int fs_mount(const char *diskname)
{
	int cmp, retVal;
	char signature[9] = {'E','C','S','1','5','0','F','S','\0'};
	SB = malloc(sizeof(sBlock));
	RD = malloc(FS_FILE_MAX_COUNT*sizeof(Root_Dir));
	
	if(block_read(0, (void*)SB)!=0){
		free(SB);
		free(RD);
		return -1;
	}
	SB = (sBlock*)SB;
	
	if(strcmp(signature, SB.Sig)!=0){
		free(SB);
		free(RD);
		return -1;
	}
	if(block_disk_count()!=SB.tNumBlocks){
		free(SB);
		free(RD);
		return -1;
	}
	fat->f_table =  malloc(SB->nDataBlocks *sizeof(FAT));
	fat->f_table[0]= FAT_EOC;
	
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
	if(FS_Mount==0)return -1;
	
	fprintf(stderr,"File System Info: ");
	fprintf(stderr,"Signature: %s",SB.Sig);
	fprintf(stderr,"Number of Total Number of Blocks: %d",SB.NumBlocks);//not sure if sould use %d
	fprintf(stderr,"Number of Datablocks: %d",SB.nDataBlocks);
	fprintf(stderr,"Number of FAT Blocks: %d",SB.nFAT_Blocks);
	
	return 0;
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
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

