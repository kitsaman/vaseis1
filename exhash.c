#include "BF.h"
#include "exhash.h"


int EH_CreateIndex(char *fileName, char* attrName, char attrType, int attrLength, int depth) {
    int blockFile, nextHash, i, j;
    EH_first* info = malloc(sizeof(EH_first));
    //info->fileName = malloc( (strlen(fileName)+1) * sizeof(char) );
    //info->attrName = malloc( (strlen(attrName)+1) * sizeof(char) );
    void *block;
    if(BF_CreateFile(fileName)<0) {
        BF_PrintError("Could not create file\n");
        return -1;
    }   
    blockFile = BF_OpenFile(fileName);
    if(blockFile<0) {
        BF_PrintError("Could not open file\n");
        BF_CloseFile(blockFile);
        return -1;
    }
    if(BF_AllocateBlock(blockFile)<0) {
        BF_PrintError("Could not allocate block\n");
        BF_CloseFile(blockFile);
        return -1;
    }
    strcpy(info->fileName,fileName);
    strcpy(info->attrName,attrName);
    info->attrType=attrType;
    info->attrLength=attrLength;
    info->globalDepth=depth;
    info->isHash=1;
    int maxBuckets = ( BLOCK_SIZE - (sizeof(int)) ) / sizeof(int);//number of buckets each block can hold
    int blockSum=1; //number of blocks we'll need to allocate
    int bucketsLeft = pow(2,depth); //buckets that still need to fit in a block
    while( bucketsLeft > maxBuckets ) {
        blockSum++;
        bucketsLeft -= maxBuckets;
    }
    if(BF_ReadBlock(blockFile,0,&block)<0) {
        BF_PrintError("Could not read block\n");
        BF_CloseFile(blockFile);
        return -1;
    }
    memcpy(block,info,sizeof(EH_first));
    if(BF_WriteBlock(blockFile,0)) {
        BF_PrintError("Could not write to block\n");
        BF_CloseFile(blockFile);
        return -1;
    }
    //free(info->fileName);
    //free(info->attrName);
    free(info);

    //hashTable construction
    for(i=0; i<blockSum; i++) { //allocate needed blocks
        int* hashTable;
        if(i==blockSum-1) {     //size of array
            hashTable = malloc( sizeof(int) * bucketsLeft);
            for (j=0; j<bucketsLeft; j++)
                hashTable[j] = -1;
        }
        else {
            hashTable = malloc( sizeof(int) * maxBuckets);
            for (j=0; j < maxBuckets; j++)
                hashTable[j] = -1;
        }
        if(BF_AllocateBlock(blockFile)<0) {
            BF_PrintError("Could not allocate block\n");
            BF_CloseFile(blockFile);
            return -1;
        }
        if(BF_ReadBlock(blockFile,i+1,&block)<0) {
            BF_PrintError("Could not read block\n");
            BF_CloseFile(blockFile);
            return -1;
        }
        if(i==blockSum-1) {
            nextHash = -1;
            memcpy(block,&nextHash,sizeof(int));
            memcpy(block+sizeof(int),hashTable,sizeof(int) * bucketsLeft);
        }
        else {
            nextHash = i + 2;
            memcpy(block,&nextHash,sizeof(int));
            memcpy(block+sizeof(int),hashTable,sizeof(int) * maxBuckets);
        }
        if(BF_WriteBlock(blockFile,i+1)) {
            BF_PrintError("Could not write to block\n");
            BF_CloseFile(blockFile);
            return -1;
        }
        free(hashTable);
    }
    printf("Created file with number of blocks = %d\n\n",BF_GetBlockCounter(blockFile));
    return 0;
}


EH_info* EH_OpenIndex(char *fileName) {
    int blockFile;
    void* block;
    EH_info* info;
    EH_first first_info;
    if((blockFile=BF_OpenFile(fileName))<0) {           //open the file
         BF_PrintError("Could not open file\n");
         return NULL;
    }
    if(BF_ReadBlock(blockFile,0,&block)<0) {            //read from the first block
         BF_PrintError("Could not read block\n");
         BF_CloseFile(blockFile);
         return NULL;
    }
    memcpy(&first_info,block,sizeof(EH_first));
    if(first_info.isHash!=1)            //check that the file is a hash file
        return NULL;
    info = malloc(sizeof(EH_info));             //allocate memory to save info
    //info->attrName = malloc( (strlen(first_info.attrName)+1) * sizeof(char) );
    info->fileDesc = blockFile;
    strcpy(info->attrName,first_info.attrName);
    info->attrType = first_info.attrType;
    info->attrLength = first_info.attrLength;
    info->globalDepth = first_info.globalDepth;
    info->initialBlocks = BF_GetBlockCounter(blockFile);
    return info;
} 



int EH_CloseIndex(EH_info* header_info) {
    if(BF_CloseFile(header_info->fileDesc)<0) {
        BF_PrintError("Could not close file\n");
        return -1;
    }
    free(header_info);
    printf("Closed file and freed memory\n");
    return 0;
}

int EH_InsertEntry(EH_info* header_info, Record record) {
    /* Add your code here */

    return -1;
   
}


int EH_GetAllEntries(EH_info header_info, void *value) {
    /* Add your code here */

    return -1;
   
}


int HashStatistics(char* filename) {
    /* Add your code here */

    return -1;
   
}


unsigned int hash_function_int(int hashsize,int num) {
    unsigned int hash_value;
    hash_value = num % hashsize;
    return hash_value;
}

unsigned int hash_function_char(int hashsize,char hash_name[20]) {
    unsigned long hash_value = 0;
    int i;
    while((i = *hash_name++))
        hash_value =  i + (hash_value << 6) + (hash_value << 16) - hash_value;
    hash_value = hash_value % hashsize;
    return (unsigned int)hash_value;
}

void print_record(Record rec) {
    printf("\nID=%d\nName=%s\nSurname=%s\nCity=%s\n",rec.id,rec.name,rec.surname,rec.city);
}