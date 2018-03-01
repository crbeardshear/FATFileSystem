void update_RD(){
	//write root directory block to first data block index
	block_write(SB.f_block_start,RD);
	
}
void read_in_FAT(){
	
	int size=0;
	char *new_array =resize_buffer(fat.f_table,SB.nDataBlocks*sizeof(uint16_t),&size);
	
	for(int i =1; i<SB.f_block_start; i++){
		y=i-1;
		block_read(i,new_array+(y*BLOCK_SIZE));
	}
	uint16_t * upFAT=(uint16_t*)new_array;
	for(int i =0; i<nDataBlocks; i++){//the indexing may lead to a segfault
		fat->f_table[i]=upFAT[i];
	}
	free(new_array);
}
void update_FAT(){
	
	int size=0;
	char *new_array =resize_buffer((char*)fat.f_table,SB.nDataBlocks*sizeof(uint16_t),&size);
	
	for(int i =1; i<SB.f_block_start; i++){
		y=i-1;
		block_write(i,fat->f_table+(y*BLOCK_SIZE));
	}
	free(new_array);
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
	char * null= calloc(BLOCK_SIZE *sizeof(char));
	memset( null, '\0', BLOCK_SIZE *sizeof(char) );
    
    //bool not_at_end=false;
    for(int i=0; i<nDataBlocks; i++){
    if(fat->f_table[fir_block]==FAT_EOC){
        fat->f_table[fir_block]=0;
		//zero out corresponding data block  
        block_write(SB.f_block_start+fir_block,null);
		
        return 0;
    }
    temp = f_table[fir_block];
    fat->f_table[f_block]=0;
	//zero out corresponding data block, cannot ignore data in
    //data blocks, it may accidentally be read in later on fs_read	
    block_write(SB.f_block_start+fir_block,null);
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