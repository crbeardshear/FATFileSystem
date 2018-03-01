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
};

typedef struct __attribute__((__packed__)) FAT{
	
	uint16_t *f_table;//will dynamically allocate, = nDataBlocks
};

typedef struct __attribute__((__packed__)) Root_Dir{
	
	char Filename[FS_FILENAME_LEN];
	uint32_t fSize;
	uint16_t  index;
	char padding[10];
};

int FS_Mount=0;
int * dir; //keeps track of which index in the directory is being used
struct sBlock * SB;
struct Root_Dir * RD;
struct FAT * fat;

/*
 * file descriptors are only visible at file system level, so they don't need to
 * be packed
 */
struct fs_filedes {
	int fd_offset;
	char *fd_filename;
	//may need more later
};

struct fs_filedes *filedes[32];

static int fs_fd_init(int fd, char *filename)
{
	filedes[fd] = malloc(sizeof(fs_filedes));
	if (filedes[fd] == NULL)
		return -1;

	filedes[fd].fd_offset = 0;
	strcpy(filedes[fd].fd_filename, filename);

	return 0;
}

/* TODO: Phase 1 */

int fs_mount(const char *diskname)
{
	int cmp, retVal; //unused?
	char signature[9] = {'E','C','S','1','5','0','F','S','\0'};
	SB = malloc(sizeof(sBlock));
	RD = malloc(FS_FILE_MAX_COUNT*sizeof(Root_Dir));

	//need to open the disk before reads
	
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
	if(FS_Mount==0)return -1;
	
	fprintf(stderr,"File System Info: ");
	fprintf(stderr,"Signature: %s",SB.Sig);
	fprintf(stderr,"Number of Total Number of Blocks: %d",SB.NumBlocks);//not sure if sould use %d
	fprintf(stderr,"Number of Datablocks: %d",SB.nDataBlocks);
	fprintf(stderr,"Number of FAT Blocks: %d",SB.nFAT_Blocks);
	
	return 0;
	/* TODO: Phase 1 */
}

/**
 * fs_create - Create a new file
 * @filename: File name
 *
 * Create a new and empty file named @filename in the root directory of the
 * mounted file system. String @filename must be NULL-terminated and its total
 * length cannot exceed %FS_FILENAME_LEN characters (including the NULL
 * character).
 *
 * Return: -1 if @filename is invalid, if a file named @filename already exists,
 * or if string @filename is too long, or if the root directory already contains
 * %FS_FILE_MAX_COUNT files. 0 otherwise.
 */
int fs_create(const char *filename)
{
	//check if root dir contains > FS_FILE_MAX_COUNT files

	//check if filename ptr is NULL
	if (filename == NULL)
		return -1;

	//check if filename already exists
	

	//check if param filename is valid
	int i = 0;
	while (filename[i] != '\0') {
		if (i > FS_FILENAME_LEN)
			return -1;
		i++;
	}

	//TODO: implement
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
	while (filedes[j] != NULL) {
		if (j > FS_OPEN_MAX_COUNT)
			return -1;
		j++;
	}

	//at this point, j == the open filedes spot
	//initialize the file descriptor
	fs_fd_init(j, filename);

	return j;
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

