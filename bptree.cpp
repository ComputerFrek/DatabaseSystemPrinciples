#include <tuple>
#include <iostream>
#include <array>
#include <unordered_map>
#include <cstring>
#include <math.h>

#include "storage.cpp"
#include "types.h"

using namespace std;

bool myNullPtr = false;

// A node in the B+ Tree.
class BPNode{
  public:
    Address* pointers;      // A pointer to an array of struct {void *blockAddress, short int offset} containing other nodes in disk.
    int* keys;            // Pointer to an array of keys in this node.
    int numKeys;            // Current number of keys in this node.
    bool isLeaf;            // Whether this node is a leaf node.
    
    BPNode(int maxKeys){
      // Initialize empty array of keys and pointers.
      keys = new int[maxKeys];
      pointers = new Address[maxKeys + 1];

      for (int i = 0; i < maxKeys + 1; i++) {
        //Address nullAddress {(void *)myNullPtr, 0};
        Address nullAddress;
        nullAddress.blockAddress = nullptr;
        nullAddress.offset = 0;
        pointers[i] = nullAddress;
        if(i < maxKeys) {
          keys[i] = 0;
        }
      }
      numKeys = 0;
    }

    //Return the #No of Keys in BPNode :WJ
    int getKeysCount(){
      return numKeys;
    }
};

// The B+ Tree itself.
class BPlusTree {
  private:
    // Variables
    Storage* disk;     // Pointer to a memory pool for data blocks.
    Storage* index;    // Pointer to a memory pool in disk for index.
    BPNode* root;           // Pointer to the main memory root (if it's loaded).
    void* rootAddress;    // Pointer to root's address on disk.
    Address rootStorageAddress;
    int maxKeys;          // Maximum keys in a node.
    int levels;           // Number of levels in this B+ Tree.
    int numNodes;         // Number of nodes in this B+ Tree.
    size_t nodeSize; // Size of a node = Size of block.

    // Updates the parent node to point at both child nodes, and adds a parent node if needed.
    //void insertInternal(int key, BPNode* cursorDiskAddress, BPNode* childDiskAddress) {
    void insertInternal(int key, Address parentStorageAddress, Address newLeafStorageAddress) {
      // Load in cursor (parent) and child from disk to get latest copy.
      //Address cursorAddress{cursorDiskAddress, 0};
      //Address cursorAddress;
      //cursorAddress.blockAddress = cursorDiskAddress;
      //cursorAddress.offset = 0;
      //BPNode* cursor = (BPNode*) index->loadFromDisk(cursorAddress, nodeSize);
      //cout << "Inside insertinsert: " << key << " - parent: " << parentStorageAddress.blockAddress << " - newLeaf: " << newLeafStorageAddress.blockAddress << endl;
      
      BPNode* root = nullptr;
      //cout << "root: " << root << endl;
      BPNode* cursor = (BPNode*) parentStorageAddress.blockAddress;
      //cout << "cursor: " << cursor << endl;

      //cout << "Comparing parentStorageAddress.blockAddress: " << parentStorageAddress.blockAddress << " == " << rootStorageAddress.blockAddress << " :rootStorageAddress.blockAddress" << endl;
      if (parentStorageAddress.blockAddress == rootStorageAddress.blockAddress) {
        //cout << "root was: " << root << endl;
        root = (BPNode*) parentStorageAddress.blockAddress;
        //cout << "root: " << root << endl;
      }

      //Address childAddress{childDiskAddress, 0};
      //Address childAddress;
      //childAddress.blockAddress = newLeafStorageAddress.blockAddress;
      //childAddress.offset = 0;
      //BPNode *child = (BPNode *)index->loadFromDisk(childAddress, nodeSize);

      BPNode* newLeaf = (BPNode*) newLeafStorageAddress.blockAddress;
      //cout << "newLeaf: " << newLeaf << endl;

      // If parent (cursor) still has space, we can simply add the child node as a pointer.
      // We don't have to load parent from the disk again since we still have a main memory pointer to it.
      //cout << "Comparing cursor->numKeys: " << cursor->numKeys << " < " << maxKeys << " :maxKeys" << endl;
      if (cursor->numKeys < maxKeys) {
        //cout << "Less than cursor->numKeys: " << cursor->numKeys << " < " << maxKeys << " :maxKeys" << endl;  
        // Iterate through the parent to see where to put in the lower bound key for the new child.
        int i = 0;
        while (key > cursor->keys[i] && i < cursor->numKeys) {
          //cout << "Search left to right: " << key << " > " << cursor->keys[i] << " & " << i << " < " << cursor->numKeys << endl;
          i++;
        }

        // Now we have i, the index to insert the key in. Bubble swap all keys back to insert the new child's key.
        // We use numKeys as index since we are going to be inserting a new key.
        for (int j = cursor->numKeys; j > i; j--) {
          //cout << "Shifting cursor->keys[" << j << "] > cursor->keys[" << j - 1 << "]" << endl;
          cursor->keys[j] = cursor->keys[j - 1];
        }

        // Shift all pointers one step right (right pointer of key points to lower bound of key).
        for (int j = cursor->numKeys + 1; j > i + 1; j--) {
          //cout << "Shifting cursor->pointers[" << j << "] > cursor->pointers[" << j - 1 << "]" << endl;
          cursor->pointers[j] = cursor->pointers[j - 1];
        }

        // Add in new child's lower bound key and pointer to the parent.
        //cout << "cursor->keys[" << i << "] was: " << cursor->keys[i] << endl;
        cursor->keys[i] = key;
        //cout << "cursor->keys[" << i << "]: " << cursor->keys[i] << endl;

        //cout << "cursor->numKeys was: " << cursor->numKeys << endl;
        cursor->numKeys++;
        //cout << "cursor->numKeys: " << cursor->numKeys << endl;

        // Right side pointer of key of parent will point to the new child node.
        //Address childAddress{childDiskAddress, 0};
        //Address childAddress;
        //childAddress.blockAddress = newLeafStorageAddress.blockAddress;
        //childAddress.offset = 0;

        //cout << "cursor->pointers[" << i + 1 << "] was: " << static_cast<void*>(cursor->pointers[i + 1].blockAddress) + cursor->pointers[i + 1].offset << endl;
        cursor->pointers[i + 1] = newLeafStorageAddress;
        //cout << "cursor->pointers[" << i + 1 << "]: " << static_cast<void*>(cursor->pointers[i + 1].blockAddress) + cursor->pointers[i + 1].offset << endl;

        // Write the updated parent (cursor) to the disk.
        //Address cursorAddress{cursorDiskAddress, 0};
        //Address cursorAddress;
        //cursorAddress.blockAddress = parentStorageAddress.blockAddress;
        //cursorAddress.offset = 0;
        //index->saveToDisk(cursor, nodeSize, cursorAddress);
      } else { // If parent node doesn't have space, we need to recursively split parent node and insert more parent nodes.
        // Make new internal node (split this parent node into two).
        // Note: We DO NOT add a new key, just a new pointer!
        //cout << "Creating newInternal of size " << maxKeys << endl;
        BPNode* newInternal = new BPNode(maxKeys);
        //cout << "Created newInternal at " << newInternal << endl;

        // Same logic as above, keep a temp list of keys and pointers to insert into the split nodes.
        // Now, we have one extra pointer to keep track of (new child's pointer).
        int tempKeyList[maxKeys + 1];
        Address tempPointerList[maxKeys + 2];

        // Copy all keys into a temp key list.
        // Note all keys are filled so we just copy till maxKeys.
        for (int i = 0; i < maxKeys; i++) {
          //cout << "Copying " << i << " of " << maxKeys << " : " << cursor->keys[i] << " to " << "tempKeyList[" << i << "]" << endl;
          tempKeyList[i] = cursor->keys[i];
        }

        // Copy all pointers into a temp pointer list.
        // There is one more pointer than keys in the node so maxKeys + 1.
        for (int i = 0; i < maxKeys + 1; i++) {
          //cout << "Copying " << i << " of " << maxKeys << " : " << static_cast<void*>(cursor->pointers[i].blockAddress) + cursor->pointers[i].offset << " to " << "tempPointerList[" << i << "]" << endl;
          tempPointerList[i] = cursor->pointers[i];
        }

        // Find index to insert key in temp key list.
        int i = 0;
        while (key > tempKeyList[i] && i < maxKeys) {
          //cout << "Search left to right: " << key << " > " << tempKeyList[i] << " & " << i << " < " << maxKeys << endl;
          i++;
        }

        // Swap all elements higher than index backwards to fit new key.
        for (int j = maxKeys; j > i; j--) {
          //cout << "Shifting tempKeyList[" << j << "] <- tempKeyList[" << j - 1 << "]" << endl;
          tempKeyList[j] = tempKeyList[j - 1];
        }

        // Move all pointers back to fit new child's pointer as well.
        for (int j = maxKeys + 1; j > i + 1; j--) {
          //cout << "Shifting tempPointerList[" << j << "] <- tempPointerList[" << j - 1 << "]" << endl;
          tempPointerList[j] = tempPointerList[j - 1];
        }

        // Insert new key into array in the correct spot (sorted).
        //cout << "tempKeyList[" << i << "] was: " << tempKeyList[i] << endl;
        tempKeyList[i] = key;
        //cout << "tempKeyList[" << i << "]: " << tempKeyList[i] << endl;

        // Insert a pointer to the child to the right of its key.
        //Address childAddress = {childDiskAddress, 0};
        //Address childAddress;
        //childAddress.blockAddress = newLeafStorageAddress.blockAddress;
        //childAddress.offset = 0;
        //cout << "tempPointerList[" << i + 1 << "] was: " << static_cast<void*>(tempPointerList[i + 1].blockAddress) + tempPointerList[i + 1].offset << endl;
        tempPointerList[i + 1] = newLeafStorageAddress;
        //cout << "tempPointerList[" << i + 1 << "]: " << static_cast<void*>(tempPointerList[i + 1].blockAddress) + tempPointerList[i + 1].offset << endl;
        
        //cout << "newLeaf->isLeaf was: " << newInternal->isLeaf << endl;
        newInternal->isLeaf = false; // New node is a leaf node.
        //cout << "newLeaf->isLeaf: " << newInternal->isLeaf << endl;

        // Split the two new nodes into two. ⌊(n)/2⌋ keys for left.
        // For right, we drop the rightmost key since we only need to represent the pointer.
        //cursor->numKeys = (maxKeys + 1) / 2;
        //newInternal->numKeys = maxKeys - (maxKeys + 1) / 2;
        //cout << "Calc the new keys after split" << endl;
        //cout << "cursor->numKeys was: " << cursor->numKeys << endl;
        //cout << "newInternal->numKeys was: " << newInternal->numKeys << endl;
        cursor->numKeys = ceil((maxKeys + 1) / 2.0);
        newInternal->numKeys = floor((maxKeys + 1) / 2.0) - 1;
        //cout << "cursor->numKeys: " << cursor->numKeys << endl;
        //cout << "newInternal->numKeys: " << newInternal->numKeys << endl;

        //cout << "Adding the keyptr back to the nodes" << endl;
        // Reassign keys into cursor from tempkeyslist to account for new child node
        for (int i = 0; i < cursor->numKeys; i++) {
          //cout << "Adding to cursor->keys[" << i << "]: " << tempKeyList[i] << endl;
          cursor->keys[i] = tempKeyList[i];
        }

        for(int i = 0; i < cursor->numKeys + 1; i++){
          //cout << "Adding to cursor->pointers[" << i << "]: " << static_cast<void*>(tempPointerList[i].blockAddress) + tempPointerList[i].offset << endl;
          cursor->pointers[i] = tempPointerList[i];
        }

        int j;
        // Insert new keys into the new internal parent node.
        for (int i = 0, j = cursor->numKeys + 1; i < newInternal->numKeys; i++, j++) {
          //cout << "Adding to newInternal->key[" << i << "]: " << tempKeyList[j] << endl;
          newInternal->keys[i] = tempKeyList[j];
        }

        // Insert pointers into the new internal parent node.
        for (int i = 0, j = cursor->numKeys + 1; i < newInternal->numKeys + 1; i++, j++) {
          //cout << "Adding to newInternal->pointers[" << i << "]: " << static_cast<void*>(tempPointerList[j].blockAddress) + tempPointerList[j].offset << endl;
          newInternal->pointers[i] = tempPointerList[j];
        }

        // Note that we don't have to modify keys in the old parent cursor.
        // Because we already reduced its numKeys as we are only adding to the right bound.

        // KIVVVVVVV

        // Get rid of unecessary cursor keys and pointers
        for (int i = cursor->numKeys; i < maxKeys; i++) {
          //cout << "wiping cursor->keys[" << i << "] was : " << cursor->keys[i] << endl;
          cursor->keys[i] = 0;
          //cout << "wiping cursor->keys[" << i << "] : " << cursor->keys[i] << endl;
        }

        for (int i = cursor->numKeys + 1; i < maxKeys + 1; i++) {
          //Address nullAddress{nullptr, 0};
          Address nullAddress;
          nullAddress.blockAddress = nullptr;
          nullAddress.offset = 0;

          //cout << "wiping cursor->pointers[" << i << "] was : " << cursor->pointers[i].blockAddress << " + " << cursor->pointers[i].offset << endl;
          cursor->pointers[i] = nullAddress;
          //cout << "wiping cursor->pointers[" << i << "] : " << cursor->pointers[i].blockAddress << " + " << cursor->pointers[i].offset << endl;
        }

        // assign the new child to the original parent
        //cout << "cursor->pointers[" << cursor->numKeys << "] was: " << static_cast<void*>(cursor->pointers[cursor->numKeys].blockAddress) + cursor->pointers[cursor->numKeys].offset << endl;
        //cursor->pointers[cursor->numKeys] = newLeafStorageAddress;
        //cout << "cursor->pointers[" << cursor->numKeys << "]: " << static_cast<void*>(cursor->pointers[cursor->numKeys].blockAddress) + cursor->pointers[cursor->numKeys].offset << endl;

        // Save the old parent and new internal node to disk.
        //Address cursorAddress{cursorDiskAddress, 0};
        //Address cursorAddress;
        //cursorAddress.blockAddress = parentStorageAddress.blockAddress;
        //cursorAddress.offset = 0;
        //index->saveToDisk(cursor, nodeSize, cursorAddress);

        // Address newInternalAddress{newInternal, 0};
        //Address newInternalDiskAddress = index->saveToDisk(newInternal, nodeSize);
        Address newInternalStorageAddress;
        newInternalStorageAddress.blockAddress = newInternal;
        newInternalStorageAddress.offset = 0;

        // If current cursor is the root of the tree, we need to create a new root.
        if (cursor == root) {
          //cout << "Creating new root BPNode of size " << maxKeys << endl;
          BPNode* newRoot = new BPNode(maxKeys);
          //cout << "Created new root BPNode at " << newRoot << endl;

          // Update newRoot to hold the children.
          // Take the rightmost key of the old parent to be the root.
          // Although we threw it away, we are still using it to denote the leftbound of the old child.
          //cout << "newRoot->keys[0] was: " << newRoot->keys[0] << endl;
          newRoot->keys[0] = tempKeyList[cursor->numKeys];
          //cout << "newRoot->keys[0]: " << newRoot->keys[0] << endl;

          // Update newRoot's children to be the previous two nodes
          //Address cursorAddress = {(BPNode*) parentStorageAddress.blockAddress, 0};
          //cout << "newRoot->pointers[0] " << static_cast<void*>(parentStorageAddress.blockAddress) + parentStorageAddress.offset << endl;
          //cout << "newRoot->pointers[1] " << static_cast<void*>(newInternalStorageAddress.blockAddress) + newInternalStorageAddress.offset << endl;
          newRoot->pointers[0] = parentStorageAddress;
          newRoot->pointers[1] = newInternalStorageAddress;

          // Update variables for newRoot
          //cout << "newRoot->isLeaf was: " << newRoot->isLeaf << endl;
          newRoot->isLeaf = false;
          //cout << "newRoot->isLeaf: " << newRoot->isLeaf << endl;
          //cout << "newRoot->numKeys was: " << newRoot->numKeys << endl;
          newRoot->numKeys = 1;
          //cout << "newRoot->numKeys: " << newRoot->numKeys << endl;

          //cout << "root was: " << root << endl;
          root = newRoot;
          //cout << "root: " << root << endl;

          // Save newRoot into disk.
          //Address newRootAddress = index->saveToDisk(root, nodeSize);

          // Update rootAddress
          //rootAddress = newRootAddress.blockAddress;

          rootStorageAddress.blockAddress = root;
          rootStorageAddress.offset = 0;
        }
        // Otherwise, parent is internal, so we need to split and make a new parent internally again.
        // This is done recursively if needed.
        // Here
        else
        {
          //cout << "Searching for cursor parent " << endl;
          Address newParentStorageAddress = findParent(cursor->keys[0], rootStorageAddress, parentStorageAddress);
          //cout << "Cursor parent found at " << static_cast<void*>(newParentStorageAddress.blockAddress) + newParentStorageAddress.offset << endl;

          // KIVVVVV
          // insertInternal(cursor->keys[cursor->numKeys], parentDiskAddress, (Node *)newInternalDiskAddress.blockAddress);
          //void insertInternal(int key, Address parentStorageAddress, Address newLeafStorageAddress) {
          //cout << "Continue recurrsive insertinsert - " << tempKeyList[cursor->numKeys] << " - " << static_cast<void*>(newParentStorageAddress.blockAddress) + newParentStorageAddress.offset << " - " << static_cast<void*>(newInternalStorageAddress.blockAddress) + newInternalStorageAddress.offset << endl;
          insertInternal(tempKeyList[cursor->numKeys], newParentStorageAddress, newInternalStorageAddress);
          //cout << "Completed recurrsive insertinsert - " << tempKeyList[cursor->numKeys] << " - " << static_cast<void*>(newParentStorageAddress.blockAddress) + newParentStorageAddress.offset << " - " << static_cast<void*>(newInternalStorageAddress.blockAddress) + newInternalStorageAddress.offset << endl;
        }
      }
    }

    // Helper function for deleting records.
    void removeInternal(int key, BPNode *cursorDiskAddress, BPNode *childDiskAddress) {
      // Load in cursor (parent) and child from disk to get latest copy.
      //Address cursorAddress{cursorDiskAddress, 0};
      Address cursorAddress;
      cursorAddress.blockAddress = cursorDiskAddress;
      cursorAddress.offset = 0;
      BPNode *cursor = (BPNode *)index->loadFromDisk(cursorAddress, nodeSize);

      // Check if cursor is root via disk address.
      if (cursorDiskAddress == rootAddress)
      {
        root = cursor;
      }

      // Get address of child to delete.
      //Address childAddress{childDiskAddress, 0};
      Address childAddress;
      childAddress.blockAddress = childDiskAddress;
      childAddress.offset = 0;

      // If current parent is root
      if (cursor == root)
      {
        // If we have to remove all keys in root (as parent) we need to change the root to its child.
        if (cursor->numKeys == 1)
        {
          // If the larger pointer points to child, make it the new root.
          if (cursor->pointers[1].blockAddress == childDiskAddress)
          {
            // Delete the child completely
            index->deallocate(childAddress, nodeSize);

            // Set new root to be the parent's left pointer
            // Load left pointer into main memory and update root.
            root = (BPNode *)index->loadFromDisk(cursor->pointers[0], nodeSize);
            rootAddress = (BPNode *)cursor->pointers[0].blockAddress;

            // We can delete the old root (parent).
            index->deallocate(cursorAddress, nodeSize);

            // Nothing to save to disk. All updates happened in main memory.
            std::cout << "Root node changed." << endl;
            return;
          }
          // Else if left pointer in root (parent) contains the child, delete from there.
          else if (cursor->pointers[0].blockAddress == childDiskAddress)
          {
            // Delete the child completely
            index->deallocate(childAddress, nodeSize);

            // Set new root to be the parent's right pointer
            // Load right pointer into main memory and update root.
            root = (BPNode *)index->loadFromDisk(cursor->pointers[1], nodeSize);
            rootAddress = (BPNode *)cursor->pointers[1].blockAddress;

            // We can delete the old root (parent).
            index->deallocate(cursorAddress, nodeSize);

            // Nothing to save to disk. All updates happened in main memory.
            std::cout << "Root node changed." << endl;
            return;
          }
        }
      }

      // If reach here, means parent is NOT the root.
      // Aka we need to delete an internal node (possibly recursively).
      int pos;

      // Search for key to delete in parent based on child's lower bound key.
      for (pos = 0; pos < cursor->numKeys; pos++)
      {
        if (cursor->keys[pos] == key)
        {
          break;
        }
      }

      // Delete the key by shifting all keys forward
      for (int i = pos; i < cursor->numKeys; i++)
      {
        cursor->keys[i] = cursor->keys[i + 1];
      }

      // Search for pointer to delete in parent
      // Remember pointers are on the RIGHT for non leaf nodes.
      for (pos = 0; pos < cursor->numKeys + 1; pos++)
      {
        if (cursor->pointers[pos].blockAddress == childDiskAddress)
        {
          break;
        }
      }

      // Now move all pointers from that point on forward by one to delete it.
      for (int i = pos; i < cursor->numKeys + 1; i++)
      {
        cursor->pointers[i] = cursor->pointers[i + 1];
      }

      // Update numKeys
      cursor->numKeys--;

      // Check if there's underflow in parent
      // No underflow, life is good.
      if (cursor->numKeys >= (maxKeys + 1) / 2 - 1)
      {
        return;
      }

      // If we reach here, means there's underflow in parent's keys.
      // Try to steal some from neighbouring nodes.
      // If we are the root, we are screwed. Just give up.
      if (cursorDiskAddress == rootAddress)
      {
        return;
      }

      // If not, we need to find the parent of this parent to get our siblings.
      // Pass in lower bound key of our child to search for it.
      //BPNode *parentDiskAddress = findParent((BPNode *)rootAddress, cursorDiskAddress, cursor->keys[0]);
      int leftSibling, rightSibling;

      // Load parent into main memory.
      //Address parentAddress{parentDiskAddress, 0};
      Address parentAddress;
      //parentAddress.blockAddress = parentDiskAddress;
      parentAddress.blockAddress = nullptr;
      parentAddress.offset = 0;
      BPNode *parent = (BPNode *)index->loadFromDisk(parentAddress, nodeSize);

      // Find left and right sibling of cursor, iterate through pointers.
      for (pos = 0; pos < parent->numKeys + 1; pos++)
      {
        if (parent->pointers[pos].blockAddress == cursorDiskAddress)
        {
          leftSibling = pos - 1;
          rightSibling = pos + 1;
          break;
        }
      }

      // Try to borrow a key from either the left or right sibling.
      // Check if left sibling exists. If so, try to borrow.
      if (leftSibling >= 0)
      {
        // Load in left sibling from disk.
        BPNode *leftNode = (BPNode *)index->loadFromDisk(parent->pointers[leftSibling], nodeSize);

        // Check if we can steal (ahem, borrow) a key without underflow.
        // Non leaf nodes require a minimum of ⌊n/2⌋
        if (leftNode->numKeys >= (maxKeys + 1) / 2)
        {
          // We will insert this borrowed key into the leftmost of current node (smaller).
          // Shift all remaining keys and pointers back by one.
          for (int i = cursor->numKeys; i > 0; i--)
          {
            cursor->keys[i] = cursor->keys[i - 1];
          }

          // Transfer borrowed key and pointer to cursor from left node.
          // Basically duplicate cursor lower bound key to keep pointers correct.
          cursor->keys[0] = parent->keys[leftSibling];
          parent->keys[leftSibling] = leftNode->keys[leftNode->numKeys - 1];

          // Move all pointers back to fit new one
          for (int i = cursor->numKeys + 1; i > 0; i--)
          {
            cursor->pointers[i] = cursor->pointers[i - 1];
          }

          // Add pointers to cursor from left node.
          cursor->pointers[0] = leftNode->pointers[leftNode->numKeys];

          // Change key numbers
          cursor->numKeys++;
          leftNode->numKeys--;

          // Update left sibling (shift pointers left)
          leftNode->pointers[cursor->numKeys] = leftNode->pointers[cursor->numKeys + 1];

          // Save parent to disk.
          //Address parentAddress{parentDiskAddress, 0};
          Address parentAddress;
          //parentAddress.blockAddress = parentDiskAddress;
          parentAddress.blockAddress = nullptr;
          parentAddress.offset = 0;
          index->saveToDisk(parent, nodeSize, parentAddress);

          // Save left sibling to disk.
          index->saveToDisk(leftNode, nodeSize, parent->pointers[leftSibling]);

          // Save current node to disk.
          Address cursorAddress = {cursorDiskAddress, 0};
          index->saveToDisk(cursor, nodeSize, cursorAddress);
          return;
        }
      }

      // If we can't take from the left sibling, take from the right.
      // Check if we even have a right sibling.
      if (rightSibling <= parent->numKeys)
      {
        // If we do, load in right sibling from disk.
        BPNode *rightNode = (BPNode *)index->loadFromDisk(parent->pointers[rightSibling], nodeSize);

        // Check if we can steal (ahem, borrow) a key without underflow.
        if (rightNode->numKeys >= (maxKeys + 1) / 2)
        {
          // No need to shift remaining pointers and keys since we are inserting on the rightmost.
          // Transfer borrowed key and pointer (leftmost of right node) over to rightmost of current node.
          cursor->keys[cursor->numKeys] = parent->keys[pos];
          parent->keys[pos] = rightNode->keys[0];

          // Update right sibling (shift keys and pointers left)
          for (int i = 0; i < rightNode->numKeys - 1; i++)
          {
            rightNode->keys[i] = rightNode->keys[i + 1];
          }

          // Transfer first pointer from right node to cursor
          cursor->pointers[cursor->numKeys + 1] = rightNode->pointers[0];

          // Shift pointers left for right node as well to delete first pointer
          for (int i = 0; i < rightNode->numKeys; ++i)
          {
            rightNode->pointers[i] = rightNode->pointers[i + 1];
          }

          // Update numKeys
          cursor->numKeys++;
          rightNode->numKeys--;

          // Save parent to disk.
          //Address parentAddress{parentDiskAddress, 0};
          Address parentAddress;
          //parentAddress.blockAddress = parentDiskAddress;
          parentAddress.blockAddress = nullptr;
          parentAddress.offset = 0;
          index->saveToDisk(parent, nodeSize, parentAddress);

          // Save right sibling to disk.
          index->saveToDisk(rightNode, nodeSize, parent->pointers[rightSibling]);

          // Save current node to disk.
          Address cursorAddress = {cursorDiskAddress, 0};
          index->saveToDisk(cursor, nodeSize, cursorAddress);
          return;
        }
      }

      // If we reach here, means no sibling we can steal from.
      // To resolve underflow, we must merge nodes.

      // If left sibling exists, merge with it.
      if (leftSibling >= 0)
      {
        // Load in left sibling from disk.
        BPNode *leftNode = (BPNode *)index->loadFromDisk(parent->pointers[leftSibling], nodeSize);

        // Make left node's upper bound to be cursor's lower bound.
        leftNode->keys[leftNode->numKeys] = parent->keys[leftSibling];

        // Transfer all keys from current node to left node.
        // Note: Merging will always suceed due to ⌊(n)/2⌋ (left) + ⌊(n-1)/2⌋ (current).
        for (int i = leftNode->numKeys + 1, j = 0; j < cursor->numKeys; j++)
        {
          leftNode->keys[i] = cursor->keys[j];
        }

        // Transfer all pointers too.
        //Address nullAddress{nullptr, 0};
        Address nullAddress;
        nullAddress.blockAddress = nullptr;
        nullAddress.offset = 0;
        for (int i = leftNode->numKeys + 1, j = 0; j < cursor->numKeys + 1; j++)
        {
          leftNode->pointers[i] = cursor->pointers[j];
          cursor->pointers[j] = nullAddress;
        }

        // Update variables, make left node last pointer point to the next leaf node pointed to by current.
        leftNode->numKeys += cursor->numKeys + 1;
        cursor->numKeys = 0;

        // Save left node to disk.
        index->saveToDisk(leftNode, nodeSize, parent->pointers[leftSibling]);

        // Delete current node (cursor)
        // We need to update the parent in order to fully remove the current node.
        //removeInternal(parent->keys[leftSibling], (BPNode *)parentDiskAddress, (BPNode *)cursorDiskAddress);
      }
      // If left sibling doesn't exist, try to merge with right sibling.
      else if (rightSibling <= parent->numKeys)
      {
        // Load in right sibling from disk.
        BPNode *rightNode = (BPNode *)index->loadFromDisk(parent->pointers[rightSibling], nodeSize);

        // Set upper bound of cursor to be lower bound of right sibling.
        cursor->keys[cursor->numKeys] = parent->keys[rightSibling - 1];

        // Note we are moving right node's stuff into ours.
        // Transfer all keys from right node into current.
        // Note: Merging will always suceed due to ⌊(n)/2⌋ (left) + ⌊(n-1)/2⌋ (current).
        for (int i = cursor->numKeys + 1, j = 0; j < rightNode->numKeys; j++)
        {
          cursor->keys[i] = rightNode->keys[j];
        }

        // Transfer all pointers from right node into current.
        Address nullAddress = {nullptr, 0};
        for (int i = cursor->numKeys + 1, j = 0; j < rightNode->numKeys + 1; j++)
        {
          cursor->pointers[i] = rightNode->pointers[j];
          rightNode->pointers[j] = nullAddress;
        }

        // Update variables
        cursor->numKeys += rightNode->numKeys + 1;
        rightNode->numKeys = 0;

        // Save current node to disk.
        //Address cursorAddress{cursorDiskAddress, 0};
        Address cursorAddress;
        cursorAddress.blockAddress = cursorDiskAddress;
        cursorAddress.offset = 0;
        index->saveToDisk(cursor, nodeSize, cursorAddress);

        // Delete right node.
        // We need to update the parent in order to fully remove the right node.
        void *rightNodeAddress = parent->pointers[rightSibling].blockAddress;
        //removeInternal(parent->keys[rightSibling - 1], (BPNode *)parentDiskAddress, (BPNode *)rightNodeAddress);
      }
    }

    // Finds the direct parent of a node in the B+ Tree.
    // Takes in root and a node to find parent for, returns parent's disk address.
    Address findParent(int lowerBoundKey, Address rootStorageAddress, Address childStorageAddress){
      // Load in cursor into main memory, starting from root.
      //Address cursorAddress{cursorDiskAddress, 0};
      //Address cursorAddress;
      //cursorAddress.blockAddress = cursorDiskAddress;
      //cursorAddress.offset = 0;
      //BPNode* cursor = (BPNode*) index->loadFromDisk(cursorAddress, nodeSize);
      BPNode* cursor = (BPNode*) rootStorageAddress.blockAddress;
      BPNode* parent = (BPNode*) cursor;

      Address nullAddress;
      nullAddress.blockAddress = nullptr;
      nullAddress.offset = 0;

      // If the root cursor passed in is a leaf node, there is no children, therefore no parent.
      if (cursor->isLeaf == true) {
        return nullAddress;
      }

      // Maintain parentDiskAddress
      //BPNode* parentDiskAddress = cursorDiskAddress;
      
      // While not leaf, keep following the nodes to correct key.
      while (cursor->isLeaf == false) {
        // Check through all pointers of the node to find match.
        for (int i = 0; i < cursor->numKeys + 1; i++) {
          //Check if the child belongs to this object
          if (cursor->pointers[i].blockAddress == childStorageAddress.blockAddress) {
            Address parentStorageAddress;
            parentStorageAddress.blockAddress = parent;
            parentStorageAddress.offset = 0;

            return parentStorageAddress;
          }
        }

        //If search all keys in this node liao still cannot find then travese down.
        for (int i = 0; i < cursor->numKeys; i++) {
          // If key is lesser than current key, go to the left pointer's node.
          if (lowerBoundKey < cursor->keys[i]) {
            // Load node in from disk to main memory.
            //BPNode *mainMemoryNode = (BPNode *)index->loadFromDisk(cursor->pointers[i], nodeSize);
            
            // Update parent address.
            //parentStorageAddress.blockAddress = cursorStorageAddress.blockAddress;
            parent = cursor;

            // Move to new node in main memory.
            cursor = (BPNode*) cursor->pointers[i].blockAddress;
            break;
          }

          // Else if key larger than all keys in the node, go to last pointer's node (rightmost).
          if (i == cursor->numKeys - 1) {
            // Load node in from disk to main memory.
            //BPNode *mainMemoryNode = (BPNode *)index->loadFromDisk(cursor->pointers[i + 1], nodeSize);

            // Update parent address.
            //parentDiskAddress = (BPNode *)cursor->pointers[i + 1].blockAddress;
            parent = cursor;

            // Move to new node in main memory.
            //cursor = (BPNode *)mainMemoryNode;
            cursor = (BPNode*) cursor->pointers[i + 1].blockAddress;
            break;
          }
        }
      }

      // If we reach here, means cannot find already.
      return nullAddress;
    }

  public:
    // Constructor, takes in block size to determine max keys/pointers in a node.
    BPlusTree(size_t blockSize, Storage* disk, Storage* index){
      // Get size left for keys and pointers in a node after accounting for node's isLeaf and numKeys attributes.
      size_t nodeBufferSize = blockSize - sizeof(bool) - sizeof(int);
      //size_t nodeBufferSize = blockSize;

      // Set max keys available in a node. Each key is a int, each pointer is a struct of {void *blockAddress, short int offset}.
      // Therefore, each key is 4 bytes. Each pointer is around 16(8 + 2 + 6?) bytes.

      // Initialize node buffer with a pointer. P | K | P , always one more pointer than keys.
      size_t sum = sizeof(Address);
      maxKeys = 0;

      // Try to fit as many pointer key pairs as possible into the node block.
      // int for key
      // address for ptr
      while (sum + sizeof(Address) + sizeof(int) <= nodeBufferSize) {
        sum += (sizeof(Address) + sizeof(int));
        maxKeys += 1;
      }

      if (maxKeys == 0) {
        throw std::overflow_error("Error: Keys and pointers too large to fit into a node!");
      }

      // Initialize root to NULL
      rootStorageAddress.blockAddress = nullptr;
      rootStorageAddress.offset = 0;
      root = nullptr;

      // Set node size to be equal to block size.
      nodeSize = blockSize;

      // Initialize initial variables
      levels = 0;
      numNodes = 0;

      // Initialize disk space for index and set reference to disk.
      
      this->disk = disk;
      this->index = index;
    }

    // Search for keys corresponding to a range in the B+ Tree given a lower and upper bound. Returns a list of matching Records.
    void search(int lowerBoundKey, int upperBoundKey) {
      // Tree is empty.
      if (rootAddress == nullptr)
      {
        throw std::logic_error("Tree is empty!");
      }
      // Else iterate through root node and follow the keys to find the correct key.
      else
      {
        // Load in root from disk.
        //Address rootDiskAddress{rootAddress, 0};
        Address rootDiskAddress;
        rootDiskAddress.blockAddress = rootAddress;
        rootDiskAddress.offset = 0;
        root = (BPNode *)index->loadFromDisk(rootDiskAddress, nodeSize);

        // for displaying to output file
        std::cout << "Index node accessed. Content is -----";
        displayNode(root);

        BPNode *cursor = root;

        bool found = false;

        // While we haven't hit a leaf node, and haven't found a range.
        while (cursor->isLeaf == false)
        {
          // Iterate through each key in the current node. We need to load nodes from the disk whenever we want to traverse to another node.
          for (int i = 0; i < cursor->numKeys; i++)
          {
            // If lowerBoundKey is lesser than current key, go to the left pointer's node to continue searching.
            if (lowerBoundKey < cursor->keys[i])
            {
              // Load node from disk to main memory.
              cursor = (BPNode *)index->loadFromDisk(cursor->pointers[i], nodeSize);

              // for displaying to output file
              std::cout << "Index node accessed. Content is -----";
              displayNode(cursor);

              break;
            }
            // If we reached the end of all keys in this node (larger than all), then go to the right pointer's node to continue searching.
            if (i == cursor->numKeys - 1)
            {
              // Load node from disk to main memory.
              // Set cursor to the child node, now loaded in main memory.
              cursor = (BPNode *)index->loadFromDisk(cursor->pointers[i + 1], nodeSize);

              // for displaying to output file
              std::cout << "Index node accessed. Content is -----";
              displayNode(cursor);
              break;
            }
          }
        }

        // When we reach here, we have hit a leaf node corresponding to the lowerBoundKey.
        // Again, search each of the leaf node's keys to find a match.
        // vector<Record> results;
        // unordered_map<void *, void *> loadedBlocks; // Maintain a reference to all loaded blocks in main memory.

        // Keep searching whole range until we find a key that is out of range.
        bool stop = false;

        while (stop == false)
        {
          int i;
          for (i = 0; i < cursor->numKeys; i++)
          {
            // Found a key within range, now we need to iterate through the entire range until the upperBoundKey.
            if (cursor->keys[i] > upperBoundKey)
            {
              stop = true;
              break;
            }
            if (cursor->keys[i] >= lowerBoundKey && cursor->keys[i] <= upperBoundKey)
            {
              // for displaying to output file
              std::cout << "Index node (LLNode) accessed. Content is -----";
              displayNode(cursor);

              // Add new line for each leaf node's linked list printout.
              std::cout << endl;
              std::cout << "LLNode: tconst for average rating: " << cursor->keys[i] << " > ";          

              // Access the linked list node and print records.
              displayLL(cursor->pointers[i]);
            }
          }

          // On the last pointer, check if last key is max, if it is, stop. Also stop if it is already equal to the max
          if (cursor->pointers[cursor->numKeys].blockAddress != nullptr && cursor->keys[i] != upperBoundKey)
          {
            // Set cursor to be next leaf node (load from disk).
            cursor = (BPNode *)index->loadFromDisk(cursor->pointers[cursor->numKeys], nodeSize);

            // for displaying to output file
            std::cout << "Index node accessed. Content is -----";
            displayNode(cursor);

          }
          else
          {
            stop = true;
          }
        }
      }
      return;
    }

    // Inserts a record into the B+ Tree.
    void insert(Address recordaddress, int key){
      BPNode* root = (BPNode*) rootStorageAddress.blockAddress;
      // If no root exists, create a new B+ Tree root.
      //cout << "Inserting record " << key << " - " << static_cast<void*>(recordaddress.blockAddress) << " + " << recordaddress.offset << " = " << static_cast<void*>(recordaddress.blockAddress) + recordaddress.offset << endl;
      if (root == nullptr) {
        //cout << "No root exists for " << key << " - " << static_cast<void*>(recordaddress.blockAddress) << " + " << recordaddress.offset << " = " << static_cast<void*>(recordaddress.blockAddress) + recordaddress.offset << endl;
        
        // Create a new linked list (for duplicates) at the key.
        //cout << "Create LLNode to handle duplicates" << endl;
        //LLNode llnode;
        //llnode.dataaddress.blockAddress = recordaddress.blockAddress;
        //llnode.dataaddress.offset = recordaddress.offset;
        //llnode.next = nullptr;

        //Address LLNodeAddress;
        //LLNodeAddress.blockAddress = &llnode;
        //LLNodeAddress.offset = 0;

        // Allocate LLNode and root address
        //cout << "Writing LLNode - " << static_cast<void*>(LLNode) << " + " << nodeSize << " = " << static_cast<void*>(LLNode) + nodeSize << endl;
        //Address LLNodeAddress = index->saveToDisk((void*) LLNode, nodeSize);
        //cout << "Wrote LLNode - " << static_cast<void*>(LLNodeAddress.blockAddress) << " + " << LLNodeAddress.offset << " = " << static_cast<void*>(LLNodeAddress.blockAddress) + LLNodeAddress.offset << endl;

        // Create new node in main memory, set it to root, and add the key and values to it.
        //cout << "Creating new BPNode of size " << maxKeys << endl;
        root = new BPNode(maxKeys);
        root->keys[0] = key;
        root->isLeaf = true; // It is both the root and a leaf.
        root->numKeys = 1;
        root->pointers[0] = recordaddress; // Add record's disk address to pointer.
        //cout << "LLNodeAddress: " << static_cast<void*>(LLNodeAddress.blockAddress) << endl;
        //cout << "Created new root BPNode at " << static_cast<void*>(root) << endl;

        // Write the root node into disk and track of root node's disk address.
        //cout << "Writing root BPNode to disk" << endl;
        //rootAddress = index->saveToDisk(root, nodeSize).blockAddress;
        //cout << "Wrote root BPNode to disk " << static_cast<void*>(rootAddress) << endl;
        rootStorageAddress.blockAddress = root;
        rootStorageAddress.offset = 0;
      } else { // Else if root exists already, traverse the nodes to find the proper place to insert the key.
        //cout << "Root exists for key " << key << " - " << static_cast<void*>(recordaddress.blockAddress) << " + " << recordaddress.offset << " = " << static_cast<void*>(recordaddress.blockAddress) + recordaddress.offset << endl;
        BPNode* cursor = root;
        BPNode* parent = nullptr;                          // Keep track of the parent as we go deeper into the tree in case we need to update it.
        Address parentDiskAddress; // Keep track of parent's disk address as well so we can update parent in disk.
        parentDiskAddress.blockAddress = nullptr;
        parentDiskAddress.offset = 0;
        Address cursorDiskAddress; // Store current node's disk address in case we need to update it in disk.
        cursorDiskAddress.blockAddress = root;
        cursorDiskAddress.offset = 0;

        //cout << "address: " << static_cast<void*>(recordaddress.blockAddress) << " + " << recordaddress.offset << endl;
        //cout << "Cursor: " << static_cast<void*>(cursor) << endl;
        //cout << "Parent: " << static_cast<void*>(parent) << endl;
        //cout << "parentDiskAddress: " << static_cast<void*>(parentDiskAddress) << endl;
        //cout << "cursorDiskAddress: " << static_cast<void*>(cursorDiskAddress) << endl;

        int level = 0;
        //cout << "Started going to leaf node" << endl;
        while (cursor->isLeaf == false) { // While not leaf, keep following the nodes to correct key.
          // Set the parent of the node (in case we need to assign new child later), and its disk address.
          //cout << "Parent was: " << static_cast<void*>(parent) << endl;
          //cout << "Parentdiskadd was: " << static_cast<void*>(parentDiskAddress) << endl;

          parent = cursor;
          //parentDiskAddress = cursorDiskAddress;
          parentDiskAddress.blockAddress = parent;

          //cout << "Parent: " << static_cast<void*>(parent) << endl;
          //cout << "Parentdiskadd: " << static_cast<void*>(parentDiskAddress) << endl;
          //cout << "Cursor: " << static_cast<void*>(cursor) << endl;
          //cout << "Cursordiskadd: " << static_cast<void*>(cursorDiskAddress) << endl;

          // Check through all keys of the node to find key and pointer to follow downwards.
          //cout << "Checking thru all keys of node and determine left or right" << endl;
          for (int i = 0; i < cursor->numKeys; i++) {
            // If key is lesser than current key, go to the left pointer's node.
            //cout << "Level: " << level << " - Leaf: " << cursor->isLeaf << " Checking thru all keys of node: " << i << " < " << cursor->numKeys << endl;

            //cout << "Level: " << level << " - Key: " << key << " < " << cursor->keys[i] << " :cursor-keys[ " << i << " ] " << endl;
            if (key < cursor->keys[i]) {
              //cout << "key " << key << " lesser than " << cursor->keys[i] << endl;

              // Load node in from disk to main memory.
              //cout << "Loading BPNode into mainMemoryNode: " << static_cast<void*>(cursor->pointers[i].blockAddress) << " : " << cursor->pointers[i].offset << " - " << nodeSize << endl;
              //BPNode* mainMemoryNode = (BPNode*) index->loadFromDisk(cursor->pointers[i], nodeSize);
              //cout << "BPNode: " << mainMemoryNode->keys[0] << endl;

              // Update cursorDiskAddress to maintain address in disk if we need to update nodes.
              //cout << "Cursordiskadd was: " << static_cast<void*>(cursorDiskAddress) << endl;
              //cursorDiskAddress = cursor->pointers[i].blockAddress;
              //cout << "Cursordiskadd: " << static_cast<void*>(cursorDiskAddress) << endl;

              // Move to new node in main memory.
              //cout << "Cursor was: " << static_cast<void*>(cursor) << endl;
              //cursor = mainMemoryNode;
              cursor = (BPNode*) cursor->pointers[i].blockAddress;
              cursorDiskAddress.blockAddress = cursor;
              //cout << "Cursor: " << static_cast<void*>(cursor) << endl;
              break;
            }
            
            //cout << "Checking if larger than all other keys: " << i << " == " << cursor->numKeys - 1 << endl;
            if (i == cursor->numKeys - 1) { // Else if key larger than all keys in the node, go to last pointer's node (rightmost).
              //cout << "Larger than all other keys in node going to last: " << i << " == " << cursor->numKeys - 1 << endl;

              // Load node in from disk to main memory.
              //cout << "Loading BPNode into mainMemoryNode: " << static_cast<void*>(cursor->pointers[i + 1].blockAddress) << " : " << cursor->pointers[i + 1].offset << " - " << nodeSize << endl;
              //BPNode* mainMemoryNode = (BPNode*) index->loadFromDisk(cursor->pointers[i + 1], nodeSize);
              //cout << "BPNode: " << mainMemoryNode->keys[0] << endl;

              // Update diskAddress to maintain address in disk if we need to update nodes.
              //cout << "Cursordiskadd was: " << static_cast<void*>(cursorDiskAddress) << endl;
              //cursorDiskAddress = cursor->pointers[i + 1].blockAddress;
              //cout << "Cursordiskadd: " << static_cast<void*>(cursorDiskAddress) << endl;

              // Move to new node in main memory.
              //cout << "Cursor was: " << static_cast<void*>(cursor) << endl;
              //cursor = mainMemoryNode;
              cursor = (BPNode*) cursor->pointers[i + 1].blockAddress;
              cursorDiskAddress.blockAddress = cursor;
              //cout << "Cursor: " << static_cast<void*>(cursor) << endl;
              break;
            }
          }

          level++;
        }

        //cout << "Reached leaf node" << endl;
        // When we reach here, it means we have hit a leaf node. Let's find a place to put our new record in.
        // If this leaf node still has space to insert a key, then find out where to put it.
        //cout << "Comparing: " << cursor->numKeys << " < " << maxKeys << endl;
        if (cursor->numKeys < maxKeys) {
          int i = 0;
          
          while (key > cursor->keys[i] && i < cursor->numKeys) { // While we haven't reached the last key and the key we want to insert is larger than current key, keep moving forward.
            //cout << "Search left to right: " << key << " > " << cursor->keys[i] << " & " << i << " < " << cursor->numKeys << endl;
            i++;
          }

          // i is where our key goes in. Check if it's already there (duplicate).
          //cout << "Checking for duplicates " << cursor->keys[i] << " == " << key << endl;
          if (cursor->keys[i] == key) {
            // If it's a duplicate, linked list already exists. Insert into linked list.
            // Insert and update the linked list head.
            //cout << "Inserting to LL " << static_cast<void*>(cursor->pointers[i].blockAddress) << " - " << static_cast<void*>(address.blockAddress) << " - " << key << endl;
            //cursor->pointers[i] = insertLL(cursor->pointers[i], address, key);
            cout << "Duplicate: " << cursor->keys[i] << endl;

            //Address LLNodeAddress;
            //LLNodeAddress.blockAddress = cursor->pointers[i].blockAddress;
            //LLNodeAddress.offset = cursor->pointers[i].offset;

            //LLNode* prenode = (LLNode*) LLNodeAddress.blockAddress;
            //LLNode curnode;
            //curnode.dataaddress.blockAddress = recordaddress.blockAddress;
            //curnode.dataaddress.offset = recordaddress.offset;
            //curnode.next = nullptr;

            //while(prenode->next != nullptr){
            //  prenode = prenode->next;
            //}

            //prenode->next = &curnode;
          } else {
            // Update the last pointer to point to the previous last pointer's node. Aka maintain cursor -> Y linked list.
            //cout << "cursor->numKeys: " << cursor->numKeys << endl;
            Address next = cursor->pointers[cursor->numKeys];
            //cout << "next: " << static_cast<void*>(next.blockAddress) << " - " << next.offset << endl;

            // Now i represents the index we want to put our key in. We need to shift all keys in the node back to fit it in.
            // Swap from number of keys + 1 (empty key) backwards, moving our last key back and so on. We also need to swap pointers.
            for (int j = cursor->numKeys; j > i; j--) {
              // Just do a simple bubble swap from the back to preserve index order.
              //cout << "Shifting cursor->keys[" << j << "] > cursor->keys[" << j - 1 << "]" << endl;
              //cout << "Shifting cursor->pointers[" << j << "] > cursor->pointers[" << j - 1 << "]" << endl;
              cursor->keys[j] = cursor->keys[j - 1];
              cursor->pointers[j] = cursor->pointers[j - 1];
            }

            // Insert our new key and pointer into this node.
            //cout << "cursor->keys[" << i << "] was: " << cursor->keys[i] << endl;
            cursor->keys[i] = key;
            //cout << "cursor->keys[" << i << "]: " << cursor->keys[i] << endl;

            // We need to make a new linked list to store our record.
            // Create a new linked list (for duplicates) at the key.
            //cout << "Create LLNode to handle duplicates: " << maxKeys << endl;
            /*
            BPNode* LLNode = new BPNode(maxKeys);
            LLNode->keys[0] = key;
            LLNode->isLeaf = false; // So we will never search it
            LLNode->numKeys = 1;
            LLNode->pointers[0] = address; // The disk address of the key just inserted
            */

            // Allocate LLNode into disk. Here?
            //cout << "Writing LLNode - " << static_cast<void*>(LLNode) << " + " << nodeSize << " = " << static_cast<void*>(LLNode) + nodeSize << endl;
            //Address LLNodeAddress = index->saveToDisk(LLNode, nodeSize);
            //cout << "Wrote LLNode - " << static_cast<void*>(LLNodeAddress.blockAddress) << " + " << LLNodeAddress.offset << " = " << static_cast<void*>(LLNodeAddress.blockAddress) + LLNodeAddress.offset << endl;

            //To handle duplicates
            //LLNode llnode;
            //llnode.dataaddress.blockAddress = recordaddress.blockAddress;
            //llnode.dataaddress.offset = recordaddress.offset;
            //llnode.next = nullptr;
            
            //Address LLNodeAddress;
            //LLNodeAddress.blockAddress = &llnode;
            //LLNodeAddress.offset = 0;

            // Update variables
            //cout << "cursor->pointers[" << i << "] was: " << static_cast<void*>(cursor->pointers[i].blockAddress) + cursor->pointers[i].offset << endl;
            cursor->pointers[i] = recordaddress;
            //cout << "cursor->pointers[" << i << "]: " << static_cast<void*>(cursor->pointers[i].blockAddress) + cursor->pointers[i].offset << endl;

            //cout << "cursor->numKeys was: " << cursor->numKeys << endl;
            cursor->numKeys++;
            //cout << "cursor->numKeys: " << cursor->numKeys << endl;
      
            // Update leaf node pointer link to next node
            cursor->pointers[cursor->numKeys] = next;

            // Now insert operation is complete, we need to store this updated node to disk.
            // cursorDiskAddress is the address of node in disk, cursor is the address of node in main memory.
            // In this case, we count read/writes as 1/O only (Assume block remains in main memory).
            //Address cursorOriginalAddress{cursorDiskAddress, 0};
            //Address cursorOriginalAddress;
            //cursorOriginalAddress.blockAddress = cursorDiskAddress;
            //cursorOriginalAddress.offset = 0;

            //cout << "Writing cursorOriginalAddress: " << static_cast<void*>(cursorOriginalAddress.blockAddress) << " -> " << static_cast<void*>(cursorOriginalAddress.blockAddress) + cursorOriginalAddress.offset + nodeSize << " - " << cursorOriginalAddress.offset << " - " << nodeSize << endl;
            //Address test = index->saveToDisk(cursor, nodeSize, cursorOriginalAddress);
            //cout << "Wrote cursorOriginalAddress: " << static_cast<void*>(test.blockAddress) << " -> " << static_cast<void*>(test.blockAddress) + test.offset + nodeSize << " - " << test.offset << " - " << nodeSize << endl;
          }
        } else { // Overflow: If there's no space to insert new key, we have to split this node into two and update the parent if required.
          // Create a new leaf node to put half the keys and pointers in.
          //cout << "Creating BPNode of size " << maxKeys << endl;
          BPNode* newLeaf = new BPNode(maxKeys);
          //cout << "Created new BPNode at " << static_cast<void*>(newLeaf) << endl;

          // Copy all current keys and pointers (including new key to insert) to a temporary list.
          int tempKeyList[maxKeys + 1];

          // We only need to store pointers corresponding to records (ignore those that points to other nodes).
          // Those that point to other nodes can be manipulated by themselves without this array later.
          Address tempPointerList[maxKeys + 1];
          Address next = cursor->pointers[cursor->numKeys];

          // Copy all keys and pointers to the temporary lists.
          for (int i = 0; i < maxKeys; i++) {
            //cout << "Copying " << i << " of " << maxKeys << " : " << cursor->keys[i] << " - " << static_cast<void*>(cursor->pointers[i].blockAddress) + cursor->pointers[i].offset << endl;
            tempKeyList[i] = cursor->keys[i];
            tempPointerList[i] = cursor->pointers[i];
          }

          // Insert the new key into the temp key list, making sure that it remains sorted. Here, we find where to insert it.
          int i = 0;
          while (key > tempKeyList[i] && i < maxKeys) {
            //cout << "Search left to right: " << key << " > " << tempKeyList[i] << " & " << i << " < " << maxKeys << endl;
            i++;
          }

          // KIVVVVVVVVVV OUT OF RANGE

          // i is where our key goes in. Check if it's already there (duplicate).
          // make sure it is not the last one 
          if (i < cursor->numKeys) {
            if (cursor->keys[i] == key) {
              // If it's a duplicate, linked list already exists. Insert into linked list.
              // Insert and update the linked list head.
              //cursor->pointers[i] = insertLL(cursor->pointers[i], address, key);
              //return;
              cout << "Duplicate Overflow: " << cursor->keys[i] << endl;
              //Address LLNodeAddress;
              //LLNodeAddress.blockAddress = cursor->pointers[i].blockAddress;
              //LLNodeAddress.offset = cursor->pointers[i].offset;

              //LLNode* prenode = (LLNode*) LLNodeAddress.blockAddress;
              //LLNode curnode;
              //curnode.dataaddress.blockAddress = recordaddress.blockAddress;
              //curnode.dataaddress.offset = recordaddress.offset;
              //curnode.next = nullptr;

              //while(prenode->next != nullptr){
              //  prenode = prenode->next;
              //}

              //prenode->next = &curnode;
              return;
            } 
          }

          // Else no duplicate, insert new key.
          // The key should be inserted at index i in the temporary lists. Move all elements back.
          for (int j = maxKeys; j > i; j--) {
            // Bubble swap all elements (keys and pointers) backwards by one index.
            //cout << "Shifting tempKeyList[" << j << "] <- tempKeyList[" << j - 1 << "]" << endl;
            //cout << "Shifting tempPointerList[" << j << "] <- tempPointerList[" << j - 1 << "]" << endl;
            tempKeyList[j] = tempKeyList[j - 1];
            tempPointerList[j] = tempPointerList[j - 1];
          }

          // Insert the new key and pointer into the temporary lists.
          //cout << "tempKeyList[" << i << "] was: " << tempKeyList[i] << endl;
          tempKeyList[i] = key;
          //cout << "tempKeyList[" << i << "]: " << tempKeyList[i] << endl;

          // The address to insert will be a new linked list node.
          // Create a new linked list (for duplicates) at the key.
          /*
          BPNode *LLNode = new BPNode(maxKeys);
          LLNode->keys[0] = key;
          LLNode->isLeaf = false; // So we will never search it
          LLNode->numKeys = 1;
          LLNode->pointers[0] = address; // The disk address of the key just inserted
          */

          //LLNode llnode;
          //llnode.dataaddress.blockAddress = recordaddress.blockAddress;
          //llnode.dataaddress.offset = recordaddress.offset;
          //llnode.next = nullptr;
          
          //Address LLNodeAddress;
          //LLNodeAddress.blockAddress = &llnode;
          //LLNodeAddress.offset = 0;

          // Allocate LLNode into disk.
          //Address LLNodeAddress = index->saveToDisk((void *)LLNode, nodeSize);
          //cout << "tempPointerList[" << i << "] was: " << static_cast<void*>(tempPointerList[i].blockAddress) + tempPointerList[i].offset << endl;
          tempPointerList[i] = recordaddress;
          //cout << "tempPointerList[" << i << "]: " << static_cast<void*>(tempPointerList[i].blockAddress) + tempPointerList[i].offset << endl;
          
          //cout << "newLeaf->isLeaf was: " << newLeaf->isLeaf << endl;
          newLeaf->isLeaf = true; // New node is a leaf node.
          //cout << "newLeaf->isLeaf: " << newLeaf->isLeaf << endl;

          // Split the two new nodes into two. ⌊(n+1)/2⌋ keys for left, n+1 - ⌊(n+1)/2⌋ (aka remaining) keys for right.
          //cursor->numKeys = (maxKeys + 1) / 2;
          //newLeaf->numKeys = (maxKeys + 1) - ((maxKeys + 1) / 2);
          //cout << "Calc the new keys after split" << endl;
          //cout << "cursor->numKeys was: " << cursor->numKeys << endl;
          //cout << "newLeaf->numKeys was: " << newLeaf->numKeys << endl;
          cursor->numKeys = ceil((maxKeys + 1) / 2.0);
          newLeaf->numKeys = floor((maxKeys + 1) / 2.0);
          //cout << "cursor->numKeys: " << cursor->numKeys << endl;
          //cout << "newLeaf->numKeys: " << newLeaf->numKeys << endl;

          // Set the last pointer of the new leaf node to point to the previous last pointer of the existing node (cursor).
          // Essentially newLeaf -> Y, where Y is some other leaf node pointer wherein cursor -> Y previously.
          // We use maxKeys since cursor was previously full, so last pointer's index is maxKeys.
          //cout << "newLeaf->pointers[" << newLeaf->numKeys << "] was: " << static_cast<void*>(next.blockAddress) + next.offset << endl;
          newLeaf->pointers[newLeaf->numKeys] = next;
          //cout << "newLeaf->pointers[" << newLeaf->numKeys << "] : " << static_cast<void*>(next.blockAddress) + next.offset << endl;

          // Now we need to deal with the rest of the keys and pointers.
          // Note that since we are at a leaf node, pointers point directly to records on disk.

          // Add in keys and pointers in both the existing node, and the new leaf node.
          // First, the existing node (cursor).
          //cout << "Adding the keyptr back to the nodes" << endl;
          for (i = 0; i < cursor->numKeys; i++) {
            //cout << "Adding to cursor " << i << " : " << tempKeyList[i] << " - " << static_cast<void*>(tempPointerList[i].blockAddress) + tempPointerList[i].offset << endl;
            cursor->keys[i] = tempKeyList[i];
            cursor->pointers[i] = tempPointerList[i];
          }

          // Then, the new leaf node. Note we keep track of the i index, since we are using the remaining keys and pointers.
          for (int j = 0; j < newLeaf->numKeys; i++, j++) {
            //cout << "Adding to newLeaf " << j << " : " << tempKeyList[i] << " - " << static_cast<void*>(tempPointerList[i].blockAddress) + tempPointerList[i].offset << endl;
            newLeaf->keys[j] = tempKeyList[i];
            newLeaf->pointers[j] = tempPointerList[i];
          }

          // Now that we have finished updating the two new leaf nodes, we need to write them to disk.
          //cout << "Writing new leaf BPNode to disk" << endl;
          //Address newLeafAddress = index->saveToDisk(newLeaf, nodeSize);
          Address newLeafStorageAddress;
          newLeafStorageAddress.blockAddress = newLeaf;
          newLeafStorageAddress.offset = 0;
          //cout << "Wrote new leaf BPNode to disk " << static_cast<void*>(newLeafAddress.blockAddress) << " -> " << static_cast<void*>(newLeafAddress.blockAddress) + newLeafAddress.offset << " - " << newLeafAddress.offset << endl;

          // Now to set the cursors' pointer to the disk address of the leaf and save it in place
          //cout << "cursor->pointers[" << cursor->numKeys << "] was: " << static_cast<void*>(cursor->pointers[cursor->numKeys].blockAddress) + cursor->pointers[cursor->numKeys].offset << endl;
          cursor->pointers[cursor->numKeys] = newLeafStorageAddress;
          //cout << "cursor->pointers[" << cursor->numKeys << "] : " << static_cast<void*>(newLeafStorageAddress.blockAddress) + newLeafStorageAddress.offset << endl;

          // wipe out the wrong pointers and keys from cursor
          for (int i = cursor->numKeys; i < maxKeys; i++) {
            //cout << "wiping cursor->keys[" << i << "] was : " << cursor->keys[i] << endl;
            cursor->keys[i] = 0;
            //cout << "wiping cursor->keys[" << i << "] : " << cursor->keys[i] << endl;
          }

          for (int i = cursor->numKeys + 1; i < maxKeys + 1; i++) {
            //Address nullAddress{nullptr, 0};
            Address nullAddress;
            nullAddress.blockAddress = nullptr;
            nullAddress.offset = 0;

            //cout << "wiping cursor->pointers[" << i << "] was : " << cursor->pointers[i].blockAddress + cursor->pointers[i].offset << endl;
            cursor->pointers[i] = nullAddress;
            //cout << "wiping cursor->pointers[" << i << "] : " << cursor->pointers[i].blockAddress + cursor->pointers[i].offset << endl;
          }

          //Address cursorOriginalAddress{cursorDiskAddress, 0};
          //Rewrite back the cursor with the divided childs
          //Address cursorOriginalAddress;
          //cursorOriginalAddress.blockAddress = cursorDiskAddress;
          //cursorOriginalAddress.offset = 0;

          //cout << "Writing cursorOriginalAddress: " << static_cast<void*>(cursorOriginalAddress.blockAddress) << " -> " << static_cast<void*>(cursorOriginalAddress.blockAddress) + cursorOriginalAddress.offset + nodeSize << " - " << cursorOriginalAddress.offset << " - " << nodeSize << endl;
          //Address test = index->saveToDisk(cursor, nodeSize, cursorOriginalAddress);
          //cout << "Wrote cursorOriginalAddress: " << static_cast<void*>(test.blockAddress) << " -> " << static_cast<void*>(test.blockAddress) + test.offset + nodeSize << " - " << test.offset << " - " << nodeSize << endl;

          // If we are at root (aka root == leaf), then we need to make a new parent root.
          if (cursor == root) {
            //cout << "Creating new root BPNode of size " << maxKeys << endl;
            BPNode* newRoot = new BPNode(maxKeys);
            //cout << "Created new root BPNode at " << newRoot << endl;

            // We need to set the new root's key to be the left bound of the right child.
            //cout << "newRoot->keys[0] was: " << newRoot->keys[0] << endl;
            newRoot->keys[0] = newLeaf->keys[0];
            //cout << "newRoot->keys[0]: " << newRoot->keys[0] << endl;

            // Point the new root's children as the existing node and the new node.
            //Address cursorDisk{cursorDiskAddress, 0};
            //Address cursorDisk;
            //cursorDisk.blockAddress = cursorDiskAddress;
            //cursorDisk.blockAddress = cursor;
            //cursorDisk.offset = 0;

            //cout << "newRoot->pointers[0] " << cursorDiskAddress.blockAddress << " + " << cursorDiskAddress.offset << endl;
            //cout << "newRoot->pointers[1] " << newLeafStorageAddress.blockAddress << " + " << newLeafStorageAddress.offset << endl;
            newRoot->pointers[0] = cursorDiskAddress;
            newRoot->pointers[1] = newLeafStorageAddress;

            // Update new root's variables.
            newRoot->isLeaf = false;
            newRoot->numKeys = 1;

            // Write the new root node to disk and update the root disk address stored in B+ Tree.
            //cout << "Writing new root BPNode to disk" << endl;
            //rootAddress = index->saveToDisk(newRoot, nodeSize).blockAddress;
            //cout << "Wrote root BPNode to disk " << static_cast<void*>(rootAddress) << endl;

            //cout << "root was: " << static_cast<void*>(root) << endl;
            root = newRoot;
            //cout << "root: " << static_cast<void*>(root) << endl;
            
            rootStorageAddress.blockAddress = root;
            rootStorageAddress.offset = 0;
          } else { // If we are not at the root, we need to insert a new parent in the middle levels of the tree.
            //cout << "Calling insertinsert for parent in middle: " << newLeaf->keys[0] << endl;
            insertInternal(newLeaf->keys[0], parentDiskAddress, newLeafStorageAddress);
            //cout << "Completed insertinsert for parent in middle: " << newLeaf->keys[0] << endl;
          }
        }
      }

      // update numnodes 
      numNodes = index->getAllocated();
    }

    // Inserts a record into a linked list. Returns the address of the new linked list head (if any).
    Address insertLL(Address LLHead, Address address, int key){
      // Load the linked list head node into main memory.
      BPNode *head = (BPNode *)index->loadFromDisk(LLHead, nodeSize);

      // Check if the head node has space to put record.
      if (head->numKeys < maxKeys)
      {

        // Move all keys back to insert at the head.
        for (int i = head->numKeys; i > 0; i--)
        {
          head->keys[i] = head->keys[i - 1];
        }

        // Move all pointers back to insert at the head.
        for (int i = head->numKeys + 1; i > 0; i--)

        {
          head->pointers[i] = head->pointers[i - 1];
        }

        // Insert new record into the head of linked list.
        head->keys[0] = key;
        head->pointers[0] = address; // the disk address of the key just inserted
        head->numKeys++;
        
        // Write head back to disk.
        LLHead = index->saveToDisk((void *)head, nodeSize, LLHead);

        // Return head address
        return LLHead;
      }
      // No space in head node, need a new linked list node.
      else
      {
        // Make a new node and add variables
        BPNode *LLNode = new BPNode(maxKeys);
        LLNode->isLeaf = false;
        LLNode->keys[0] = key;
        LLNode->numKeys = 1;

        // Insert key into head of linked list node.
        LLNode->pointers[0] = address;

        // Now this node is head of linked list, point to the previous head's disk address as next.
        LLNode->pointers[1] = LLHead;

        // Write new linked list node to disk.
        Address LLNodeAddress = index->saveToDisk((void *)LLNode, nodeSize);

        // Return disk address of new linked list head
        return LLNodeAddress;
      }
    }

    // Prints out the B+ Tree in the console.
    void display(Address rootStorageAddress, int level) {
      // Load in cursor from disk.
      //Address cursorMainMemoryAddress{cursorDiskAddress, 0};
      //Address cursorMainMemoryAddress;
      //cursorMainMemoryAddress.blockAddress = cursorDiskAddress;
      //cursorMainMemoryAddress.offset = 0;
      //BPNode *cursor = (BPNode *)index->loadFromDisk(cursorMainMemoryAddress, nodeSize);
      BPNode* cursor = (BPNode*) rootStorageAddress.blockAddress;

      // If tree exists, display all nodes.
      if (cursor != nullptr) {
        cout << "level " << level << ": ";
        for (int i = 0; i < level; i++) { cout << "  "; }

        displayNode(cursor);

        if (cursor->isLeaf != true) {
          for (int i = 0; i < cursor->numKeys + 1; i++) {
            // Load node in from disk to main memory.
            //BPNode *mainMemoryNode = (BPNode *)index->loadFromDisk(cursor->pointers[i], nodeSize);
            Address cursorStorageAddress;
            cursorStorageAddress.blockAddress = cursor->pointers[i].blockAddress;
            cursorStorageAddress.offset = 0;

            display(cursorStorageAddress, level + 1);
          }
        }
      }
    }

    // Prints out a specific node and its contents in the B+ Tree.
    void displayNode(BPNode* node) {
      // Print out all contents in the node as such |pointer|key|pointer|
      int i = 0;
      cout << node << " - |";
      for (int i = 0; i < node->numKeys; i++) {
        cout << static_cast<void*>(node->pointers[i].blockAddress) + node->pointers[i].offset << "|";
        cout << node->keys[i] << "|";
      }

      // Print last filled pointer
      if (node->pointers[node->numKeys].blockAddress == nullptr) {
        cout << "null|";
      } else {
        cout << node->pointers[node->numKeys].blockAddress << "|";
      }

      for (int i = node->numKeys; i < maxKeys; i++) {
        cout << "x|";      // Remaining empty keys
        cout << "null|";   // Remaining empty pointers
      }

      cout << endl;
    }

    // Prints out a data block and its contents in the disk.
    void displayBlock(void *blockAddress) {
      // Load block into memory
      void *block = operator new(nodeSize);
      std::memcpy(block, blockAddress, nodeSize);

      unsigned char testBlock[nodeSize];
      memset(testBlock, '\0', nodeSize);

      // Block is empty.
      if (memcmp(testBlock, block, nodeSize) == 0)
      {
        std::cout << "Empty block!" << '\n';
        return;
      }

      unsigned char *blockChar = (unsigned char *)block;

      int i = 0;
      while (i < nodeSize)
      {
        // Load each record
        void *recordAddress = operator new(sizeof(Record));
        std::memcpy(recordAddress, blockChar, sizeof(Record));

        Record *record = (Record *)recordAddress;

        std::cout << "[" << record->tconst << "|" << record->avgRating << "|" << record->numVotes << "]  ";
        blockChar += sizeof(Record);
        i += sizeof(Record);
      }
    }

    // Prints out an entire linked list's records.
    void displayLL(Address LLHeadAddress) {
      // Load linked list head into main memory.
      BPNode *head = (BPNode *)index->loadFromDisk(LLHeadAddress, nodeSize);

      // Print all records in the linked list.
      for (int i = 0; i < head->numKeys; i++)
      {
        // Load the block from disk.
        // void *blockMainMemoryAddress = operator new(nodeSize);
        // std::memcpy(blockMainMemoryAddress, head->pointers[i].blockAddress, nodeSize);

        std::cout << "\nData block accessed. Content is -----";
        displayBlock(head->pointers[i].blockAddress);
        std::cout << endl;

        Record result = *(Record *)(disk->loadFromDisk(head->pointers[i], sizeof(Record)));
        std::cout << result.tconst << " | ";


      }

      // Print empty slots
      for (int i = head->numKeys; i < maxKeys; i++)
      {
        std::cout << "x | ";
      }
      
      // End of linked list
      if (head->pointers[head->numKeys].blockAddress == nullptr)
      {
        std::cout << "End of linked list" << endl;
        return;
      }

      // Move to next node in linked list.
      if (head->pointers[head->numKeys].blockAddress != nullptr)
      {
        displayLL(head->pointers[head->numKeys]);
      }
    }

    // Remove a range of records from the disk (and B+ Tree).
    // Accepts a key to delete.
    int remove(int key){
      // set numNodes before deletion
      numNodes = index->getAllocated();

      // Tree is empty.
      if (rootAddress == nullptr)
      {
        throw std::logic_error("Tree is empty!");
      }
      else
      {
        // Load in root from the disk.
        //Address rootDiskAddress{rootAddress, 0};
        Address rootDiskAddress;
        rootDiskAddress.blockAddress = rootAddress;
        rootDiskAddress.offset = 0;
        root = (BPNode *)index->loadFromDisk(rootDiskAddress, nodeSize);

        BPNode *cursor = root;
        BPNode *parent;                          // Keep track of the parent as we go deeper into the tree in case we need to update it.
        void *parentDiskAddress = rootAddress; // Keep track of parent's disk address as well so we can update parent in disk.
        void *cursorDiskAddress = rootAddress; // Store current node's disk address in case we need to update it in disk.
        int leftSibling, rightSibling; // Index of left and right child to borrow from.

        // While not leaf, keep following the nodes to correct key.
        while (cursor->isLeaf == false)
        {
          // Set the parent of the node (in case we need to assign new child later), and its disk address.
          parent = cursor;
          parentDiskAddress = cursorDiskAddress;

          // Check through all keys of the node to find key and pointer to follow downwards.
          for (int i = 0; i < cursor->numKeys; i++)
          {
            // Keep track of left and right to borrow.
            leftSibling = i - 1;
            rightSibling = i + 1;

            // If key is lesser than current key, go to the left pointer's node.
            if (key < cursor->keys[i])
            {
              // Load node in from disk to main memory.
              BPNode *mainMemoryNode = (BPNode *)index->loadFromDisk(cursor->pointers[i], nodeSize);

              // Update cursorDiskAddress to maintain address in disk if we need to update nodes.
              cursorDiskAddress = cursor->pointers[i].blockAddress;

              // Move to new node in main memory.
              cursor = (BPNode *)mainMemoryNode;
              break;
            }
            // Else if key larger than all keys in the node, go to last pointer's node (rightmost).
            if (i == cursor->numKeys - 1)
            {
              leftSibling = i;
              rightSibling = i + 2;

              // Load node in from disk to main memory.
              BPNode *mainMemoryNode = (BPNode *)index->loadFromDisk(cursor->pointers[i + 1], nodeSize);

              // Update cursorDiskAddress to maintain address in disk if we need to update nodes.
              cursorDiskAddress = cursor->pointers[i + 1].blockAddress;

              // Move to new node in main memory.
              cursor = (BPNode *)mainMemoryNode;
              break;
            }
          }
        }

        // now that we have found the leaf node that might contain the key, we will try and find the position of the key here (if exists)
        // search if the key to be deleted exists in this bplustree
        bool found = false;
        int pos;
        // also works for duplicates
        for (pos = 0; pos < cursor->numKeys; pos++)
        {
          if (cursor->keys[pos] == key)
          {
            found = true;
            break;
          }
        }

        // If key to be deleted does not exist in the tree, return error.
        if (!found)
        {
          std::cout << "Can't find specified key " << key << " to delete!" << endl;
          
          // update numNodes and numNodesDeleted after deletion
          int numNodesDeleted = numNodes - index->getAllocated();
          numNodes = index->getAllocated();
          return numNodesDeleted;
        }

        // pos is the position where we found the key.
        // We must delete the entire linked-list before we delete the key, otherwise we lose access to the linked list head.
        // Delete the linked list stored under the key.
        removeLL(cursor->pointers[pos]);

        // Now, we can delete the key. Move all keys/pointers forward to replace its values.
        for (int i = pos; i < cursor->numKeys; i++)
        {
          cursor->keys[i] = cursor->keys[i + 1];
          cursor->pointers[i] = cursor->pointers[i + 1];
        }

        cursor->numKeys--;

        // // Change the key removed to empty float
        // for (int i = cursor->numKeys; i < maxKeys; i++) {
        //   cursor->keys[i] = float();
        // }

        // Move the last pointer forward (if any).
        cursor->pointers[cursor->numKeys] = cursor->pointers[cursor->numKeys + 1];

        // Set all forward pointers from numKeys onwards to nullptr.
        for (int i = cursor->numKeys + 1; i < maxKeys + 1; i++)
        {
          //Address nullAddress{nullptr, 0};
          Address nullAddress;
          nullAddress.blockAddress = nullptr;
          nullAddress.offset = 0;
          cursor->pointers[i] = nullAddress;
        }

        // If current node is root, check if tree still has keys.
        if (cursor == root)
        {
          if (cursor->numKeys == 0)
          {
            // Delete the entire root node and deallocate it.
            std::cout << "Congratulations! You deleted the entire index!" << endl;

            // Deallocate block used to store root node.
            //Address rootDiskAddress{rootAddress, 0};
            Address rootDiskAddress;
            rootDiskAddress.blockAddress = rootAddress;
            rootDiskAddress.offset = 0;
            index->deallocate(rootDiskAddress, nodeSize);

            // Reset root pointers in the B+ Tree.
            root = nullptr;
            rootAddress = nullptr;
            
          }
          std::cout << "Successfully deleted " << key << endl;
          
          // update numNodes and numNodesDeleted after deletion
          int numNodesDeleted = numNodes - index->getAllocated();
          numNodes = index->getAllocated();

          // Save to disk.
          Address cursorAddress = {cursorDiskAddress, 0};
          index->saveToDisk(cursor, nodeSize, cursorAddress);
          
          return numNodesDeleted;
        }

        // If we didn't delete from root, we check if we have minimum keys ⌊(n+1)/2⌋ for leaf.
        if (cursor->numKeys >= (maxKeys + 1) / 2)
        {
          // No underflow, so we're done.
          std::cout << "Successfully deleted " << key << endl;

          // update numNodes and numNodesDeleted after deletion
          int numNodesDeleted = numNodes - index->getAllocated();
          numNodes = index->getAllocated();

          // Save to disk.
          Address cursorAddress = {cursorDiskAddress, 0};
          index->saveToDisk(cursor, nodeSize, cursorAddress);

          return numNodesDeleted;
        }

        // If we reach here, means we have underflow (not enough keys for balanced tree).
        // Try to take from left sibling (node on same level) first.
        // Check if left sibling even exists.
        if (leftSibling >= 0)
        {
          // Load in left sibling from disk.
          BPNode *leftNode = (BPNode *)index->loadFromDisk(parent->pointers[leftSibling], nodeSize);

          // Check if we can steal (ahem, borrow) a key without underflow.
          if (leftNode->numKeys >= (maxKeys + 1) / 2 + 1)
          {
            // We will insert this borrowed key into the leftmost of current node (smaller).

            // Shift last pointer back by one first.
            cursor->pointers[cursor->numKeys + 1] = cursor->pointers[cursor->numKeys];

            // Shift all remaining keys and pointers back by one.
            for (int i = cursor->numKeys; i > 0; i--)
            {
              cursor->keys[i] = cursor->keys[i - 1];
              cursor->pointers[i] = cursor->pointers[i - 1];
            }

            // Transfer borrowed key and pointer (rightmost of left node) over to current node.
            cursor->keys[0] = leftNode->keys[leftNode->numKeys - 1];
            cursor->pointers[0] = leftNode->pointers[leftNode->numKeys - 1];
            cursor->numKeys++;
            leftNode->numKeys--;

            // Update left sibling (shift pointers left)
            leftNode->pointers[cursor->numKeys] = leftNode->pointers[cursor->numKeys + 1];

            // Update parent node's key
            parent->keys[leftSibling] = cursor->keys[0];

            // Save parent to disk.
            //Address parentAddress{parentDiskAddress, 0};
            Address parentAddress;
            parentAddress.blockAddress = parentDiskAddress;
            parentAddress.offset = 0;
            index->saveToDisk(parent, nodeSize, parentAddress);

            // Save left sibling to disk.
            index->saveToDisk(leftNode, nodeSize, parent->pointers[leftSibling]);

            // Save current node to disk.
            Address cursorAddress = {cursorDiskAddress, 0};
            index->saveToDisk(cursor, nodeSize, cursorAddress);
        
            // update numNodes and numNodesDeleted after deletion
            int numNodesDeleted = numNodes - index->getAllocated();
            numNodes = index->getAllocated();
            return numNodesDeleted;
          }
        }

        // If we can't take from the left sibling, take from the right.
        // Check if we even have a right sibling.
        if (rightSibling <= parent->numKeys)
        {
          // If we do, load in right sibling from disk.
          BPNode *rightNode = (BPNode *)index->loadFromDisk(parent->pointers[rightSibling], nodeSize);

          // Check if we can steal (ahem, borrow) a key without underflow.
          if (rightNode->numKeys >= (maxKeys + 1) / 2 + 1)
          {

            // We will insert this borrowed key into the rightmost of current node (larger).
            // Shift last pointer back by one first.
            cursor->pointers[cursor->numKeys + 1] = cursor->pointers[cursor->numKeys];

            // No need to shift remaining pointers and keys since we are inserting on the rightmost.
            // Transfer borrowed key and pointer (leftmost of right node) over to rightmost of current node.
            cursor->keys[cursor->numKeys] = rightNode->keys[0];
            cursor->pointers[cursor->numKeys] = rightNode->pointers[0];
            cursor->numKeys++;
            rightNode->numKeys--;

            // Update right sibling (shift keys and pointers left)
            for (int i = 0; i < rightNode->numKeys; i++)
            {
              rightNode->keys[i] = rightNode->keys[i + 1];
              rightNode->pointers[i] = rightNode->pointers[i + 1];
            }

            // Move right sibling's last pointer left by one too.
            rightNode->pointers[cursor->numKeys] = rightNode->pointers[cursor->numKeys + 1];

            // Update parent node's key to be new lower bound of right sibling.
            parent->keys[rightSibling - 1] = rightNode->keys[0];

            // Save parent to disk.
            //Address parentAddress{parentDiskAddress, 0};
            Address parentAddress;
            parentAddress.blockAddress = parentDiskAddress;
            parentAddress.offset = 0;
            index->saveToDisk(parent, nodeSize, parentAddress);

            // Save right sibling to disk.
            index->saveToDisk(rightNode, nodeSize, parent->pointers[rightSibling]);

            // Save current node to disk.
            Address cursorAddress = {cursorDiskAddress, 0};
            index->saveToDisk(cursor, nodeSize, cursorAddress);

            // update numNodes and numNodesDeleted after deletion
            int numNodesDeleted = numNodes - index->getAllocated();
            numNodes = index->getAllocated();
            return numNodesDeleted;        
          }
        }

        // If we reach here, means no sibling we can steal from.
        // To resolve underflow, we must merge nodes.

        // If left sibling exists, merge with it.
        if (leftSibling >= 0)
        {
          // Load in left sibling from disk.
          BPNode *leftNode = (BPNode *)index->loadFromDisk(parent->pointers[leftSibling], nodeSize);

          // Transfer all keys and pointers from current node to left node.
          // Note: Merging will always suceed due to ⌊(n)/2⌋ (left) + ⌊(n-1)/2⌋ (current).
          for (int i = leftNode->numKeys, j = 0; j < cursor->numKeys; i++, j++)
          {
            leftNode->keys[i] = cursor->keys[j];
            leftNode->pointers[i] = cursor->pointers[j];
          }

          // Update variables, make left node last pointer point to the next leaf node pointed to by current.
          leftNode->numKeys += cursor->numKeys;
          leftNode->pointers[leftNode->numKeys] = cursor->pointers[cursor->numKeys];

          // Save left node to disk.
          index->saveToDisk(leftNode, nodeSize, parent->pointers[leftSibling]);

          // We need to update the parent in order to fully remove the current node.
          removeInternal(parent->keys[leftSibling], (BPNode *)parentDiskAddress, (BPNode *)cursorDiskAddress);

          // Now that we have updated parent, we can just delete the current node from disk.
          //Address cursorAddress{cursorDiskAddress, 0};
          Address cursorAddress;
          cursorAddress.blockAddress = cursorDiskAddress;
          cursorAddress.offset = 0;
          index->deallocate(cursorAddress, nodeSize);
        }
        // If left sibling doesn't exist, try to merge with right sibling.
        else if (rightSibling <= parent->numKeys)
        {
          // Load in right sibling from disk.
          BPNode *rightNode = (BPNode *)index->loadFromDisk(parent->pointers[rightSibling], nodeSize);

          // Note we are moving right node's stuff into ours.
          // Transfer all keys and pointers from right node into current.
          // Note: Merging will always suceed due to ⌊(n)/2⌋ (left) + ⌊(n-1)/2⌋ (current).
          for (int i = cursor->numKeys, j = 0; j < rightNode->numKeys; i++, j++)
          {
            cursor->keys[i] = rightNode->keys[j];
            cursor->pointers[i] = rightNode->pointers[j];
          }

          // Update variables, make current node last pointer point to the next leaf node pointed to by right node.
          cursor->numKeys += rightNode->numKeys;
          cursor->pointers[cursor->numKeys] = rightNode->pointers[rightNode->numKeys];

          // Save current node to disk.
          //Address cursorAddress{cursorDiskAddress, 0};
          Address cursorAddress;
          cursorAddress.blockAddress = cursorDiskAddress;
          cursorAddress.offset = 0;
          index->saveToDisk(cursor, nodeSize, cursorAddress);

          // We need to update the parent in order to fully remove the right node.
          void *rightNodeAddress = parent->pointers[rightSibling].blockAddress;
          removeInternal(parent->keys[rightSibling - 1], (BPNode *)parentDiskAddress, (BPNode *)rightNodeAddress);

          // Now that we have updated parent, we can just delete the right node from disk.
          //Address rightNodeDiskAddress{rightNodeAddress, 0};
          Address rightNodeDiskAddress;
          rightNodeDiskAddress.blockAddress = rightNodeAddress;
          rightNodeDiskAddress.offset = 0;
          index->deallocate(rightNodeDiskAddress, nodeSize);
        }
      }

      // update numNodes and numNodesDeleted after deletion
      int numNodesDeleted = numNodes - index->getAllocated();
      numNodes = index->getAllocated();
      return numNodesDeleted;
    }

    // Remove an entire linked list from the start to the end for a given linked list head
    void removeLL(Address LLHeadAddress) {
      // Load in first node from disk.
      BPNode *head = (BPNode *)index->loadFromDisk(LLHeadAddress, nodeSize);

      // Removing the current head. Simply deallocate the entire block since it is safe to do so for the linked list
      // Keep going down the list until no more nodes to deallocate.

      // Deallocate the current node.
      index->deallocate(LLHeadAddress, nodeSize);

      // End of linked list
      if (head->pointers[head->numKeys].blockAddress == nullptr)
      {
        std::cout << "End of linked list";
        return;
      }

      if (head->pointers[head->numKeys].blockAddress != nullptr)
      {

        removeLL(head->pointers[head->numKeys]);
      }
    }

    // Getters and setters
    // Returns a pointer to the root of the B+ Tree.
    BPNode* getRoot() { return root; };

    Address getRootStorageAddress() { return rootStorageAddress; };

    // Returns the number of levels in this B+ Tree.
    int getLevels() {
      if (rootStorageAddress.blockAddress == nullptr) { return 0; }

      // Load in the root node from disk
      //Address rootDiskAddress{rootAddress, 0};
      //Address rootDiskAddress;
      //rootDiskAddress.blockAddress = rootAddress;
      //rootDiskAddress.offset = 0;
      //root = (BPNode*) index->loadFromDisk(rootDiskAddress, nodeSize);
      root = (BPNode*) rootStorageAddress.blockAddress;
      BPNode* cursor = root;

      levels = 1;

      while (!cursor->isLeaf) {
        //cursor = (BPNode *)index->loadFromDisk(cursor->pointers[0], nodeSize);
        cursor = (BPNode*) cursor->pointers[0].blockAddress;
        levels++;
      }

      // Account for linked list (count as one level)
      levels++;

      return levels;
    }

    int getNumNodes() { return numNodes; }

    int getMaxKeys() { return maxKeys; }

    void search2(int lowerBoundKey, int upperBoundKey){
        cout << "B+Tree root: " << rootStorageAddress.blockAddress << endl;
        BPNode* cursor = (BPNode*) rootStorageAddress.blockAddress; //current target in B+Tree

        cout << "isLeaf: " << cursor->isLeaf << endl; //any non-zero value to bool is true

        if (cursor != nullptr){

          //Check Not Leaf Node
          while(!cursor->isLeaf)
          {
            for (int i = 0; i < cursor->numKeys; i++){
              int key = getCursorKey(cursor,i);

              cout << "Key Value: " << key << endl;
              cout << "Key getKeysCount(): " << cursor->getKeysCount() << endl;

              if (lowerBoundKey < key){
                  cout << " go left "<< endl;
                  cursor = (BPNode *)index->loadFromDisk(cursor->pointers[i], nodeSize);

                  printCurrentPointer(cursor,i);
                  //displayNode(cursor);
                  break;
              }

              if(cursor->getKeysCount()-1 == i){
                cout << "at last key "<< endl;
                cursor = (BPNode *)index->loadFromDisk(cursor->pointers[i + 1], nodeSize);
                displayNode(cursor);
                printCurrentPointer(cursor,i);
                break;
              }
            }
          }

          cout << "End of isLeaf == False while loop "<< endl;
          //displayNode(cursor);
          cout << "Key getKeysCount(): " << cursor->getKeysCount() << endl;

          bool flag = false;
          while(!flag){
            int i;
            for (i = 0; i < cursor->getKeysCount(); i++){
              int key = getCursorKey(cursor,i);
              if (key > upperBoundKey)
              {
                flag = true;
                cout << "key > upperBoundKey "<< endl;
                break;
              }
              else if(key >= lowerBoundKey && key<= upperBoundKey){
                cout << "Found key(low,upper) at [i]="<< i << endl;
              
                printCurrentPointer(cursor,i);
                printCurrentPointerWithOffset(cursor,i); //This pointing to the Key 

                //auto mg = getNewPtr(cursor,i);


                // Address testOffSet;
                // testOffSet.blockAddress = cursor->pointers[i].blockAddress + cursor->pointers[i].offset;
                // testOffSet.offset = 0;

                // BPNode *test = (BPNode *)index->loadFromDisk(testOffSet, nodeSize);


                
                //displayBlock(blockAddr);  //24/9/22 Last debug here 2:16am
                // displayBlock(cursor->pointers[i].blockAddress);

                //25.9.22 1:47pm testing displayLL
                //displayLL(cursor->pointers[i]);

                Address testA;

                //test1: raw
                // testA.blockAddress = cursor->pointers[i].blockAddress;
                // testA.offset = cursor->pointers[i].offset;

                //test2: blockAddress + offset (tested > OK)
                testA.blockAddress = cursor->pointers[i].blockAddress + cursor->pointers[i].offset;
                testA.offset = 0;
                cout << "Record blockAddr: "<< testA.blockAddress << endl;

                //moved into displayLL2()
                // void *recordAddress = operator new(sizeof(Record));
                // std::memcpy(recordAddress, testA.blockAddress, sizeof(Record));

                // Record *record = (Record *)recordAddress;
                // cout << "======="<< endl;
                // cout << "tconst: "<< record->tconst << endl;
                // cout << "avgRating: "<< record->avgRating << endl;
                // cout << "numVotes: "<< record->numVotes << endl;
                // cout << "======="<< endl;
                //end


                displayLL2(testA);
                //displayLL2(cursor->pointers[i]);  //i: upperBoundKey's Index from Node
                

                //test we take with OffSet Addr, and get the Block details
              

                displayNode(cursor);
                
                flag = true;
                break;
              }
            }

            // if (cursor->pointers[cursor->numKeys].blockAddress != nullptr && cursor->keys[i] != upperBoundKey)
            //   {
            //     // Set cursor to be next leaf node (load from disk).
            //     cursor = (BPNode *)index->loadFromDisk(cursor->pointers[cursor->numKeys], nodeSize);
            //     displayNode(cursor);
            //   }
            //   else
            //   {
            //     flag = true;
            //   }
          }
        }

        auto m = 0;

    }

    //Display Key Value from Cursor :WJ
    int getCursorKey(BPNode *cursor,int i) { 
      return cursor->keys[i]; 
    }

    //refactor displayLL2 :WJ (pending 24.9.22)
    void displayLL2(Address LLHeadAddress){
      cout << "entering displayLL2()....."<<endl;

      void *recordAddress = operator new(sizeof(Record));
      std::memcpy(recordAddress, LLHeadAddress.blockAddress, sizeof(Record));

      Record *record = (Record *)recordAddress;
      cout << "==== Record Details ===="<< endl;
      cout << "tconst: "<< record->tconst << endl;
      cout << "avgRating: "<< record->avgRating << endl;
      cout << "numVotes: "<< record->numVotes << endl;
      cout << "====================="<< endl;


      cout << "displaying Key's LinkedList"<<endl;
      cout << "coming soon...."<<endl;

      BPNode *head = (BPNode *)index->loadFromDisk(LLHeadAddress, nodeSize);
      auto m = 0;

      // if(head->isLeaf){
      //    cout << "numNodes: " << numNodes <<endl;
      // }

      for (int i = 0; i < head->numKeys; i++){
        //head->pointers[i].blockAddress
        auto m = 0;
      }
    }

    //Print Current Cursor's blockAddress :WJ
    void printCurrentPointer (BPNode *cursor,int i){
      auto blockAddress = cursor->pointers[i].blockAddress;
      cout << "printCurrentPointer: "<< blockAddress << endl;
    }

    //Print Current Cursor's blockAddress with Offset :WJ
    void printCurrentPointerWithOffset (BPNode *cursor,int i){
      auto curPtr = cursor->pointers[i];
      auto addr = curPtr.blockAddress + curPtr.offset;
      cout << "printCurrentPointerWithOffset: "<< addr << endl;
    }

    void *getNewPtr(BPNode *cursor,int i){
      auto curPtr = cursor->pointers[i];
      auto addr = curPtr.blockAddress + curPtr.offset;
      return addr;
    }
};