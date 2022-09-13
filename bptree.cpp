#include <iostream>
#include <vector>
#include <math.h>
#include "storage.cpp"
#include "record.h"
using namespace std;

struct BPKeyPtr {
    void* dataptr;
    int key;
};

class LLNode {
    public:
        void* ptr;
        LLNode* next;
};

class BPNode {
    public:
        BPKeyPtr* keyptr;
        void* nextbpnode;
        int curnumkeys;
        bool isleaf;

        BPNode(int maxkeys){
            keyptr = new BPKeyPtr[maxkeys];
            for(int i=0; i<maxkeys; i++){
                keyptr[i].dataptr = nullptr;
                keyptr[i].key = 0;
            }
            nextbpnode = nullptr;
            curnumkeys = 0;
            isleaf = false;
        };
};

class BPTree {
    public:
        Storage *data;
        Storage *index;
        void* root;
        int maxkeyspernode;

        BPTree(size_t blocksize, Storage *data, Storage *index){
            size_t noderemainingsize = blocksize - sizeof(void*) - sizeof(int) - sizeof(bool);

            maxkeyspernode = 0;
            size_t usedsize = sizeof(BPKeyPtr);
            while((usedsize + sizeof(BPKeyPtr)) <= noderemainingsize){
                usedsize += sizeof(BPKeyPtr);
                maxkeyspernode++;
            }

            root = nullptr;

            this->data = data;
            this->index = index;
        }

        void inserttotree(int numvotes, void* recptr){
            if(root == nullptr){
                BPNode* node = new BPNode(maxkeyspernode);

                LLNode* llnode = nullptr;
                llnode = new LLNode();
                llnode->ptr = recptr;
                llnode->next = nullptr;

                node[node->curnumkeys].keyptr->dataptr = llnode;
                node[node->curnumkeys].keyptr->key = numvotes;

                node->curnumkeys++;
                root = node;
            }
        }
};