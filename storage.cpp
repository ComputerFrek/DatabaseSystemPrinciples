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
        vector<Record*> recordspointers;
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

        //void addRecord(string inputtconst, double inputavgrating, int inputnumvotes);
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

        Record* addRecord(string inputtconst, double inputavgrating, int inputnumvotes){
            Record record;
            strcpy(record.tconst, inputtconst.c_str());
            record.avgRating = inputavgrating;
            record.numVotes = inputnumvotes;

            if((currentblockutilized + sizeof(record)) > blocksize or numallocatedblocks == 0){
                if(!AllocateBlock()){
                    throw "Unable to allocate block";
                }
            }
            
            //tuple<void * , uint> recaddress(blockptr, currentblockutilized);
            //void *rcdAdr = (unsigned char *)get<0>(recaddress) + get<1>(recaddress);
            char* testptr = blockptr + currentblockutilized;
            memcpy(testptr, &record, sizeof(record));
            recordspointers.push_back((Record*) testptr);

            cout << "Storing " << inputtconst << " - " << inputavgrating << " - " << inputnumvotes << " : ";
            printf("%p - %p\n", blockptr, testptr);
            

            currentblockutilized += sizeof(record);
            
            return (Record*) testptr;
        }

        ~Storage(){
            delete storageptr;
            storageptr = nullptr;
        }
};
#endif