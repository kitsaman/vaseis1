#include "BF.h"
#include "hash.h"

int HT_CreateIndex( char *fileName, char attrType, char* attrName, int attrLength, int buckets) {
	int blockFile, bucketsN, i, j;
	HT_first* info = malloc(sizeof(HT_first));
	info->fileName = malloc( (strlen(fileName)+1) * sizeof(char) );
	info->attrName = malloc( (strlen(attrName)+1) * sizeof(char) );
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
	printf("Filename=%s\nName=%s\nType=%c\nLength=%d\nBuckets=%d\n",info->fileName,info->attrName,info->attrType,info->attrLength,info->size);
	free(info->fileName);
	free(info->attrName);
	free(info);

	//hashTable construction

	int maxBuckets = 512 / sizeof(int);//number of buckets each block can hold
	int blockSum=1; //number of blocks we'll need to allocate
	int bucketsLeft = buckets; //buckets that still need to fit in a block
	while( bucketsLeft > maxBuckets ){
		blockSum++;
		bucketsLeft -= maxBuckets;
	}
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
}

HT_info* HT_OpenIndex(char *fileName) {
    /* Add your code here */

    return;
    
} 


int HT_CloseIndex( HT_info* header_info ) {
    /* Add your code here */
    
    return -1;
    
}

int HT_InsertEntry(HT_info header_info, Record record) {
    /* Add your code here */
    
    return -1;
    
}



int HT_GetAllEntries(HT_info header_info, void *value) {
    /* Add your code here */

    return -1;

}


int HashStatistics(char* filename) {
    /* Add your code here */
    
    return -1;
    
}

int main(void){
	BF_Init();
	HT_CreateIndex( "peos", 'c', "poutsa", 6, 5);
}
