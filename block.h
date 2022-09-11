#ifndef BLOCK_HEADER
#define BLOCK_HEADER
#include <vector>
#include "record.h"
using namespace std;

class Block {
    public:
        vector <Record> records;

        int getSizeofOneRecord() {
            Record record;
            return sizeof(record);
        }

        int getNumberofRecords() {
            return records.size();
        }

        void addRecord(string inputtconst, double inputavgrating, int inputnumvotes) {
            Record record;
            strcpy(record.tconst, inputtconst.c_str());
            record.avgRating = inputavgrating;
            record.numVotes = inputnumvotes;

            records.push_back(record);
        }
};

#endif