#include <iostream>
#include <vector>
#include <math.h>
#include "storage.cpp"
#include "record.h"
using namespace std;

struct BPKeyPtr {
    int key;
    void* dataptr;
};

class BPNode {
    public:
        BPKeyPtr keyptr;
        void* nextnode;
        int curnumkeys;
        bool isleaf;

        BPNode(int maxkeys){
            keyptr.key = 0;
            keyptr.dataptr = nullptr;
            nextnode = nullptr;
            curnumkeys = 0;
            isleaf = false;
        };
};

class BPTree {
    public:
        int maxkeyspernode;
        BPTree(size_t blocksize, Storage *data, Storage *index){
            size_t noderemainingsize = blocksize - sizeof(void*) - sizeof(int) - sizeof(bool);

            maxkeyspernode = 0;
            size_t usedsize = sizeof(BPKeyPtr);
            while((usedsize + sizeof(BPKeyPtr)) <= noderemainingsize){
                usedsize += sizeof(BPKeyPtr);
                maxkeyspernode++;
            }
        }
};