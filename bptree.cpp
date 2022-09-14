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
        bool isroot;
        bool isllnode;

        BPNode(int maxkeys){
            keyptr = new BPKeyPtr[maxkeys];
            for(int i=0; i<maxkeys; i++){
                keyptr[i].dataptr = nullptr;
                keyptr[i].key = 0;
            }
            nextbpnode = nullptr;
            curnumkeys = 0;
            isleaf = false;
            isllnode = false;
        };
};

class BPTree {
    public:
        Storage *data;
        Storage *index;
        BPNode* root;
        void* rootstorageptr;
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
            //If root ptr is null, means no tree yet
            if(root == nullptr){
                LLNode* llnode = nullptr;
                llnode = new LLNode();
                llnode->ptr = recptr;
                llnode->next = nullptr;

                //void* storedptr = nullptr;
                //storedptr = index->writeToDisk(&llnode, sizeof(llnode));
                
                BPNode* node = new BPNode(maxkeyspernode);
                node->keyptr[node->curnumkeys].dataptr = llnode;
                node->keyptr[node->curnumkeys].key = numvotes;
                node->isleaf = true;
                node->curnumkeys++;

                //storedptr = nullptr;
                //storedptr = index->writeToDisk(&node, sizeof(node));
                
                //rootstorageptr = storedptr;
                root = node;
            } else {
                BPNode* cursor = root;
                BPNode* parent;
                void* parentstorageptr;
                void* cursorstorageptr;

                //Keep searching till reach leaf
                while(cursor->isleaf == false){
                    parent = cursor;
                    parentstorageptr = rootstorageptr;

                    //todo
                }

                //Reach leaf, find space to put
                if(cursor->curnumkeys < maxkeyspernode){
                    int i = 0;
                    while(i < cursor->curnumkeys && numvotes > cursor->keyptr[i].key){
                        i++;
                    }

                    if(cursor->keyptr[i].key == numvotes){
                        LLNode* curnode = new LLNode();
                        LLNode* prenode = (LLNode*) cursor->keyptr[i].dataptr;
                        curnode->ptr = recptr;
                        curnode->next = nullptr;

                        while(prenode->next != nullptr){
                            prenode = prenode->next;
                        }

                        prenode->next = curnode;

                        //curnode = (LLNode*) index->loadFromDisk(cursor[i].keyptr->dataptr, sizeof(curnode));
                    } else {
                        //Shift everything back
                        for(int j = cursor->curnumkeys; j > i; j--){
                            cursor->keyptr[j].key = cursor->keyptr[j-1].key;
                            cursor->keyptr[j].dataptr = cursor->keyptr[j-1].dataptr;
                        }

                        //Add the current key in i, add ll to handle duplicates.
                        LLNode* llnode = nullptr;
                        llnode = new LLNode();
                        llnode->ptr = recptr;
                        llnode->next = nullptr;

                        cursor->keyptr[i].dataptr = llnode;
                        cursor->keyptr[i].key = numvotes;
                        cursor->curnumkeys++;
                    }
                } else {
                    //Overflow
                    BPNode* newbpnode = new BPNode(maxkeyspernode);

                    //Create temp list & copy contents of existing
                    BPKeyPtr* templist = new BPKeyPtr[maxkeyspernode + 1];
                    for(int i=0; i < maxkeyspernode; i++){
                        templist[i].key = cursor->keyptr[i].key;
                        templist[i].dataptr = cursor->keyptr[i].dataptr;
                    }

                    //Inserting the new node.
                    //Find new slot in the templist
                    int i = 0;
                    while(i < maxkeyspernode && numvotes > templist[i].key){
                        i++;
                    }

                    //dk
                    if(i < cursor->curnumkeys){
                        if(cursor->keyptr[i].key == numvotes){
                            cout << "Overflow duplicate : " << numvotes << endl;
                            return;
                        }
                    }

                    //Shift all back
                    for(int j = maxkeyspernode; j > i; j--){
                        templist[j].key = templist[j - 1].key;
                        templist[j].dataptr = templist[j - 1].dataptr;
                    }

                    //Create ll to handle duplicates
                    LLNode* llnode = nullptr;
                    llnode = new LLNode();
                    llnode->ptr = recptr;
                    llnode->next = nullptr;

                    //Add current i
                    templist[i].key = numvotes;
                    templist[i].dataptr = llnode;

                    //Calc the number of keys
                    cursor->curnumkeys = (maxkeyspernode + 1) / 2;
                    newbpnode->curnumkeys = (maxkeyspernode + 1) - ((maxkeyspernode + 1) / 2);

                    //Set the next node to the new node
                    cursor->nextbpnode = newbpnode;

                    //Add back the items into bpnodes from templist
                    i = 0;
                    for(i = 0; i < cursor->curnumkeys; i++){
                        cursor->keyptr[i].key = templist[i].key;
                        cursor->keyptr[i].dataptr = templist[i].dataptr;
                    }

                    for(int j = 0; j < newbpnode->curnumkeys; i++, j++){
                        newbpnode->keyptr[j].key = templist[j].key;
                        newbpnode->keyptr[j].dataptr = templist[j].dataptr;
                    }

                    //Previously once full, copy all to temp list & split, need to erase the "suppose" blank space.
                    for(i = cursor->curnumkeys; i < maxkeyspernode; i++){
                        cursor->keyptr[i].key = 0;
                        cursor->keyptr[i].dataptr = nullptr;
                    }
                    
                    //If cursor was root & overflow means splitted & need new root
                    if(cursor == root) {
                        BPNode* newroot = new BPNode(maxkeyspernode);
                        newroot->keyptr[0].dataptr = cursor;
                        newroot->keyptr[0].key = newbpnode->keyptr[0].key;
                        newroot->keyptr[1].dataptr = newbpnode;
                        newroot->isleaf = false;
                        root = newroot;
                    }
                }
            }
        }
};