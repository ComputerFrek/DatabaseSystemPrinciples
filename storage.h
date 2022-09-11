#ifndef STORAGE_HEADER
#define STORAGE_HEADER
#include <vector>
#include <math.h>
#include "block.h"
using namespace std;

class Storage {
    public:
        vector <Block> blocks;
        int numofrecordsperblock;

        Storage(int blocksize) {
            Block block;
            numofrecordsperblock = floor(blocksize/block.getSizeofOneRecord());
            blocks.push_back(block);
        }

        int getNumberOfBlocks() {
            return blocks.size();
        }

        void addRecord(string inputtconst, double inputavgrating, int inputnumvotes) {
            //find available
            int currentlastblock = blocks.size();
            //check if avaible block has space
            if(blocks[blocks.size() - 1].getNumberofRecords() < numofrecordsperblock) {
                blocks[blocks.size() - 1].addRecord(inputtconst, inputavgrating, inputnumvotes);
            } else {
                Block block;
                blocks.push_back(block);
                blocks[blocks.size() - 1].addRecord(inputtconst, inputavgrating, inputnumvotes);
            }
            //if not create new block
        }

        
};
#endif