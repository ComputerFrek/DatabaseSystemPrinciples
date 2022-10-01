#include <tuple>
#include <iostream>
#include <array>
#include <unordered_map>
#include <cstring>
#include <math.h>

#include "storage.cpp"
#include "types.h"

using namespace std;

// A node in the B+ Tree.
class BPNode{
  public:
    Address* ptrToNode;      // A pointer to an array of struct {void *blockAddress, short int offset} containing other nodes in storage.
    int numOfKeys;           // Total number of keys in the current node
    int* ptrToKeys;          // A node contains number of keys, this parameter pointer to the address of every key
    bool isLeafNode;         // If current node is a leaf node.
    
    BPNode(int _maxNumOfKey){
      // Initialize arrya of pointer to keys and pointer to Node
      ptrToKeys = new int[_maxNumOfKey];
      ptrToNode = new Address[_maxNumOfKey + 1];

      // Initialize every single key to null and offset to 0
      for (int i = 0; i < _maxNumOfKey + 1; i++) {
        Address nullAddress;
        nullAddress.blockAddress = nullptr;
        nullAddress.offset = 0;
        ptrToNode[i] = nullAddress;
        if(i < _maxNumOfKey) {
          ptrToKeys[i] = 0;
        }
      }
      numOfKeys = 0;
    }

    // Get the total number of keys 
    int getCountOfKeys(){
      return numOfKeys;
    }
};

// The B+ Tree itself.
class BPlusTree {
  private:
    // Variables in a B+ tree
    DiskStorage* _storage;           // Pointer to a memory pool for data blocks.
    BPNode* _rootNode;               // Pointer to the main memory _rootNode (if it's loaded).
    Address _rootAddressOfStorage;   // Ppointer to the root address(disk)

    int _maxNumOfKey;                // Maximum keys defined in a node.
    int _numOfNodes;                 // B+ tree total number of nodes 
    int _levelOfTrees;               // B+ tree level 
    size_t _sizeOfNode;              // Node size in B+ tree equivalent to block size in data file , 200kB in this project 

    // Updates the parent node to point at both child nodes, and adds a parent node if needed.
    void addInternalNode(int keyID, Address parentNodeAddress, Address newLeafNodeAddress) {
      // Create a root node and temp ptr which pointing to parent node, to load the copy of data from disk storage
      BPNode* _rootNode = nullptr;
      BPNode* tempPtr = (BPNode*) parentNodeAddress.blockAddress;

      // Check if parent node is root node, if yes then update root node pointing to parent node 
      if (parentNodeAddress.blockAddress == _rootAddressOfStorage.blockAddress) {
        _rootNode = (BPNode*) parentNodeAddress.blockAddress;
      }

      // If parent node stil not full yet, can directly insertRecord new child node and iterate through all keys in the node to sort the keys 
      // and update the pointer pointing to child node 
      if (tempPtr->numOfKeys < _maxNumOfKey) {
        int i = 0;

        // If current key ID larger than parent node key, then move to next key of parent node
        // and use i to store this loaction 
        while (keyID > tempPtr->ptrToKeys[i] && i < tempPtr->numOfKeys) {
          i++;
        }

        // From above step find out the location to insertRecord new keys , and use bubble sort to sort the order 
        for (int j = tempPtr->numOfKeys; j > i; j--) {
          tempPtr->ptrToKeys[j] = tempPtr->ptrToKeys[j - 1];
        }

        // Move parent node pointer one step right pointing  to lower bound of a key 
        for (int j = tempPtr->numOfKeys + 1; j > i + 1; j--) {
          tempPtr->ptrToNode[j] = tempPtr->ptrToNode[j - 1];
        }

        tempPtr->ptrToKeys[i] = keyID;                       // Add in new child and pointing to its upper level parent node
        tempPtr->numOfKeys++;                                // Update parent total number of keys
        tempPtr->ptrToNode[i + 1] = newLeafNodeAddress;      // Next key of newly added child pointing to new lead node 
      } 

      // If parent node no more space for new key , then need to split parent node and insertRecord new parent node 
      // right behind the previous parent node 
      else {

        BPNode* newInternalNode = new BPNode(_maxNumOfKey);   // Create new internal node, since parent node full
        _numOfNodes++;                                    // Update total number of nodes

        // Same logic as above, keep a temp list of ptrToKeys and ptrToNode to insertRecord into the split nodes.
        // Now, we have one extra pointer to keep track of (new child's pointer).

        // Create a temp key list array and address pointers to keep track of newly added nodes
        int tempKeyList[_maxNumOfKey + 1];
        Address tempPointerList[_maxNumOfKey + 2];

       // Copy over all the parent node keys data into temp list array 
        for (int i = 0; i < _maxNumOfKey; i++) {
          tempKeyList[i] = tempPtr->ptrToKeys[i];
        }

        // Copy over all the parent node pointer data into temp list array
        for (int i = 0; i < _maxNumOfKey + 1; i++) {
          tempPointerList[i] = tempPtr->ptrToNode[i];
        }

        // Find out the location to insertRecord new key into temp key list 
        int i = 0;
        while (keyID > tempKeyList[i] && i < _maxNumOfKey) {
          i++;
        }

        // Swap the key which is higher than the location we find out from last step to with its left side key 
        // to make one space for new key 
        for (int j = _maxNumOfKey; j > i; j--) {
          tempKeyList[j] = tempKeyList[j - 1];
        }

        // Move pointer one step left since above keys move back also, to keep track 
        for (int j = _maxNumOfKey + 1; j > i + 1; j--) {
          tempPointerList[j] = tempPointerList[j - 1];
        }

        tempKeyList[i] = keyID;                         // New key has been inserted into temp list 
        tempPointerList[i + 1] = newLeafNodeAddress;    // Pointer address updated
        newInternalNode->isLeafNode = false;            // update new internal node if leaf node 


        // Split node into two new node 
        tempPtr->numOfKeys = ceil((_maxNumOfKey + 1) / 2.0);
        newInternalNode->numOfKeys = floor((_maxNumOfKey + 1) / 2.0) - 1;

        // Update data from temp list array back to parent node 
        for (int i = 0; i < tempPtr->numOfKeys; i++) {
          tempPtr->ptrToKeys[i] = tempKeyList[i];
        }

        for(int i = 0; i < tempPtr->numOfKeys + 1; i++){
          tempPtr->ptrToNode[i] = tempPointerList[i];
        }

        // Update data from temp list array back to newly added internal node 
        int j;
        for (int i = 0, j = tempPtr->numOfKeys + 1; i < newInternalNode->numOfKeys; i++, j++) {
          newInternalNode->ptrToKeys[i] = tempKeyList[j];
        }

        for (int i = 0, j = tempPtr->numOfKeys + 1; i < newInternalNode->numOfKeys + 1; i++, j++) {
          //cout << "Adding to newInternalNode->ptrToNode[" << i << "]: " << static_cast<void*>(tempPointerList[j].blockAddress) + tempPointerList[j].offset << endl;
          newInternalNode->ptrToNode[i] = tempPointerList[j];
        }

        // To handle the situation when the number of parent key larger than max number of key allowed, set extra key value 
        // to 0 and address to nullptr
        for (int i = tempPtr->numOfKeys; i < _maxNumOfKey; i++) {
          tempPtr->ptrToKeys[i] = 0;
        }

        for (int i = tempPtr->numOfKeys + 1; i < _maxNumOfKey + 1; i++) {
          Address nullAddress;
          nullAddress.offset = 0;
          nullAddress.blockAddress = nullptr;
          tempPtr->ptrToNode[i] = nullAddress;
        }

        // Update current internal node pointer address 
        Address newInternalNodeAddress;
        newInternalNodeAddress.blockAddress = newInternalNode;
        newInternalNodeAddress.offset = 0;

        // Check whether parent node and root node is same, if yes then create a new root node
        if (tempPtr == _rootNode) {
          BPNode* newRoot = new BPNode(_maxNumOfKey);
          _numOfNodes++;

          // Link the new root with parent node and new interal node address
          newRoot->ptrToKeys[0] = tempKeyList[tempPtr->numOfKeys];
          newRoot->ptrToNode[0] = parentNodeAddress;
          newRoot->ptrToNode[1] = newInternalNodeAddress;

          // Update new root variables
          newRoot->isLeafNode = false;
          newRoot->numOfKeys = 1;
          _rootNode = newRoot;

          // Update root address 
          _rootAddressOfStorage.blockAddress = _rootNode;
          _rootAddressOfStorage.offset = 0;
        }

        // If parent node not root node 
        else
        {
          // Insert new parent node 
          Address newParentNodeAddress = findParentNode(tempPtr->ptrToKeys[0], _rootAddressOfStorage, parentNodeAddress);
          addInternalNode(tempKeyList[tempPtr->numOfKeys], newParentNodeAddress, newInternalNodeAddress);
          
        }
      }
    }

    void removeInternalNode(int target, BPNode* parentNode, BPNode* childNode) {
      //parent(cursor)
      BPNode* cursor = parentNode;

      if(cursor == _rootAddressOfStorage.blockAddress) {
        if(cursor->numOfKeys == 1){
          if (cursor->ptrToNode[1].blockAddress == childNode){
            Address updateRoot;
            updateRoot.blockAddress = cursor->ptrToNode[0].blockAddress;
            updateRoot.offset = 0;

            _rootNode = (BPNode*) updateRoot.blockAddress;
            _rootAddressOfStorage = updateRoot;
            
            return;
          } else if(cursor->ptrToNode[0].blockAddress == childNode){
            Address updateRoot;
            updateRoot.blockAddress = cursor->ptrToNode[1].blockAddress;
            updateRoot.offset = 0;

            _rootNode = (BPNode*) updateRoot.blockAddress;
            _rootAddressOfStorage = updateRoot;

            return;
          }
        }
      }

      //Parent is not root, find position of target to delete
      int position;
      for(position = 0; position < cursor->numOfKeys; position++) {
        if(cursor->ptrToKeys[position] == target) {
          break;
        }
      }

      //Shift everything infront
      for(int i = position; i < cursor->numOfKeys; i++) {
        cursor->ptrToKeys[i] = cursor->ptrToKeys[i + 1];
      }

      for(position = 0; position < cursor->numOfKeys + 1; position++) {
        if(cursor->ptrToNode[position].blockAddress == childNode) {
          break;
        }
      }

      for(int i = position; i < cursor->numOfKeys + 1; i++) {
        cursor->ptrToNode[i] = cursor->ptrToNode[i + 1];
      }

      cursor->numOfKeys--;

      if(cursor->numOfKeys >= (_maxNumOfKey + 1) / 2 - 1) {
        return;
      }

      if(cursor == _rootNode) {
        return;
      }

      int leftsibling, rightsibling;
      Address parentdiskadd;
      parentdiskadd.blockAddress = parentNode;
      parentdiskadd.offset = 0;

      Address parentparentnodeadd = findParentNode(cursor->ptrToKeys[0], _rootAddressOfStorage, parentdiskadd);
      BPNode* parentparentnode = (BPNode*) parentparentnodeadd.blockAddress;

      for(position = 0; position < parentparentnode->numOfKeys + 1; position++) {
        if(parentparentnode->ptrToNode[position].blockAddress == parentNode) {
          leftsibling = position - 1;
          rightsibling = position + 1;
          break;
        }
      }

      if(leftsibling >= 0){
        BPNode* leftnode = (BPNode*) parentparentnode->ptrToNode[leftsibling].blockAddress;

        if(leftnode->numOfKeys >= (_maxNumOfKey + 1) / 2){
          for(int i = cursor->numOfKeys; i > 0; i--){
            cursor->ptrToKeys[i] = cursor->ptrToKeys[i - 1];
          }

          cursor->ptrToKeys[0] = parentparentnode->ptrToKeys[leftsibling];
          parentparentnode->ptrToKeys[leftsibling] = leftnode->ptrToKeys[leftnode->numOfKeys - 1];

          for(int i = cursor->numOfKeys + 1; i > 0; i--){
            cursor->ptrToNode[i] = cursor->ptrToNode[i - 1];
          }

          cursor->ptrToNode[0] = leftnode->ptrToNode[leftnode->numOfKeys];

          cursor->numOfKeys++;
          leftnode->numOfKeys++;

          leftnode->ptrToNode[cursor->numOfKeys] = leftnode->ptrToNode[cursor->numOfKeys + 1];
          return;
        }
      }

      if(rightsibling <= parentparentnode->numOfKeys){
        BPNode* rightnode = (BPNode*) parentparentnode->ptrToNode[rightsibling].blockAddress;

        if(rightnode->numOfKeys >= (_maxNumOfKey + 1) / 2){
          cursor->ptrToKeys[cursor->numOfKeys] = parentparentnode->ptrToKeys[position];
          parentparentnode->ptrToKeys[position] = rightnode->ptrToKeys[0];

          for(int i = 0; i < rightnode->numOfKeys - 1; i++){
            rightnode->ptrToKeys[i] = rightnode->ptrToKeys[i + 1];
          }
          
          cursor->ptrToNode[cursor->numOfKeys + 1] = rightnode->ptrToNode[0];

          for(int i = 0; i < rightnode->numOfKeys; i++){
            rightnode->ptrToNode[i] = rightnode->ptrToNode[i + 1];
          }

          cursor->numOfKeys++;
          rightnode->numOfKeys--;
          return;
        }
      }

      //No sibiling to steal, merge
      if(leftsibling >= 0){
        BPNode* leftnode = (BPNode*) parentparentnode->ptrToNode[leftsibling].blockAddress;

        leftnode->ptrToKeys[leftnode->numOfKeys] = parentparentnode->ptrToKeys[leftsibling];

        int j;
        for (int i = leftnode->numOfKeys + 1, j = 0; j < cursor->numOfKeys; j++){
          leftnode->ptrToKeys[i] = cursor->ptrToKeys[j];
        }

        Address nulladdress;
        nulladdress.blockAddress = nullptr;
        nulladdress.offset = 0;

        for (int i = leftnode->numOfKeys + 1, j = 0; j < cursor->numOfKeys + 1; j++){
          leftnode->ptrToNode[i] = cursor->ptrToNode[j];
          cursor->ptrToNode[j] = nulladdress;
        }

        leftnode->numOfKeys += cursor->numOfKeys + 1;
        cursor->numOfKeys = 0;

        removeInternalNode(parentparentnode->ptrToKeys[leftsibling], parentparentnode, parentNode);
      } else if(rightsibling <= parentparentnode->numOfKeys){
        BPNode* rightnode = (BPNode*) parentparentnode->ptrToNode[rightsibling].blockAddress;

        cursor->ptrToKeys[cursor->numOfKeys] = parentparentnode->ptrToKeys[rightsibling - 1];

        for(int i = cursor->numOfKeys + 1, j = 0; j < rightnode->numOfKeys; j++){
          cursor->ptrToKeys[i] = rightnode->ptrToKeys[j];
        }

        Address nulladdress;
        nulladdress.blockAddress = nullptr;
        nulladdress.offset = 0;

        for(int i = cursor->numOfKeys + 1, j = 0; j < rightnode->numOfKeys + 1; j++){
          cursor->ptrToNode[i] = rightnode->ptrToNode[j];
          rightnode->ptrToNode[j] = nulladdress;
        }

        cursor->numOfKeys += rightnode->numOfKeys + 1;
        rightnode->numOfKeys = 0;

        removeInternalNode(parentparentnode->ptrToKeys[rightsibling - 1], parentNode, rightnode);
      }
    }

    // Find the parent address of a node by pass in parameter,  lower bound key and child node address, root storage address
    Address findParentNode(int lowerBoundKey, Address _rootAddressOfStorage, Address childNodeAddress){
      // Load parent from disk first 
      BPNode* tempPtr = (BPNode*) _rootAddressOfStorage.blockAddress;
      BPNode* parent = (BPNode*) tempPtr;

      Address nullAddress;
      nullAddress.blockAddress = nullptr;
      nullAddress.offset = 0;

      // If parent node is a leaf ndoe ..obvisouly no child node 
      if (tempPtr->isLeafNode == true) {
        return nullAddress;
      }

      // If not leaf node, then iterate all parent key and finde out the location
      while (tempPtr->isLeafNode == false) {
        for (int i = 0; i < tempPtr->numOfKeys + 1; i++) {
          if (tempPtr->ptrToNode[i].blockAddress == childNodeAddress.blockAddress) {
            Address parentNodeAddress;
            parentNodeAddress.blockAddress = parent;
            parentNodeAddress.offset = 0;

            return parentNodeAddress;
          }
        }

        // If unable find from above iteration, then need go down next tree level 
        for (int i = 0; i < tempPtr->numOfKeys; i++) {

          // If key is lesser, then check left side 
          if (lowerBoundKey < tempPtr->ptrToKeys[i]) {
            parent = tempPtr;
            tempPtr = (BPNode*) tempPtr->ptrToNode[i].blockAddress;
            break;
          }

          // If key is larger than current parent node pointer, then check right side 
          if (i == tempPtr->numOfKeys - 1) {
            parent = tempPtr;
            tempPtr = (BPNode*) tempPtr->ptrToNode[i + 1].blockAddress;
            break;
          }
        }
      }

      // If above still can't find, return null address
      return nullAddress;
    }

  public:
    // Constructor
    BPlusTree(DiskStorage* _storage, size_t sizeOfBlock){
      // Check the left over size in a node to store pointers and keys
      size_t avaliableNodeSize = sizeOfBlock - sizeof(bool) - sizeof(int);

      // Size of Address struct 
      size_t sizeOfAddress = sizeof(Address);
      _maxNumOfKey = 0;

      // To check how many keys can fit in one node and update max number of key in a node 
      while (sizeOfAddress + sizeof(Address) + sizeof(int) <= avaliableNodeSize) {
        sizeOfAddress += (sizeof(Address) + sizeof(int));
        _maxNumOfKey += 1;
      }

      if (_maxNumOfKey == 0) {
        throw overflow_error("Error: Size of pointer and keys exceeds limit!");
      }

      // Initialize _rootNode
      _rootAddressOfStorage.blockAddress = nullptr;
      _rootAddressOfStorage.offset = 0;
      _rootNode = nullptr;

      // Set node size to be equal to block size.
      _sizeOfNode = sizeOfBlock;

      // Initialize variables
      _levelOfTrees = 0;
      _numOfNodes = 0;

      this->_storage = _storage;
    }

    // Inserts new record into B+ tree
    void insertRecord(Address recordaddress, int key){
      BPNode* _rootNode = (BPNode*) _rootAddressOfStorage.blockAddress;

      // Check if have root node, if no create a new root node 
      if (_rootNode == nullptr) {
        
        // Create a new linked list reserved for each key, to handlie duplicate
        // For example, some movie may have same number of votes, so record may
        // have more than one record with same number of notes 
        LinkedListNode* llnode = new LinkedListNode();
        llnode->dataaddress.blockAddress = recordaddress.blockAddress;
        llnode->dataaddress.offset = recordaddress.offset;
        llnode->next = nullptr;

        Address linkedListNodeAdress;
        linkedListNodeAdress.blockAddress = llnode;
        linkedListNodeAdress.offset = 0;

        // Create new root node 
        _rootNode = new BPNode(_maxNumOfKey);
        _numOfNodes++;
        _rootNode->ptrToKeys[0] = key;
        _rootNode->numOfKeys = 1;
        _rootNode->isLeafNode = true;            // It is root node and leaf node also
        _rootNode->ptrToNode[0] = linkedListNodeAdress; 

        _rootAddressOfStorage.blockAddress = _rootNode;
        _rootAddressOfStorage.offset = 0;
      } else { 
        
        // If already have a root node, then find a location to insert the new record 
        BPNode* tempPtr = _rootNode;
        
        // Create a parent node to keep track during searchKey the location for new node 
        BPNode* parent = nullptr;                          
        Address parentDiskAddress;                         
        parentDiskAddress.blockAddress = nullptr;
        parentDiskAddress.offset = 0;

        // Assign the address of tempPtr pointing to root node 
        Address tempPtrAddress; // Store current node's _storage address in case we need to update it in _storage.
        tempPtrAddress.blockAddress = _rootNode;
        tempPtrAddress.offset = 0;


        int treeLevel = 0;

        // Loop through all non-leaf nodes and keys to find out location which to insert the new record
        while (tempPtr->isLeafNode == false) {

          parent = tempPtr;
          parentDiskAddress.blockAddress = parent;

          for (int i = 0; i < tempPtr->numOfKeys; i++) {
            
            // if searchKey key is lesser than the current key we check, then check the life side of tree
            if (key < tempPtr->ptrToKeys[i]) {
              tempPtr = (BPNode*) tempPtr->ptrToNode[i].blockAddress;
              tempPtrAddress.blockAddress = tempPtr;
              break;
            }

            // Else check right side of the tree
            if (i == tempPtr->numOfKeys - 1) { 
              tempPtr = (BPNode*) tempPtr->ptrToNode[i + 1].blockAddress;
              tempPtrAddress.blockAddress = tempPtr;
              break;
            }
          }

          treeLevel++;
        }

        // Out of while loop means already reach a leaf node, to put in new record here if space avaliable
        if (tempPtr->numOfKeys < _maxNumOfKey) {
          int i = 0;
      
        // Loop through all keys to find a place put in the key for new record 
          while (key > tempPtr->ptrToKeys[i] && i < tempPtr->numOfKeys) { 
            i++;
          }

          // Check for duplicate, whether already a key exists , if exist same key, create a linked list to hold for duplicates
          if (tempPtr->ptrToKeys[i] == key) {
            Address linkedListNodeAdress;
            linkedListNodeAdress.blockAddress = tempPtr->ptrToNode[i].blockAddress;
            linkedListNodeAdress.offset = tempPtr->ptrToNode[i].offset;
            
            LinkedListNode* prenode = (LinkedListNode*) linkedListNodeAdress.blockAddress + linkedListNodeAdress.offset;
            LinkedListNode* curnode = new LinkedListNode();
            curnode->dataaddress.blockAddress = recordaddress.blockAddress;
            curnode->dataaddress.offset = recordaddress.offset;
            curnode->next = nullptr;
            
            while(prenode->next != nullptr){
              prenode = prenode->next;
            }

            prenode->next = curnode;
          } else {
            // Update pointer location, to keep track the last pointer of key in a node, 1st key pointer for new node 
            Address next = tempPtr->ptrToNode[tempPtr->numOfKeys];

            // In this case i represents the key id we found to insert new record
            //, and iterate through all keys and sort them to let the new key fit in 
            for (int j = tempPtr->numOfKeys; j > i; j--) {
              tempPtr->ptrToKeys[j] = tempPtr->ptrToKeys[j - 1];
              tempPtr->ptrToNode[j] = tempPtr->ptrToNode[j - 1];
            }

            // Location has been determined, and insert new key here
            tempPtr->ptrToKeys[i] = key;

            // Create a new linked list to store record, put all duplicates into the list with same key value 
            LinkedListNode* llnode = new LinkedListNode();
            llnode->dataaddress.blockAddress = recordaddress.blockAddress;
            llnode->dataaddress.offset = recordaddress.offset;
            llnode->next = nullptr;
            
            Address linkedListNodeAdress;
            linkedListNodeAdress.blockAddress = llnode;
            linkedListNodeAdress.offset = 0;
            
            // Update current pointer variables 
            tempPtr->ptrToNode[i] = linkedListNodeAdress;
            tempPtr->numOfKeys++;
            tempPtr->ptrToNode[tempPtr->numOfKeys] = next;

          }
        } 
        else { 
          
          // Overflow: If there's no space to insertRecord new key, we have to split this node into two and update the parent if required.
          // Create a new leaf node to put half the ptrToKeys and ptrToNode in.

          // If no more space to store new record in current node, then split into two node and update the pointers in leaf and parent node  
          BPNode* newLeafNode = new BPNode(_maxNumOfKey);
          _numOfNodes++;

          // create temp list array to store key and pointers seperately, copy over current node data into it 
          int tempKeyList[_maxNumOfKey + 1];
          Address tempPointerList[_maxNumOfKey + 1];
          Address next = tempPtr->ptrToNode[tempPtr->numOfKeys];

          for (int i = 0; i < _maxNumOfKey; i++) {
            tempKeyList[i] = tempPtr->ptrToKeys[i];
            tempPointerList[i] = tempPtr->ptrToNode[i];
          }

          // Add the new record into temp list array by iterating current node keys to find proper location
          int i = 0;
          while (key > tempKeyList[i] && i < _maxNumOfKey) {
            i++;
          }

          if (i < tempPtr->numOfKeys) {
            if (tempPtr->ptrToKeys[i] == key) {
              Address linkedListNodeAdress;
              linkedListNodeAdress.blockAddress = tempPtr->ptrToNode[i].blockAddress;
              linkedListNodeAdress.offset = tempPtr->ptrToNode[i].offset;

              LinkedListNode* prenode = (LinkedListNode*) linkedListNodeAdress.blockAddress + linkedListNodeAdress.offset;
              LinkedListNode* curnode = new LinkedListNode();
              curnode->dataaddress.blockAddress = recordaddress.blockAddress;
              curnode->dataaddress.offset = recordaddress.offset;
              curnode->next = nullptr;

              while(prenode->next != nullptr){
                prenode = prenode->next;
              }

              prenode->next = curnode;
              return;
            } 
          }

          // Use bubble sort to sort keys 
          for (int j = _maxNumOfKey; j > i; j--) {
            tempKeyList[j] = tempKeyList[j - 1];
            tempPointerList[j] = tempPointerList[j - 1];
          }

          // Add new record key and pointer  into the temporary lists.
          tempKeyList[i] = key;

          //Create linked list to handle duplicate key situation 
          LinkedListNode* llnode = new LinkedListNode();
          llnode->dataaddress.blockAddress = recordaddress.blockAddress;
          llnode->dataaddress.offset = recordaddress.offset;
          llnode->next = nullptr;
          
          Address linkedListNodeAdress;
          linkedListNodeAdress.blockAddress = llnode;
          linkedListNodeAdress.offset = 0;
          
          // Store the data into temp list
          tempPointerList[i] = linkedListNodeAdress;
          newLeafNode->isLeafNode = true;                          
          tempPtr->numOfKeys = ceil((_maxNumOfKey + 1) / 2.0);
          newLeafNode->numOfKeys = floor((_maxNumOfKey + 1) / 2.0);
          newLeafNode->ptrToNode[newLeafNode->numOfKeys] = next;
          
          for (i = 0; i < tempPtr->numOfKeys; i++) {
            tempPtr->ptrToKeys[i] = tempKeyList[i];
            tempPtr->ptrToNode[i] = tempPointerList[i];
          }

          // Update keys and pointers for the new leaf node 
          for (int j = 0; j < newLeafNode->numOfKeys; i++, j++) {
            newLeafNode->ptrToKeys[j] = tempKeyList[i];
            newLeafNode->ptrToNode[j] = tempPointerList[i];
          }

          // Add new leaf node address into disk
          Address newLeafNodeAddress;
          newLeafNodeAddress.blockAddress = newLeafNode;
          newLeafNodeAddress.offset = 0;
          
          // Set the current pointer cursor pointing to new leaf node, so later can save new leaf node into disk
          tempPtr->ptrToNode[tempPtr->numOfKeys] = newLeafNodeAddress;

          // Check if any wrong key and pointer , set it to 0 and null 
          for (int i = tempPtr->numOfKeys; i < _maxNumOfKey; i++) {
            tempPtr->ptrToKeys[i] = 0;
          }

          for (int i = tempPtr->numOfKeys + 1; i < _maxNumOfKey + 1; i++) {
            Address nullAddress;
            nullAddress.blockAddress = nullptr;
            nullAddress.offset = 0;
            tempPtr->ptrToNode[i] = nullAddress;
          }

          // If current pointer cursor at root node, then make this cursor into a root node 
          if (tempPtr == _rootNode) {
            BPNode* newRoot = new BPNode(_maxNumOfKey);
            _numOfNodes++;
            newRoot->ptrToKeys[0] = newLeafNode->ptrToKeys[0];
            newRoot->ptrToNode[0] = tempPtrAddress;
            newRoot->ptrToNode[1] = newLeafNodeAddress;
            newRoot->isLeafNode = false;
            newRoot->numOfKeys = 1;
            _rootNode = newRoot;
            _rootAddressOfStorage.blockAddress = _rootNode;
            _rootAddressOfStorage.offset = 0;
          } else { // If we are not at the _rootNode, we need to insertRecord a new parent in the middle _levelOfTrees of the tree.
            addInternalNode(newLeafNode->ptrToKeys[0], parentDiskAddress, newLeafNodeAddress);
          }
        }
      }
    }

    int removeRecord(int target){
      BPNode* cursor = (BPNode*) _rootAddressOfStorage.blockAddress; //current target in B+Tree
      BPNode* parent; // Keep track of the parent as we go deeper into the tree in case we need to update it.

      cout << "rootStorageAddress: " << _rootAddressOfStorage.blockAddress << endl;

      int leftSibling, rightSibling; // Index of left and right child to borrow from.
      int deletedNodesCount; //Count of node is deleted or two nodes are merged
      int updatedNodesCount; //number nodes of the updated B+ tree;
      int heightOfBPlusTree; //height of the updated B+ tree

      if (cursor != nullptr){
        while(!cursor->isLeafNode){
          parent = cursor; // Set the parent of the node

          //cout << "line2046: " << cursor << endl;
          //cout << "line2046: " << cursor->numKeys << endl;
          for (int i = 0; i < cursor->numOfKeys; i++){
            cout << "cursor->pointers[ " << i << " ]: " << cursor->ptrToNode[i].blockAddress << endl;

            leftSibling = i - 1;
            rightSibling = i + 1;

            cout << "leftSibling: " << leftSibling << endl;
            cout << "rightSibling: " << rightSibling << endl;

            int key = getKeyAddress(cursor,i);

            // If key is lesser than current key, go to the left pointer's node.
            if (target < key) {
                cursor = (BPNode*) cursor->ptrToNode[i].blockAddress;
                break;
            }

            // Else if key larger than all keys in the node, go to last pointer's node (rightmost).
            if(cursor->getCountOfKeys() - 1 == i) {
              leftSibling = i;
              rightSibling = i + 2;

              //to-fix: need to update cursor with Next Node
              cursor = (BPNode*) cursor->ptrToNode[i + 1].blockAddress;
              break;
            }
          }
        }

        //Reach leaf node
        bool foundkey = false;
        int pos;  //The position of the target key if found

        cout << "numKeys: " << cursor->numOfKeys << endl;
        for (pos = 0; pos < cursor->numOfKeys; pos++) {
          cout << "[ " << pos << " ] = " << cursor->ptrToKeys[pos] << endl;

          if (getKeyAddress(cursor,pos) == target) {
            foundkey = true;
            break;
          }
        }

        if (!foundkey){
          cout << "The end: No key found" << endl;

          //todo: need to return numNodesDeleted after deletion
          return 0; 
        }

        deleteTargetKeyFromNode(cursor, pos);

        movePointersForward(cursor, _maxNumOfKey);

        // If current node is root, check if tree still has keys.
        if (cursor == _rootNode && cursor->numOfKeys == 0) {
           // Reset root pointers in the B+ Tree.
          _rootNode = nullptr;
          Address nulladd;
          nulladd.blockAddress = nullptr;
          nulladd.offset = 0;

          _rootAddressOfStorage = nulladd;

          //todo: need to return numNodesDeleted after deletion
          return 999; 
        }

        bool hasUnderflow = checkHasUnderflow(cursor,_maxNumOfKey);
        if(hasUnderflow){
          //Try to lend from Left Sibling
          if (leftSibling >= 0) {
            int numNodesDeleted = borrowFromLeftSibling(cursor,parent,leftSibling,_maxNumOfKey);

            if(numNodesDeleted > 0){
              return numNodesDeleted;
            }
          }

          //Try to lend from Right Sibling
          if (rightSibling <= parent->numOfKeys) {
            int numNodesDeleted = borrowFromRightSibling(cursor,parent,rightSibling,_maxNumOfKey);

            if(numNodesDeleted > 0){
              return numNodesDeleted;
            }
          }
          
          //No Left/Right Sibling to borrow, thus we do Merge Nodes to resolve Underflow
          // If left sibling exists, merge with it.
          if (leftSibling >= 0){
            cout << "mergeWithLeftSibling" << endl;
            mergeWithLeftSibling(cursor,parent,leftSibling);
          } else if (rightSibling <= parent->numOfKeys){
            cout << "mergeWithRightSibling" << endl;
            mergeWithRightSibling(cursor,parent,rightSibling);
          }
        } else { //No hasUnderflow
          //todo: need to return numNodesDeleted after deletion
          return 999; 
        } 
      }
      return 999;
    }

    // Method used to search a key 
    tuple<int,int> searchKey(int lowerBoundKey, int upperBoundKey) {
      // Initialize variables
      BPNode* parentNode = (BPNode*) _rootAddressOfStorage.blockAddress; 
      int displayCount = 0;
      int indexOfBlock = 0;
      int recordIDOfBlock = 0;
      int numOfRecordFound = 0;
      float avgRating = 0.0;

      if (parentNode != nullptr) {

        // Loop through non-leaf node keys 
        while(!parentNode->isLeafNode) {
          if(displayCount < 5) {
            cout << "Index block: ";
            showNodeContent(parentNode);
            displayCount++;
          }

          for (int i = 0; i < parentNode->numOfKeys; i++){
            int key = getKeyAddress(parentNode, i);
            if (lowerBoundKey < key){
                parentNode = (BPNode*) parentNode->ptrToNode[i].blockAddress;
                break;
            }

            // If reach last key, move to right node continue search 
            if(parentNode->getCountOfKeys() - 1 == i){
              parentNode = (BPNode*) parentNode->ptrToNode[i + 1].blockAddress;
              break;
            }
          }
          indexOfBlock++;
        }

        // Loop through leaf node keys to match with target and showBPlusTree result 
        displayCount = 0;
      
        bool continueSearch = false;
        while(!continueSearch){
          if(displayCount < 5) {
            cout << "Current index block is : ";
            showNodeContent(parentNode);
            displayCount++;
          }

          int i;
          for (i = 0; i < parentNode->getCountOfKeys(); i++){
            int key = getKeyAddress(parentNode,i);
            if (key > upperBoundKey) {
              continueSearch = true;
              cout << "Average of rating: " << (avgRating / numOfRecordFound) << endl;
              cout << "Searching completed."<< endl;
              break;
            }
            
            if (key >= lowerBoundKey && key <= upperBoundKey){
              float averageRating = 0.0;
              int numOfResultFound = 0;

              Address linkedListNodeAdress;
              linkedListNodeAdress.blockAddress = parentNode->ptrToNode[i].blockAddress;
              linkedListNodeAdress.offset = parentNode->ptrToNode[i].offset;

              tie(numOfResultFound, averageRating) = showLinkedList(linkedListNodeAdress);
              avgRating += averageRating;
              numOfRecordFound += numOfResultFound;
              recordIDOfBlock += numOfResultFound;
            }
          }

          if(parentNode->ptrToNode[parentNode->numOfKeys].blockAddress != nullptr && parentNode->ptrToKeys[i] != upperBoundKey){
            parentNode = (BPNode*) parentNode->ptrToNode[parentNode->numOfKeys].blockAddress;
            indexOfBlock++;
          } else {
            continueSearch = true;
          }
        }
      } 
      else 
      {
        cout << "No result found!" << endl;
      }
      return make_tuple(indexOfBlock, recordIDOfBlock);
    }

    // To display current B+ tree in console
    void showBPlusTree(Address _rootAddressOfStorage, int treeLevel) {
      BPNode* cursorPointer = (BPNode*) _rootAddressOfStorage.blockAddress;

      // If tree is not empty 
      if (cursorPointer != nullptr) {
        cout << "Current B+ tree Level is:  " << treeLevel;
        for (int i = 0; i < treeLevel; i++) { cout << "  "; }

        showNodeContent(cursorPointer);

        if (cursorPointer->isLeafNode != true) {
          for (int i = 0; i < cursorPointer->numOfKeys + 1; i++) {
            
            // Store data to disk
            Address cursorStorageAddress;
            cursorStorageAddress.blockAddress = cursorPointer->ptrToNode[i].blockAddress;
            cursorStorageAddress.offset = 0;
            showBPlusTree(cursorStorageAddress, treeLevel + 1);
          }
        }
      }
    }

    // Display a specific node from B+ tree and display all the content of this node 
    void showNodeContent(BPNode* node) {
      // Display contents in the node with this format  |pointer|key|pointer|
      int i = 0;
      cout << "  " << node << " - |";
      for (int i = 0; i < node->numOfKeys; i++) {
        if(node->isLeafNode == true){
          int count = 0;

          Address linkedListNodeAdress;
          linkedListNodeAdress.blockAddress = node->ptrToNode[i].blockAddress;
          linkedListNodeAdress.offset = node->ptrToNode[i].offset;

          LinkedListNode* prenode = (LinkedListNode*) linkedListNodeAdress.blockAddress + linkedListNodeAdress.offset;
          count++;
          while(prenode->next != nullptr){
            count++;
            prenode = prenode->next;
          }

          cout << static_cast<void*>(node->ptrToNode[i].blockAddress) << "|";
          cout << node->ptrToKeys[i] << "-" << count << "|";
        } else {
          cout << static_cast<void*>(node->ptrToNode[i].blockAddress) << "|";
          cout << node->ptrToKeys[i] << "|";
        }
      }

      // Display last remaining pointer 
      if (node->ptrToNode[node->numOfKeys].blockAddress == nullptr) {
        cout << "null|";
      } else {
        cout << node->ptrToNode[node->numOfKeys].blockAddress << "|";
      }

      for (int i = node->numOfKeys; i < _maxNumOfKey; i++) {
        cout << "x|";      // Remaining empty ptrToKeys
        cout << "null|";   // Remaining empty ptrToNode
      }
      cout << endl;
    }

    tuple<int, float> showLinkedList(Address LLHeadAddress){
      float avgRating = 0.0;
      int numOfRecords = 0;
      LinkedListNode* parentNode = (LinkedListNode*) LLHeadAddress.blockAddress + LLHeadAddress.offset;

      while(parentNode->next != nullptr){
        Address recordAddress;
        recordAddress.blockAddress = parentNode->dataaddress.blockAddress;
        recordAddress.offset = parentNode->dataaddress.offset;

        Record* currecord = (Record*) _storage->retrieveDataFromDisk(recordAddress, sizeof(Record));
        avgRating += currecord->avgRating;
        numOfRecords++;
        cout << currecord->tconst << "  " << currecord->avgRating << "  " << currecord->numVotes << endl;
        parentNode = parentNode->next;
      }

      return make_tuple(numOfRecords, avgRating);
    }

    int getMaxNumOfKeys() { return _maxNumOfKey; }

    int getTotalNumOfNode() { return _numOfNodes; }

    int getBPTreeLevel(Address _rootAddressOfStorage, int treeLevel) {
      BPNode* parentNode = (BPNode*) _rootAddressOfStorage.blockAddress;

      // If tree not empty, display all node
      if (parentNode != nullptr) {
        if (parentNode->isLeafNode != true) {
          for (int i = 0; i < parentNode->numOfKeys + 1; i++) {
            Address cursorStorageAddress;
            cursorStorageAddress.blockAddress = parentNode->ptrToNode[i].blockAddress;
            cursorStorageAddress.offset = 0;

            return getBPTreeLevel(cursorStorageAddress, treeLevel + 1);
          }
        } else {
          return treeLevel;
        }
      }
      return 0;
    }

    Address getDiskRootAddress() { return _rootAddressOfStorage; };

    int getKeyAddress(BPNode* node, int i) { return node->ptrToKeys[i]; }

    void deleteTargetKeyFromNode(BPNode* cursor, int pos){
      for (int i = pos; i < cursor->numOfKeys; i++) {
        cursor->ptrToKeys[i] = cursor->ptrToKeys[i + 1];
        cursor->ptrToNode[i] = cursor->ptrToNode[i + 1];
      }
      cursor->numOfKeys--; //update numKeys (minus 1 key)
    }

    void movePointersForward(BPNode* cursor, int maxKeys){
        // Move the last pointer forward (if any).
        cursor->ptrToNode[cursor->numOfKeys] = cursor->ptrToNode[cursor->numOfKeys + 1];

        // Set all forward pointers from numKeys onwards to nullptr.
        for (int i = cursor->numOfKeys + 1; i < maxKeys + 1; i++) {
          Address nullAddress;
          nullAddress.blockAddress = nullptr;
          nullAddress.offset = 0;
          cursor->ptrToNode[i] = nullAddress;
        }
    }

    bool checkHasUnderflow(BPNode* cursor, int maxKeys){
      if (cursor->numOfKeys >= (maxKeys + 1) / 2){
        cout << "Underflow: false" << endl;
        return false;
      }
      cout << "Underflow: true" << endl;
      return true;
    }

    int borrowFromLeftSibling(BPNode* cursor, BPNode* parent, int leftSibling, int maxKeys){
      BPNode* leftNode = (BPNode*) parent->ptrToNode[leftSibling].blockAddress;

      if (leftNode->numOfKeys >= (maxKeys + 1) / 2 + 1) {
        // Shift last pointer back by one first.
        //cout << "cursor->pointers[" << cursor->numKeys + 1 << "] was : " << static_cast<void*>(cursor->pointers[cursor->numKeys + 1].blockAddress) + cursor->pointers[cursor->numKeys + 1].offset << endl;
        cursor->ptrToNode[cursor->numOfKeys + 1] = cursor->ptrToNode[cursor->numOfKeys];
        //cout << "cursor->pointers[" << cursor->numKeys + 1 << "] : " << static_cast<void*>(cursor->pointers[cursor->numKeys + 1].blockAddress) + cursor->pointers[cursor->numKeys + 1].offset << endl;
        //displayNode(cursor);

        // Shift all remaining keys and pointers back by one.
        for (int i = cursor->numOfKeys; i > 0; i--) {
          //cout << "cursor->keys[" << i << "] : " << cursor->keys[i] << " < " << cursor->keys[i - 1] << endl;
          //cout << "cursor->pointers[" << i << "] : " << static_cast<void*>(cursor->pointers[i].blockAddress) << " < " << static_cast<void*>(cursor->pointers[i - 1].blockAddress) << endl;
          cursor->ptrToKeys[i] = cursor->ptrToKeys[i - 1];
          cursor->ptrToNode[i] = cursor->ptrToNode[i - 1];
        }

        // Transfer borrowed key and pointer (rightmost of left node) over to current node.
        cursor->ptrToKeys[0] = leftNode->ptrToKeys[leftNode->numOfKeys - 1];
        cursor->ptrToNode[0] = leftNode->ptrToNode[leftNode->numOfKeys - 1];
        //cout << "cursor: ";
        //displayNode(cursor);

        cursor->numOfKeys++;
        leftNode->numOfKeys--;

        // Update left sibling (shift pointers left)
        leftNode->ptrToNode[cursor->numOfKeys + 1].blockAddress = cursor;
        leftNode->ptrToNode[cursor->numOfKeys + 1].offset = 0;

        // Update parent node's key
        parent->ptrToKeys[leftSibling] = cursor->ptrToKeys[0];

        //todo: need to return numNodesDeleted after deletion
        return 999;  
      }

      return 0;
    }

    int borrowFromRightSibling(BPNode* cursor, BPNode* parent, int rightSibling, int maxKeys){
      BPNode* rightNode = (BPNode*) parent->ptrToNode[rightSibling].blockAddress;

      if (rightNode->numOfKeys >= (maxKeys + 1) / 2 + 1){
        // Shift last pointer back by one first.
        cursor->ptrToNode[cursor->numOfKeys + 1] = cursor->ptrToNode[cursor->numOfKeys];

        // No need to shift remaining pointers and keys since we are inserting on the rightmost.
        // Transfer borrowed key and pointer (leftmost of right node) over to rightmost of current node.
        cursor->ptrToKeys[cursor->numOfKeys] = rightNode->ptrToKeys[0];
        cursor->ptrToNode[cursor->numOfKeys] = rightNode->ptrToNode[0];
        cursor->numOfKeys++;
        rightNode->numOfKeys--;

        // Update right sibling (shift keys and pointers left)
        for (int i = 0; i < rightNode->numOfKeys; i++) {
          rightNode->ptrToKeys[i] = rightNode->ptrToKeys[i + 1];
          rightNode->ptrToNode[i] = rightNode->ptrToNode[i + 1];
        }

        // Move right sibling's last pointer left by one too.
        rightNode->ptrToNode[cursor->numOfKeys] = rightNode->ptrToNode[cursor->numOfKeys + 1];

        // Update parent node's key to be new lower bound of right sibling.
        parent->ptrToKeys[rightSibling - 1] = rightNode->ptrToKeys[0];

        //todo: need to return numNodesDeleted after deletion
        return 999;  
      }

      return 0;
    }

    void mergeWithLeftSibling(BPNode* cursor, BPNode* parent, int leftSibling) {
       BPNode* leftNode = (BPNode*) parent->ptrToNode[leftSibling].blockAddress;

       // Transfer all keys and pointers from current node to left node.
      for (int i = leftNode->numOfKeys, j = 0; j < cursor->numOfKeys; i++, j++) {
        leftNode->ptrToKeys[i] = cursor->ptrToKeys[j];
        leftNode->ptrToNode[i] = cursor->ptrToNode[j];
      }

      // Update variables, make left node last pointer point to the next leaf node pointed to by current.
      leftNode->numOfKeys += cursor->numOfKeys;
      leftNode->ptrToNode[leftNode->numOfKeys] = cursor->ptrToNode[cursor->numOfKeys];

      //todo: 
      // We need to update the parent in order to fully remove the current node.
      removeInternalNode(parent->ptrToKeys[leftSibling], parent, cursor);
    }

    void mergeWithRightSibling(BPNode* cursor, BPNode* parent, int rightSibling) {
      BPNode* rightNode = (BPNode*) parent->ptrToNode[rightSibling].blockAddress;
      cout << "rightNode1: " << rightNode->ptrToNode->blockAddress << endl;
      cout << "rightNode2: " << parent->ptrToNode[rightSibling].blockAddress << endl;
      cout << "rootAddress: " << _rootAddressOfStorage.blockAddress << endl;

      // Transfer all keys and pointers from right node into current.
      for (int i = cursor->numOfKeys, j = 0; j < rightNode->numOfKeys; i++, j++) {
        cursor->ptrToKeys[i] = rightNode->ptrToKeys[j];
        cursor->ptrToNode[i] = rightNode->ptrToNode[j];
      }

      // Update variables, make current node last pointer point to the next leaf node pointed to by right node.
      cursor->numOfKeys += rightNode->numOfKeys;
      cursor->ptrToNode[cursor->numOfKeys] = rightNode->ptrToNode[rightNode->numOfKeys];

      //todo: 
      // We need to update the parent in order to fully remove the right node.
      //void *rightNodeAddress = parent->pointers[rightSibling].blockAddress;
      //  removeInternal(parent->keys[rightSibling - 1], (BPNode *)parentDiskAddress, (BPNode *)rightNodeAddress);
      removeInternalNode(parent->ptrToKeys[rightSibling - 1], parent, rightNode);
    }

};