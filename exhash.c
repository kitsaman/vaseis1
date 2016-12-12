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
    for(i=1; i<=blockSum; i++) { //allocate needed blocks
        int* hashTable;
        if(i == blockSum-1) {     //size of array
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
        if(BF_ReadBlock(blockFile,i,&block)<0) {
            BF_PrintError("Could not read block\n");
            BF_CloseFile(blockFile);
            return -1;
        }
        if(i == blockSum) {
            nextHash = -1;
            memcpy(block,&nextHash,sizeof(int));
            memcpy(block+sizeof(int),hashTable,sizeof(int) * bucketsLeft);
        }
        else {
            nextHash = i + 1;
            memcpy(block,&nextHash,sizeof(int));
            memcpy(block+sizeof(int),hashTable,sizeof(int) * maxBuckets);
        }
        if(BF_WriteBlock(blockFile,i)) {
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
    if(first_info.isHash != 1)            //check that the file is a hash file
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
    unsigned int hash_value;
    int numBuckets, numRecords, localDepth, bucketValue, i;
    int maxRecords = (BLOCK_SIZE-(2*sizeof(int)))/sizeof(Record); 
    void* block;

    // Compute hash value
    numBuckets = pow(2,header_info->globalDepth);
    if(header_info->attrType=='i' && strcmp(header_info->attrName,"id")==0)         //use hash_function_int
        hash_value = hash_function_int(numBuckets,record.id);
    else if(header_info->attrType=='c') {                                            //use hash_function_char
        if(strcmp(header_info->attrName,"city")==0)
            hash_value = hash_function_char(numBuckets,record.city);
        else if(strcmp(header_info->attrName,"name")==0)
            hash_value = hash_function_char(numBuckets,record.name);
        else if(strcmp(header_info->attrName,"surname")==0)
            hash_value = hash_function_char(numBuckets,record.surname);
        else{
            printf("Incorrect 'c' attrType\n");
            return -1;
        }
    }
    else{
        printf("Incorrect attrType\n");
        return -1;
    }
    printf("Hash value %d\n",hash_value);
    int maxBuckets = ( BLOCK_SIZE - sizeof(int) ) / sizeof(int);          //number of buckets each block can hold
    int hashBlocks=1, nextHashBlock, newBlock;
    while(hash_value>=maxBuckets) {             //Find correct hash table block
        hashBlocks++;
        hash_value -= maxBuckets;
    }
    if(BF_ReadBlock(header_info->fileDesc,1,&block)<0) {
        BF_PrintError("Could not read block\n");
        BF_CloseFile(header_info->fileDesc);
        return -1;
    }
    for(i=1; i < hashBlocks; i++){
        memcpy(&nextHashBlock,block,sizeof(int));                                       //check which block has the next part of the hash table
        if(BF_ReadBlock(header_info->fileDesc,nextHashBlock,&block)<0) {                 //read that block
            BF_PrintError("Could not read block\n");
            BF_CloseFile(header_info->fileDesc);
            return -1;
        }
    }
    memcpy(&bucketValue,block+sizeof(int)+hash_value*sizeof(int),sizeof(int));
    if(bucketValue == -1) {
        if(BF_AllocateBlock(header_info->fileDesc)<0) {                  //create new block
            BF_PrintError("Could not allocate block\n");
            BF_CloseFile(header_info->fileDesc);
            return -1;
        }
        if((newBlock=BF_GetBlockCounter(header_info->fileDesc))<0) {    //get number of newly created block
            BF_PrintError("Could not get block counter\n");
            BF_CloseFile(header_info->fileDesc);
            return -1;
        }
        newBlock--;
        printf("New block has number %d\n",newBlock);
        memcpy(block+sizeof(int)+hash_value*sizeof(int),&newBlock,sizeof(int));    //write to hashtable block the corresponding block for this hash value
        if(BF_WriteBlock(header_info->fileDesc,nextHashBlock)<0) {
            BF_PrintError("Could not write to block\n");
            BF_CloseFile(header_info->fileDesc);
            return -1;
        }
        if(BF_ReadBlock(header_info->fileDesc,newBlock,&block)<0) {     //read the block for this hash value
            BF_PrintError("Could not read block\n");
            BF_CloseFile(header_info->fileDesc);
            return -1;
        }
        numRecords = 1;
        memcpy(block,&numRecords,sizeof(int));
        localDepth = header_info->globalDepth;
        memcpy(block+sizeof(int),&localDepth,sizeof(int));
        memcpy(block+2*sizeof(int),&record,sizeof(Record));             //copy the record after the space for the number of records in block and local depth of block
        if(BF_WriteBlock(header_info->fileDesc,newBlock)<0) {
            BF_PrintError("Could not write to block\n");
            BF_CloseFile(header_info->fileDesc);
            return -1;
        }
    }
    else {
        if(BF_ReadBlock(header_info->fileDesc,bucketValue,&block)<0) {
            BF_PrintError("Could not read block\n");
            BF_CloseFile(header_info->fileDesc);
            return -1;
        }
        memcpy(&numRecords,block,sizeof(int));
        if(numRecords == maxRecords) {                                  //no more space in this block
            memcpy(&localDepth,block+sizeof(int),sizeof(int));          //need to check local depth
            if(localDepth < header_info->globalDepth) {                 //only need to split the content of the block
                Record* temp;
                temp = malloc( sizeof(Record) * numRecords);            //need to temporarily save the records
                memcpy(temp,block+2*sizeof(int),numRecords*sizeof(Record));
                numRecords = 0;
                memcpy(block,&numRecords,sizeof(int));
                localDepth++;
                memcpy(block+sizeof(int),&localDepth,sizeof(int));
                memset(block+2*sizeof(int),0,BLOCK_SIZE-2*sizeof(int)); //empty the block from records
                if(BF_WriteBlock(header_info->fileDesc,bucketValue)<0) {
                    BF_PrintError("Could not write to block\n");
                    BF_CloseFile(header_info->fileDesc);
                    return -1;
                }
                for(i=0;i<numRecords;i++) {                             //reinsert all the records
                    EH_InsertEntry(header_info,temp[i]);
                }
                free(temp);
            }
            else {                                                      //need to double the size of hashtable
                (header_info->globalDepth)++;
            }
        }
        else {
            memcpy(block+numRecords*sizeof(Record)+2*sizeof(int),&record,sizeof(Record));       //add record at the end
            numRecords++;
            memcpy(block,&numRecords,sizeof(int));                                              //+1 number of records
            if(BF_WriteBlock(header_info->fileDesc,bucketValue)<0) {                            //write to block
                BF_PrintError("Could not write to block\n");
                BF_CloseFile(header_info->fileDesc);
                return -1;
            }
            printf("Inserted record at block number %d\n\n",bucketValue);
        }
    }
    return 0;
}


int EH_GetAllEntries(EH_info header_info, void *value) {
    int foundRecords, numBuckets, bucketValue, i;
    int id;
    char str[25];
    unsigned int hash_value;
    Record temp_record;
    void* block;

    // Compute hash value
    numBuckets = pow(2,header_info.globalDepth);
    if(header_info.attrType=='i' && strcmp(header_info.attrName,"id")==0) {         //use hash_function_int
        id = *((int *) value);
        hash_value = hash_function_int(numBuckets,id);
    }
    else if(header_info.attrType=='c') {                                            //use hash_function_char
        strcpy(str,value);
        if(strcmp(header_info.attrName,"city")==0)
            hash_value = hash_function_char(numBuckets,str);
        else if(strcmp(header_info.attrName,"name")==0)
            hash_value = hash_function_char(numBuckets,str);
        else if(strcmp(header_info.attrName,"surname")==0)
            hash_value = hash_function_char(numBuckets,str);
        else{
            printf("Incorrect 'c' attrType\n");
            return -1;
        }
    }
    else{
        printf("Incorrect attrType\n");
        return -1;
    }

    printf("Hash value %d\n",hash_value);
    int maxBuckets = ( BLOCK_SIZE - sizeof(int) ) / sizeof(int);          //number of buckets each block can hold
    int hashBlocks=1, numRecords, nextHashBlock;
    while(hash_value>=maxBuckets) {             //Find correct hash table block
        hashBlocks++;
        hash_value -= maxBuckets;
    }

    if(BF_ReadBlock(header_info.fileDesc,1,&block)<0) {
        BF_PrintError("Could not read block\n");
        BF_CloseFile(header_info.fileDesc);
        return -1;
    }
    for(i=1; i < hashBlocks; i++){
        memcpy(&nextHashBlock,block,sizeof(int));                                       //check which block has the next part of the hash table
        if(BF_ReadBlock(header_info.fileDesc,nextHashBlock,&block)<0) {                 //read that block
            BF_PrintError("Could not read block\n");
            BF_CloseFile(header_info.fileDesc);
            return -1;
        }
    }
    memcpy(&bucketValue,block+sizeof(int)+hash_value*sizeof(int),sizeof(int));
    if(bucketValue == -1){
        printf("No record with this value\n");
        return 0;
    }
    if(BF_ReadBlock(header_info.fileDesc,bucketValue,&block)<0) {
        BF_PrintError("Could not read block\n");
        BF_CloseFile(header_info.fileDesc);
        return -1;
    }
    foundRecords = 0;
    memcpy(&numRecords,block,sizeof(int));                                              //get number of records in this block
    for(i=0; i<numRecords; i++) {
        memcpy(&temp_record,block+2*sizeof(int)+i*sizeof(Record),sizeof(Record));
        if(header_info.attrType=='i' && strcmp(header_info.attrName,"id")==0) {         //use hash_function_int
            if(temp_record.id == id) {
                print_record(temp_record);
                foundRecords++;
            }
        }
        else if(header_info.attrType == 'c') {                                          //use hash_function_char
            if(strcmp(header_info.attrName,"city") == 0 && strcmp(temp_record.city,str) == 0) {
                print_record(temp_record);
                foundRecords++;
            }
            else if(strcmp(header_info.attrName,"name") == 0 && strcmp(temp_record.name,str) == 0) {
                print_record(temp_record);
                foundRecords++;
            }
            else if(strcmp(header_info.attrName,"surname") == 0 && strcmp(temp_record.surname,str) == 0) {
                print_record(temp_record);
                foundRecords++;
            }
        }
    }
    printf("\nNumber of records that were read = %d\n", numRecords);
    printf("Number of records that had the value = %d\n\n", foundRecords);
    return foundRecords;
}


int HashStatistics(char* filename) {
    int numBlocks, numBuckets, bucketsLeft, i, j;
    int maxBuckets = ( BLOCK_SIZE - sizeof(int) ) / sizeof(int);//number of buckets each block can hold
    int hashBlocks = 1, nextHashBlock, blockValue, currentBlock;
    int leastRecords, mostRecords, blockRecords, totalRecords;
    double averageRecords;
    EH_info* info = EH_OpenIndex(filename);
    void* block;
    if((numBlocks=BF_GetBlockCounter(info->fileDesc))<0) {          //got number of blocks the file has
        BF_PrintError("Could not get block counter\n");
        BF_CloseFile(info->fileDesc);
        return -1;
    }
    numBuckets = pow(2,info->globalDepth);
    bucketsLeft = numBuckets;
    while(bucketsLeft>=maxBuckets) {                        //Find correct hash table block
        hashBlocks++;
        bucketsLeft -= maxBuckets;
    }
    nextHashBlock = 1;
    if(BF_ReadBlock(info->fileDesc,nextHashBlock,&block)<0) {                           //read first block of hash table
        BF_PrintError("Could not read block\n");
        BF_CloseFile(info->fileDesc);
        return -1;
    }
    leastRecords = 8;
    mostRecords = 0;
    totalRecords = 0;
    for(i=1; i < hashBlocks+1; i++){
        currentBlock = nextHashBlock;
        memcpy(&nextHashBlock,block,sizeof(int));                                       //check which block has the next part of the hash table
        if(i < hashBlocks - 1) {
            for(j=0; j<maxBuckets; j++) {
                memcpy(&blockValue,block+sizeof(int)+j*sizeof(int),sizeof(int));
                if(blockValue != -1) {
                    if(BF_ReadBlock(info->fileDesc,blockValue,&block)<0) {
                        BF_PrintError("Could not read block\n");
                        BF_CloseFile(info->fileDesc);
                        return -1;
                    }
                    memcpy(&blockRecords,block,sizeof(int));
                    if(blockRecords < leastRecords)
                        leastRecords = blockRecords;
                    if(blockRecords > mostRecords)
                        mostRecords = blockRecords;
                    printf("Bucket with value %d has %d records\n\n", ((i - 1)*maxBuckets)+j,blockRecords);
                    totalRecords += blockRecords;
                    if(BF_ReadBlock(info->fileDesc,currentBlock,&block)<0) {
                        BF_PrintError("Could not read block\n");
                        BF_CloseFile(info->fileDesc);
                        return -1;
                    }
                }
                else
                    printf("Bucket with value %d doesn't have any records\n\n", ((i - 1)*maxBuckets)+j);                    
            }
            bucketsLeft -= maxBuckets;
        }
        else {
            for(j=0; j<bucketsLeft; j++) {
                memcpy(&blockValue,block+sizeof(int)+j*sizeof(int),sizeof(int));
                if(blockValue != -1) {
                    if(BF_ReadBlock(info->fileDesc,blockValue,&block)<0) {
                        BF_PrintError("Could not read block\n");
                        BF_CloseFile(info->fileDesc);
                        return -1;
                    }
                    memcpy(&blockRecords,block,sizeof(int));
                    if(blockRecords < leastRecords)
                        leastRecords = blockRecords;
                    if(blockRecords > mostRecords)
                        mostRecords = blockRecords;
                    printf("Bucket with value %d has %d records\n\n", ((i - 1)*maxBuckets)+j,blockRecords);
                    totalRecords += blockRecords;
                    if(BF_ReadBlock(info->fileDesc,currentBlock,&block)<0) {
                        BF_PrintError("Could not read block\n");
                        BF_CloseFile(info->fileDesc);
                        return -1;
                    }
                }
                else
                    printf("Bucket with value %d doesn't have any records\n\n", ((i - 1)*maxBuckets)+j);
            }
        }
        if(i < hashBlocks) {
            if(BF_ReadBlock(info->fileDesc,nextHashBlock,&block)<0) {                 //read next hashtable block
                BF_PrintError("Could not read block\n");
                BF_CloseFile(info->fileDesc);
                return -1;
            }
        }
    }
    averageRecords = (double)totalRecords/(double)numBuckets;
    printf("Total number of blocks = %d\n",numBlocks);
    printf("Least number of records in a bucket = %d\n",leastRecords);
    printf("Biggest number of records in a bucket = %d\n",mostRecords);
    printf("Average number of records in a bucket = %.03f\n",averageRecords);
    return 0; 
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