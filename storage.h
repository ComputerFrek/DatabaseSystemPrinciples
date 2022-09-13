#ifndef STORAGE_HEADER
#define STORAGE_HEADER
#include <iostream>
#include <vector>
#include <math.h>
#include "record.h"
using namespace std;

class Storage {
    private:
        vector <Block> datablocks;
        vector <Record*> recordpointers;
        int numofrecordsperblock;
        unsigned int storagesize;
        unsigned int blocksize;
    public:
        Storage(unsigned int storagesize, unsigned int blocksize);

        //int getNumberOfDataBlocks();

        //void addRecord(string inputtconst, double inputavgrating, int inputnumvotes);
};
#endif