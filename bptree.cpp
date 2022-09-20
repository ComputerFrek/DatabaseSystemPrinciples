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
        int s;

        BPTree(size_t blocksize, Storage *data, Storage *index){
            size_t noderemainingsize = blocksize - sizeof(void*) - sizeof(int) - sizeof(bool);

            maxkeyspernode = 0;
            s = 0;
            size_t usedsize = sizeof(BPKeyPtr);
            while((usedsize + sizeof(BPKeyPtr)) <= noderemainingsize){
                usedsize += sizeof(BPKeyPtr);
                maxkeyspernode++;
            }

            maxkeyspernode = 3;

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
                cout << "root: ";
                displaybpnode(root);
            } else {
                BPNode* cursor = root;
                BPNode* parent;
                int parentindex = 0;
                void* parentstorageptr;
                void* cursorstorageptr;

                cout << "root: ";
                displaybpnode(root);

                //Keep searching till reach leaf
                while(cursor->isleaf == false){
                    parent = cursor;
                    //parentstorageptr = rootstorageptr;

                    //Search from left to right of bpnode:
                    for(int i = 0; i < cursor->curnumkeys; i++){
                        //Go left if smaller than key
                        if(numvotes < cursor->keyptr[i].key){
                            cursor = (BPNode*) cursor->keyptr[i].dataptr;
                            parentindex = i;
                            break;
                        }

                        if(i == cursor->curnumkeys - 1){
                            if((i + 1) > maxkeyspernode){
                                cursor = (BPNode*) cursor->nextbpnode;
                            } else {
                                cursor = (BPNode*) cursor->keyptr[i + 1].dataptr;
                            }
                            parentindex = i;
                            break;
                        }
                    }

                    
                    cout << "parent: ";
                    displaybpnode(parent);

                    cout << "child: ";
                    displaybpnode(cursor);
                    
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

                        //Inserting to the start of node, means need update the parent.
                        /*if(i == 0 && parent->isleaf == false){
                            parent->keyptr[parentindex].key = numvotes;
                        }*/
                    }
                    cout << "space to put: ";
                    displaybpnode(cursor);
                } else {
                    //Overflow
                    BPNode* newbpnode = new BPNode(maxkeyspernode);

                    //New node is leaf
                    newbpnode->isleaf = true;

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
                    for(int j = maxkeyspernode + 1; j > i; j--){
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

                    cout << "templist: ";
                    for(int i=0; i < maxkeyspernode + 1; i++){
                        cout << templist[i].dataptr << "|" << templist[i].key << "|";
                    }
                    cout << endl;

                    //Calc the number of keys
                    cursor->curnumkeys = ceil((maxkeyspernode + 1) / 2.0);
                    newbpnode->curnumkeys = floor((maxkeyspernode + 1) / 2.0);

                    //Set the next node to the new node
                    cursor->nextbpnode = newbpnode;
                    
                    //check parent index before & after to attach
                    /*if(parent->keyptr[parentindex - 1].dataptr != nullptr){
                        BPNode* prevnode = parent->keyptr[parentindex - 1].dataptr;
                        prevnode->nextbpnode = cursor;
                    }*/

                    //Add back the items into bpnodes from templist
                    for(i = 0; i < cursor->curnumkeys; i++){
                        cursor->keyptr[i].key = templist[i].key;
                        cursor->keyptr[i].dataptr = templist[i].dataptr;
                    }

                    for(int j = 0; j < newbpnode->curnumkeys; i++, j++){
                        newbpnode->keyptr[j].key = templist[i].key;
                        newbpnode->keyptr[j].dataptr = templist[i].dataptr;
                    }

                    //Previously once full, copy all to temp list & split, need to erase the "suppose" blank space.
                    for(i = cursor->curnumkeys; i < maxkeyspernode; i++){
                        cursor->keyptr[i].key = 0;
                        cursor->keyptr[i].dataptr = nullptr;
                    }

                    cout << cursor << ": ";
                    for(int i=0; i < maxkeyspernode; i++){
                        cout << cursor->keyptr[i].dataptr << "|" << cursor->keyptr[i].key << "|";
                    }
                    cout << cursor->nextbpnode << " -> " << newbpnode << ": ";
                    for(int i=0; i < maxkeyspernode; i++){
                        cout << newbpnode->keyptr[i].dataptr << "|" << newbpnode->keyptr[i].key << "|";
                    }
                    cout << newbpnode->nextbpnode << endl;

                    cout << "cursor: ";
                    displaybpnode(cursor);

                    cout << "newbpnode: ";
                    displaybpnode(newbpnode);

                    
                    //If cursor was root & overflow means splitted & need new root
                    if(cursor == root) {
                        BPNode* newroot = new BPNode(maxkeyspernode);
                        newroot->keyptr[0].dataptr = cursor;
                        newroot->keyptr[0].key = newbpnode->keyptr[0].key;
                        newroot->keyptr[1].dataptr = newbpnode;
                        newroot->isleaf = false;
                        newroot->curnumkeys = 1;
                        root = newroot;
                    } else {
                        //Need new parent in the middle of tree
                        insertinsert(numvotes, parent, newbpnode);
                    }
                    displaybpnode(parent);
                }

            }
        }

        void insertinsert(int numvotes, BPNode* parent, BPNode* child) {
            //If parent got space
            if(parent->curnumkeys < maxkeyspernode){
                //Find location in parent to insert the key
                int i = 0;
                while(i < parent->curnumkeys && numvotes > parent->keyptr[i].key){
                    i++;
                }

                //Location found for key, shifting right to slot in key
                for(int j = parent->curnumkeys; j > i; j--){
                    parent->keyptr[j].key = parent->keyptr[j - 1].key;
                }

                //Shift right to slot in for ptr, if last ptr, put in the nextbpnode
                for(int j = parent->curnumkeys + 1; j > i + 1; j--){
                    if(j == maxkeyspernode + 1){
                        parent->nextbpnode = parent->keyptr[j - 1].dataptr;
                        continue;
                    }
                    parent->keyptr[j].dataptr = parent->keyptr[j - 1].dataptr;
                }

                //Update keys & pointers for new child
                parent->keyptr[i].key = child->keyptr[0].key;
                parent->keyptr[i + 1].dataptr = child;
                parent->curnumkeys++;
            } else {
                //Parent no space
                //Create new parent node
                BPNode* newparent = new BPNode(maxkeyspernode);

                //Create a temp list to store first
                BPKeyPtr* templist = new BPKeyPtr[maxkeyspernode];
                //Lastptr to act as the last ptr cox non leaf can have keys + 1 ptr, this is the +1
                void* lastptr = nullptr;

                //Copy all the keys & ptr
                for(int i = 0; i < maxkeyspernode; i++){
                    templist[i].key = parent->keyptr[i].key;
                    templist[i].dataptr = parent->keyptr[i].dataptr;
                }

                //Search location to insert key & ptr
                int i = 0;
                while(i < maxkeyspernode && numvotes > templist[i].key){
                    i++;
                }

                //Shift right for all those after pos we want to slot in
                for(int z = maxkeyspernode; z > i; z--){
                    templist[z].key = templist[z - 1].key;
                }

                //Shift right for ptr
                for(int z = maxkeyspernode + 1; z > i + 1; z--){
                    if(z == maxkeyspernode + 1){
                        lastptr = templist[z - 1].dataptr;
                        continue;
                    }
                    templist[z].dataptr = templist[z - 1].dataptr;
                }

                //Insert in the new key & ptr
                templist[i].key = numvotes;
                templist[i + 1].dataptr = child;

                //Mark the new parent as not leaf, although the default
                newparent->isleaf = false;

                //Calculate the number of keys we should have based on lect.
                //parent now being the left parent, newparent is the right parent
                parent->curnumkeys = ceil(maxkeyspernode / 2.0);
                newparent->curnumkeys = floor(maxkeyspernode / 2.0);

                //Put back the keys & ptr
                bool putright = false;
                int z = 0;
                for(int i = 0; i < maxkeyspernode; i++){
                    if(putright == false){
                        parent->keyptr[z].key = templist[i].key;
                        parent->keyptr[z].dataptr = templist[i].dataptr;
                    } else {
                        newparent->keyptr[z].key = templist[i].key;
                        newparent->keyptr[z].dataptr = templist[i].dataptr;
                        if(i == maxkeyspernode - 1){
                            newparent->keyptr[z + 1].dataptr = lastptr;
                        }
                    }

                    if(z == parent->curnumkeys){
                        putright = true;
                        z = 0;
                    } else {
                        z++;
                    }
                }

                //Erased unused key & ptr space
                for(int i = parent->curnumkeys; i < maxkeyspernode; i++){
                    parent->keyptr[i].key = 0;
                }

                for(int i = parent->curnumkeys + 1; i < maxkeyspernode; i++){
                    if(i == parent->curnumkeys + 1){
                        parent->nextbpnode = nullptr;
                    }
                    parent->keyptr[i].dataptr = nullptr;
                }

                if(parent == root){
                    BPNode* newroot = new BPNode(maxkeyspernode);
                    newroot->keyptr[0].dataptr = parent;
                    newroot->keyptr[0].key = parent->keyptr[parent->curnumkeys].key;
                    newroot->keyptr[1].dataptr = newparent;
                    newroot->isleaf = false;
                    newroot->curnumkeys = 1;
                    
                    root = newroot;
                } else {
                    //Find parent
                    insertinsert(templist[parent->curnumkeys].key, parent, newparent);
                }
            }

        }

        BPNode* findparent(BPNode* cursor, BPNode* child, int key){
            if(cursor->isleaf){
                return nullptr;
            }

            BPNode* parent = cursor;
            while(cursor->isleaf == false){
                for(int i = 0; i < cursor->curnumkeys + 1; i++){
                    if(cursor->keyptr[i].dataptr == child){
                        return parent;
                    }
                }

                for(int i = 0; i < cursor->curnumkeys; i++){
                    if(key < cursor->keyptr[i].key){
                        parent = (BPNode*) cursor->keyptr[i].dataptr;

                    }
                }
            }
        }

        void displaybpnode(BPNode* curnode){
            cout << curnode->isleaf << "-" << curnode << ": |";
            for(int z = 0; z < maxkeyspernode; z++){
                if(curnode->keyptr[z].dataptr == nullptr){
                    cout << "null|";
                } else {
                    cout << curnode->keyptr[z].dataptr << "|";
                }
                
                if(curnode->keyptr[z].key == 0){
                    cout << "x|";
                } else {
                    cout << curnode->keyptr[z].key << "|";
                }
            }

            if(curnode->nextbpnode == nullptr){
                cout << "null";
            } else {
                cout << curnode->nextbpnode;
            }

            cout << endl;
        }

        void displaytree(BPNode* curnode, int curlevel){
            if(curnode != nullptr){
                cout << " level " << curlevel << ": ";
                for(int i = 0; i < curlevel; i++){
                    cout << "   ";
                }
            }

            displaybpnode(curnode);

            if(curnode->isleaf != true){
                for(int i = 0; i < curnode->curnumkeys + 1; i++){
                    displaytree((BPNode*) curnode->keyptr[i].dataptr, curlevel + 1);
                }
            }
        }

        //Work in progress
        void linkleaf(BPNode* root){
            BPNode* cursor = root;

            if(cursor->isleaf != true){
                for(int i = 0; i < cursor->curnumkeys + 1; i++){
                    linkleaf((BPNode*) cursor->keyptr[i].dataptr);
                }
            }
        }
};