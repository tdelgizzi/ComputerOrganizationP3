#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>


#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000


enum actionType
{
  cacheToProcessor,
  processorToCache,
  memoryToCache,
  cacheToMemory,
  cacheToNowhere
};

typedef struct blockStruct
{
  int data[MAX_BLOCK_SIZE];
  bool isDirty;
  int lruLabel;
  int set;
  int tag;
  int addr;
} blockStruct;

typedef struct cacheStruct
{
  blockStruct blocks[MAX_CACHE_SIZE];
  int blocksPerSet;
  int blockSize;
  int lru;
  int numSets;
    int blockOffsetBits;
    int setIndexBits;
    int tagBits;
} cacheStruct;

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;




int load(int addr); // Properly simulates the cache for a load from
// memory address “addr”. Returns the loaded value.

void store(int addr, int data); // Properly simulates the cache for a store
// to memory address “addr”. Returns nothing.

int field0(int instruction);
int field1(int instruction);
int field2(int instruction);
int opcode(int instruction);
int convertNum(int num);

int getTag(int  instr);

int getOffset(int instr);

int getSetIndex(int instr);

void printAction(int address, int size, enum actionType type);
void printCache(void);

/* Global Cache variable */
cacheStruct cache;
stateType state;

int
main(int argc, char *argv[])
{
    
    int blockSizeInWords;
    int numberOfSets;
    int blocksPerSet;
    int cacheSize;
    
    char line[MAXLINELENGTH];
    FILE *filePtr;
    
    if (argc != 5) {
        printf("error: usage: %s did not have the correct ammount of args\n", argv[0]);
        exit(1);
    }
    
    blockSizeInWords = atoi(argv[2]);
    numberOfSets = atoi(argv[3]);
    blocksPerSet = atoi(argv[4]);
    cacheSize = blockSizeInWords * numberOfSets * blocksPerSet;
    
    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }
    
    //**********************************
    /* read in the entire machine-code file into memory */
    //**********************************
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
         state.numMemory++) {
        
        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        
        //printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }
    
    //**********************************
    /*set up reg and  PC */
    //**********************************
    for (int i = 0; i < NUMREGS;i++){
        state.reg[i] = 0;
    }
    state.pc = 0;
    
    //**********************************
    /*set up sets and cache memory stuff */
    //**********************************
    
    //typedef struct blockStruct
    // int data[MAX_BLOCK_SIZE]; bool isDirty; int lruLabel; int set; int tag;
    
    //typedef struct cacheStruct
    //blockStruct blocks[MAX_CACHE_SIZE]; int blocksPerSet; int blockSize; int lru; int numSets;
   
    //cacheStruct cache;
    
    
    cache.blocksPerSet  = blocksPerSet;
    cache.blockSize = blockSizeInWords;
    cache.numSets = numberOfSets;
    
    cache.blockOffsetBits = log2(cache.blockSize);
    //cache.blockOffsetBits = (int) log(cache.blockSize)/log(2);
    cache.setIndexBits = log2(cache.numSets);
    //cache.setIndexBits = (int) log(cache.numSets)/log(2);
    cache.tagBits  = 32 - (cache.setIndexBits + cache.blockOffsetBits);

    
    for (int i = 0; i <  MAX_CACHE_SIZE;  i++){
        cache.blocks[i].tag=-1;
        cache.blocks[i].lruLabel = 1000;
        cache.blocks[i].isDirty  = 0;
    }
    
    
    
    
    //**********************************
    /* main loop */
    //**********************************
    bool sim = true;
    int op;
    int f0;
    int f1;
    int f2;
    
    while(sim){
        //printCache();
        int instruction = load(state.pc);
       // printCache();
        op = opcode(instruction);
        f0  = field0(instruction);
        f1 = field1(instruction);
        f2  = field2(instruction);
        
        //add
        if  (op ==  0){
            
            state.reg[f2]  = state.reg[f0] +  state.reg[f1];
            
        }
        
        //nor
        else if  (op ==  1){
            
            state.reg[f2]  = ~(state.reg[f0] | state.reg[f1]);
            
        }
        
        //lw
        else if  (op ==  2){
            
            state.reg[f1] = load(state.reg[f0]  + convertNum(f2));
            
        }
        
        //sw
        else if  (op ==  3){
            
            store(state.reg[f0]  +  convertNum(f2), state.reg[f1]);
        }
        
        //beq
        else if  (op ==  4){
            
            //*****CHANGED THIS FROM f1 == f2 *****
            if (state.reg[f0] == state.reg[f1]){
                state.pc += convertNum(f2);
                //+1 is done  at the end
            }
            
        }
        
        //jalr
        else if  (op ==  5){
            
            state.reg[f1] = state.pc + 1;
            state.pc  = state.reg[f0] - 1;
            
        }
        
        //halt
        else if  (op ==  6){
            
            sim = false;
        }
        
        //noop
        else  if  (op ==  7){
            
            
        }
        
        state.pc++;
        
    }//while
    
    return(0);

}//main







int load(int addr){
    
    int tag = getTag(addr);
    int setIndex = getSetIndex(addr);
    int blockOffset = getOffset(addr);
    
    bool hit  = false;
    int hitIndex = -1;
    
    for (int i = setIndex * cache.blocksPerSet; i <= setIndex * cache.blocksPerSet + cache.blocksPerSet -1; i++){
        if (tag == cache.blocks[i].tag){
            hit = true;
            hitIndex = i;
        }
        
        cache.blocks[i].lruLabel++;
    }
    
    if (hit){
        printAction(addr, 1, cacheToProcessor);
        cache.blocks[hitIndex].lruLabel = 0;
        return (cache.blocks[hitIndex].data[blockOffset]);
    }
    

    else {
        
        int addrNew = addr - blockOffset;
        
        int blockIndex = setIndex * cache.blocksPerSet;
        int currentLRU = cache.blocks[setIndex * cache.blocksPerSet].lruLabel;
        for (int i = setIndex * cache.blocksPerSet;
             i <= setIndex * cache.blocksPerSet + cache.blocksPerSet -1; i++){
            if (cache.blocks[i].lruLabel > currentLRU){
                blockIndex = i;
                currentLRU = cache.blocks[i].lruLabel;
            }
        }//for
        
    
        if (cache.blocks[blockIndex].isDirty == 1){
            printAction(cache.blocks[blockIndex].addr, cache.blockSize, cacheToMemory);
            for (int i = 0; i < cache.blockSize;i++){
                state.mem[cache.blocks[blockIndex].addr + i] = cache.blocks[blockIndex].data[i];
            }
            
            printAction(addrNew, cache.blockSize, memoryToCache);
            for (int i = 0; i < cache.blockSize;i++){
                cache.blocks[blockIndex].data[i] = state.mem[addrNew + i];
            }
            cache.blocks[blockIndex].lruLabel = 0;
            cache.blocks[blockIndex].tag = tag;
            cache.blocks[blockIndex].set = setIndex;
            cache.blocks[blockIndex].addr = addrNew;
            cache.blocks[blockIndex].isDirty = 0;
            
        }//if
        else {
            if (cache.blocks[blockIndex].tag != -1){
            printAction(cache.blocks[blockIndex].addr, cache.blockSize, cacheToNowhere);
            }
            
            //printAction(addr, cache.blockSize, memoryToCache);
            printAction(addrNew, cache.blockSize, memoryToCache);
            
            for (int i = 0; i < cache.blockSize;i++){
                cache.blocks[blockIndex].data[i] = state.mem[addrNew + i];
            }
            cache.blocks[blockIndex].lruLabel = 0;
            cache.blocks[blockIndex].tag = tag;
            cache.blocks[blockIndex].set = setIndex;
            cache.blocks[blockIndex].addr = addrNew;
            cache.blocks[blockIndex].isDirty = 0;
            
            
        }//else
        
        printAction(addr, 1, cacheToProcessor);
        return (cache.blocks[blockIndex].data[blockOffset]);
        
        
    }//eles
    
    
    
}

void store(int addr, int data){
    
    int tag = getTag(addr);
    int setIndex = getSetIndex(addr);
    int blockOffset = getOffset(addr);
    
    bool hit  = false;
    int hitIndex = -1;
    
    for (int i = setIndex * cache.blocksPerSet; i <= setIndex * cache.blocksPerSet + cache.blocksPerSet -1; i++){
        if (tag == cache.blocks[i].tag){
            hit = true;
            hitIndex = i;
            //cache.blocks[i].lruLabel = 0;
        }
        
        cache.blocks[i].lruLabel++;
    }
    
    if (hit){
        printAction(addr, 1, processorToCache);
        cache.blocks[hitIndex].lruLabel = 0;
        cache.blocks[hitIndex].isDirty = 1;
        cache.blocks[hitIndex].data[blockOffset] = data;

    }
    
    
    else {
    
        int addrNew = addr - blockOffset;
        
        int blockIndex = setIndex * cache.blocksPerSet;
        int currentLRU = cache.blocks[setIndex * cache.blocksPerSet].lruLabel;
        for (int i = setIndex * cache.blocksPerSet;
             i <= setIndex * cache.blocksPerSet + cache.blocksPerSet -1; i++){
            if (cache.blocks[i].lruLabel > currentLRU){
                blockIndex = i;
                currentLRU = cache.blocks[i].lruLabel;
            }
        }//for
        
        
        if (cache.blocks[blockIndex].isDirty == 1){
            printAction(cache.blocks[blockIndex].addr, cache.blockSize, cacheToMemory);
            for (int i = 0; i < cache.blockSize;i++){
                state.mem[cache.blocks[blockIndex].addr + i] = cache.blocks[blockIndex].data[i];
            }
            
            printAction(addrNew, cache.blockSize, memoryToCache);
            for (int i = 0; i < cache.blockSize;i++){
                cache.blocks[blockIndex].data[i] = state.mem[addrNew + i];
            }
            cache.blocks[blockIndex].lruLabel = 0;
            cache.blocks[blockIndex].tag = tag;
            cache.blocks[blockIndex].set = setIndex;
            cache.blocks[blockIndex].addr = addrNew;
            cache.blocks[blockIndex].isDirty = 0;
            
        }//if
        else {
            if (cache.blocks[blockIndex].tag != -1){
            printAction(cache.blocks[blockIndex].addr, cache.blockSize, cacheToNowhere);
            }
           // printAction(addr, cache.blockSize, memoryToCache);
            printAction(addrNew, cache.blockSize, memoryToCache);
            
            for (int i = 0; i < cache.blockSize;i++){
                cache.blocks[blockIndex].data[i] = state.mem[addrNew + i];
            }
            cache.blocks[blockIndex].lruLabel = 0;
            cache.blocks[blockIndex].tag = tag;
            cache.blocks[blockIndex].set = setIndex;
            cache.blocks[blockIndex].addr = addrNew;
            cache.blocks[blockIndex].isDirty = 0;
            
            
        }//else
        
        printAction(addr, 1, processorToCache);
        cache.blocks[blockIndex].isDirty = 1;
        cache.blocks[blockIndex].data[blockOffset] = data;
        
        
        
    }
    
    
    
}


int
field0(int instruction)
{
    return( (instruction>>19) & 0x7);
}

int
field1(int instruction)
{
    return( (instruction>>16) & 0x7);
}

int
field2(int instruction)
{
    return(instruction & 0xFFFF);
}

int
opcode(int instruction)
{
    return(instruction>>22);
}

int getTag(int  instr){
    int  temp1  = cache.blockOffsetBits + cache.setIndexBits;
    return (instr >> temp1);
}

int getOffset(int instr){
    //int temp  = (instr) & cache.blockOffsetBits;
    int temp2 = (instr) % cache.blockSize;
    return (temp2);
    
}

int getSetIndex(int instr){
    //return (instr >> cache.blockOffsetBits) & cache.setIndexBits;
    //int temp1 = instr;
   ////int temp2 = cache.blockOffsetBits;
   // int temp3 = instr >> cache.blockOffsetBits;
   // int temp4 = cache.setIndexBits;
  //  int temp5 = temp3 % temp4;
    //return ((instr >> cache.blockOffsetBits) % (cache.setIndexBits +1));
    int temp = (instr >> cache.blockOffsetBits);
    int temp2 = (1 <<cache.setIndexBits) - 1;
    return (temp) & temp2;
}

int
convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Linux integer */
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return(num);
}

/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -	cacheToProcessor: reading data from the cache to the processor
 *  -	processorToCache: writing data from the processor to the cache
 *  -	memoryToCache: reading data from the memory to the cache
 *  -	cacheToMemory: evicting cache data and writing it to the memory
 *  -	cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
	printf("@@@ transferring word [%d-%d] ", address, address + size - 1);

	if (type == cacheToProcessor) {
		printf("from the cache to the processor\n");
	}
	else if (type == processorToCache) {
		printf("from the processor to the cache\n");
	}
	else if (type == memoryToCache) {
		printf("from the memory to the cache\n");
	}
	else if (type == cacheToMemory) {
		printf("from the cache to the memory\n");
	}
	else if (type == cacheToNowhere) {
		printf("from the cache to nowhere\n");
	}
}

/*
 * Prints the cache based on the configurations of the struct
 */
void printCache()
{
  printf("\n@@@\ncache:\n");

  for (int set = 0; set < cache.numSets; ++set) {
    printf("\tset %i:\n", set);
    for (int block = 0; block < cache.blocksPerSet; ++block) {
      printf("\t\t[ %i ]: {", block);
      for (int index = 0; index < cache.blockSize; ++index) {
        printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
      }
      printf(" }\n");
    }
  }

  printf("end cache\n");
}
