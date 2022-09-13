#ifndef STORAGE_HEADER
#define STORAGE_HEADER
#include <iostream>
#include <vector>
#include <math.h>
#include "record.h"
using namespace std;

class Storage {
    public:
        vector<char*> blockspointers;
        vector<void*> datapointers;
        int numofrecordsperblock;
        size_t storagesize;
        size_t blocksize;
        unsigned int numallocatedblocks;
        size_t currentblockutilized;
        char* storageptr;
        char* blockptr;

        Storage(size_t storagesize, size_t blocksize){
            this->storagesize = storagesize;
            this->blocksize = blocksize;
            this->storageptr = nullptr;
            this->storageptr = new char[storagesize];
            this->blockptr = nullptr;
            this->numallocatedblocks = 0;
            this->currentblockutilized = 0;
        };

        //int getNumberOfDataBlocks();

        bool AllocateBlock(){
            if((numallocatedblocks * blocksize) < storagesize){
                blockptr = storageptr + (numallocatedblocks * blocksize);
                blockspointers.push_back(blockptr);
                numallocatedblocks += 1;
                currentblockutilized = 0;
                return true;
            } else {
                return false;
            }
        }
        
        void* writeToDisk(void* ptr, size_t size){
            if((currentblockutilized + size) > blocksize or numallocatedblocks == 0){
                if(!AllocateBlock()){
                    throw "Unable to allocate block";
                }
            }

            char* destptr = blockptr + currentblockutilized;
            memcpy(destptr, ptr, size);
            datapointers.push_back(destptr);

            currentblockutilized += size;

            return destptr;
        }

        ~Storage(){
            delete storageptr;
            storageptr = nullptr;
        }
};
#endif