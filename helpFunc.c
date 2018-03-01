/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/

#include <stdio.h>
#include <stdint.h>
int main()
{
    uint16_t f=0xFFFF;
    printf("Hello World %u",(unsigned)f);

    return 0;
}


struct Root_Dir * next_root(){
    
    for(int i=0; i<128; i++){
        if(dir[i]==0){
          return RD+i;
        }
    }
    return NULL;
}
int delete_block(int fir_block){
    int temp =0, i=0;
    
    //bool not_at_end=false;
    while(i<nDataBlocks){
    if(fat->f_table[fir_block]==FAT_EOC){
        fat->f_table[fir_block]=0;
        return 0;
    }
    temp = f_table[fir_block];
    fat->f_table[f_block]=0;
    
    fir_block = temp;
    i++;
    }
    //EOC wasn't found
    return -1;
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

