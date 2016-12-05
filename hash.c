#include "BF.h"
#include "hash.h"

int HT_CreateIndex( char *fileName, char attrType, char* attrName, int attrLength, int buckets) {
	int blockFile, bucketsN, i;
	HT_first* info = malloc(sizeof(HT_first));
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
	info->size=buckets;
	info->isHash=1;
	int maxBuckets = 512 / sizeof(int);//number of buckets each block can hold
	int blockSum=1; //number of blocks we'll need to allocate
	int bucketsLeft = buckets; //buckets that still need to fit in a block
	while( bucketsLeft > maxBuckets ){
		blockSum++;
		bucketsLeft -= maxBuckets;
	}
	info->initialBlocks=blockSum+1;
	if(BF_ReadBlock(blockFile,0,&block)<0) {
 	 	BF_PrintError("Could not read block\n");
 	 	BF_CloseFile(blockFile);
  		return -1;
  	}
	memcpy(block,info,sizeof(HT_first));
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
		if(i==blockSum-1)		//size of array
			hashTable = malloc( sizeof(int) * bucketsLeft);	
		else
			hashTable = malloc( sizeof(int) * maxBuckets);
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
  		if(i==blockSum-1)
			memcpy(block,hashTable,sizeof(int) * bucketsLeft);
		else
			memcpy(block,hashTable,sizeof(int) * maxBuckets);
		if(BF_WriteBlock(blockFile,i+1)) {
			BF_PrintError("Could not write to block\n");
 		 	BF_CloseFile(blockFile);
  			return -1;
		}
  		free(hashTable);
	}
	printf("Created file!\n");
	return 0;
}

HT_info* HT_OpenIndex(char *fileName) {
    int blockFile;
    void* block;
    HT_info* info;
    HT_first first_info;
    if((blockFile=BF_OpenFile(fileName))<0) {		//open the file
  		 BF_PrintError("Could not open file\n");
  		 return NULL;
  	}
    if(BF_ReadBlock(blockFile,0,&block)<0){			//read from the first block
  		 BF_PrintError("Could not read block\n");
  		 BF_CloseFile(blockFile);
  		 return NULL;
  	}
  	memcpy(&first_info,block,sizeof(HT_first));
  	if(first_info.isHash!=1)					//check that the file is a hash file
  		return NULL;
  	info = malloc(sizeof(HT_info));												//allocate memory to save info
  	//info->attrName = malloc( (strlen(first_info.attrName)+1) * sizeof(char) );
  	info->fileDesc = blockFile;
  	strcpy(info->attrName,first_info.attrName);
  	info->attrType = first_info.attrType;
  	info->attrLength = first_info.attrLength;
  	info->numBuckets = first_info.size;
  	info->initialBlocks = first_info.initialBlocks;
  	printf("Name=%s\nType=%c\nLength=%d\nBuckets=%ld\nBlocks=%d\n",info->attrName,info->attrType,info->attrLength,info->numBuckets,info->initialBlocks);
  	return info;
} 


int HT_CloseIndex( HT_info* header_info ) {
    if(BF_CloseFile(header_info->fileDesc)<0){
  		BF_PrintError("Could not close file\n");
  		return -1;
  	}
    //free(header_info->attrName);
    free(header_info);
    printf("Closed file and freed memory\n");
    return 0;
}

int HT_InsertEntry(HT_info header_info, Record record) {
	unsigned int hash_value;
	int blockFile;
	void* block;

	// Compute hash value
    if(header_info.attrType=='i' && strcmp(header_info.attrName,"id")==0)			//use hash_function_int
    	hash_value = hash_function_int(header_info.numBuckets,record.id);
    else if(header_info.attrType=='c') {											//use hash_function_char
    	if(strcmp(header_info.attrName,"city")==0)
    		hash_value = hash_function_char(header_info.numBuckets,record.city);
    	else if(strcmp(header_info.attrName,"name")==0)
    		hash_value = hash_function_char(header_info.numBuckets,record.name);
    	else if(strcmp(header_info.attrName,"surname")==0)
    		hash_value = hash_function_char(header_info.numBuckets,record.surname);
    	else
    		return -1;
    }
    else
    	return -1;

    // Check if hash value block has already been created
	int maxBuckets = 512 / sizeof(int);//number of buckets each block can hold
	int hashBlock=1, gotoBlock;
	while(hash_value>maxBuckets) {
		hashBlock++;
		hash_value -= maxBuckets;
	}
	if(BF_ReadBlock(header_info.fileDesc,hashBlock,&block)<0) {
 	 	BF_PrintError("Could not read block\n");
 	 	BF_CloseFile(header_info.fileDesc);
  		return -1;
  	}
  	if(block+hash_value*sizeof(int)==NULL) {					//hash value doesnt have a block
  		if(BF_AllocateBlock(header_info.fileDesc)<0) {			//create new block
  			BF_PrintError("Could not allocate block\n");
  			BF_CloseFile(header_info.fileDesc);
  			return -1;
  		}
  		if((gotoBlock=BF_GetBlockCounter(header_info.fileDesc))<0){		//get number of newly created block
			BF_PrintError("Could not get block counter\n");
			BF_CloseFile(header_info.fileDesc);
			return -1;
		}
		memcpy(block+hash_value*sizeof(int),&gotoBlock,sizeof(int));
  		if(BF_ReadBlock(header_info.fileDesc,gotoBlock,&block)<0){
			BF_PrintError("Could not read block\n");
			BF_CloseFile(header_info.fileDesc);
			return -1;
		}
		memcpy(block+2*sizeof(int),&record,sizeof(Record));
		if(BF_WriteBlock(header_info.fileDesc,gotoBlock)<0){
			BF_PrintError("Could not write to block\n");
			BF_CloseFile(header_info.fileDesc);
			return -1;
		}
  	}
  	else {														// open corresponding block to hash value
  		
  	
  	}
    return 0;
}



int HT_GetAllEntries(HT_info header_info, void *value) {
    /* Add your code here */

    return -1;

}


int HashStatistics(char* filename) {
    /* Add your code here */
    
    return -1;
    
}

unsigned int hash_function_int(int hashsize,int num){
	unsigned int hash_value;
	hash_value=num%hashsize;
	return hash_value;
}

unsigned int hash_function_char(int hashsize,char hash_name[25]){
	unsigned long hash_value=0;
	int i;
	while(i=*hash_name++)
		hash_value=i+(hash_value<<6)+(hash_value<<16)-hash_value;
	hash_value=hash_value%hashsize;
	return (unsigned int)hash_value;
}

int main(void){
	HT_info* bla;
	BF_Init();
	HT_CreateIndex( "peos", 'c', "poutsa", 6, 500);
	bla=HT_OpenIndex("peos");
	HT_CloseIndex(bla);
}
