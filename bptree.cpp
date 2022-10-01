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
    Address* pointers;      // A pointer to an array of struct {void *blockAddress, short int offset} containing other nodes in disk.
    int* keys;            // Pointer to an array of keys in this node.
    int numKeys;            // Current number of keys in this node.
    bool isLeaf;            // Whether this node is a leaf node.
    
    BPNode(int maxKeys){
      // Initialize empty array of keys and pointers.
      keys = new int[maxKeys];
      pointers = new Address[maxKeys + 1];

      for (int i = 0; i < maxKeys + 1; i++) {
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
    DiskStorage* disk;     // Pointer to a memory pool for data blocks.
    BPNode* root;           // Pointer to the main memory root (if it's loaded).
    Address rootStorageAddress;
    int maxKeys;          // Maximum keys in a node.
    int levels;           // Number of levels in this B+ Tree.
    int numNodes;         // Number of nodes in this B+ Tree.
    size_t nodeSize; // Size of a node = Size of block.

    // Updates the parent node to point at both child nodes, and adds a parent node if needed.
    void insertInternal(int key, Address parentStorageAddress, Address newLeafStorageAddress) {
      // Load in cursor (parent) and child from disk to get latest copy.
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
        numNodes++;
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
          numNodes++;
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
      BPNode* cursor = (BPNode *)index->loadFromDisk(cursorAddress, nodeSize);

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

    void removeInternal2(int target, BPNode* parentNode, BPNode* childNode) {
      //parent(cursor)
      cout << "remove Internal2()...." << endl;
      cout << "parentNode:" << parentNode << endl;
      displayNode(parentNode);
      cout << "childNode:" << childNode << endl;
      displayNode(childNode);
      cout << "root:" << root << endl;

      BPNode* cursor = parentNode;
      
      cout << "cursor->numKeys:" << cursor->numKeys << endl;
      cout << "identify parent Node:" << endl;

      if(cursor == rootStorageAddress.blockAddress) {
        if(cursor->numKeys == 1){
          if (cursor->pointers[1].blockAddress == childNode){
            Address updateRoot;
            updateRoot.blockAddress = cursor->pointers[0].blockAddress;
            updateRoot.offset = 0;

            root = (BPNode*) updateRoot.blockAddress;
            rootStorageAddress = updateRoot;
            
            cout << "Updated root from right" << endl;
            return;
          } else if(cursor->pointers[0].blockAddress == childNode){
            Address updateRoot;
            updateRoot.blockAddress = cursor->pointers[1].blockAddress;
            updateRoot.offset = 0;

            root = (BPNode*) updateRoot.blockAddress;
            rootStorageAddress = updateRoot;

            cout << "Update root from left" << endl;
            return;
          }
        }
      }

      //Parent is not root, find position of target to delete
      int position;
      for(position = 0; position < cursor->numKeys; position++) {
        if(cursor->keys[position] == target) {
          break;
        }
      }

      //Shift everything infront
      for(int i = position; i < cursor->numKeys; i++) {
        cursor->keys[i] = cursor->keys[i + 1];
      }

      for(position = 0; position < cursor->numKeys + 1; position++) {
        if(cursor->pointers[position].blockAddress == childNode) {
          break;
        }
      }

      for(int i = position; i < cursor->numKeys + 1; i++) {
        cursor->pointers[i] = cursor->pointers[i + 1];
      }

      cursor->numKeys--;

      if(cursor->numKeys >= (maxKeys + 1) / 2 - 1) {
        return;
      }

      if(cursor == root) {
        return;
      }

      int leftsibling, rightsibling;
      Address parentdiskadd;
      parentdiskadd.blockAddress = parentNode;
      parentdiskadd.offset = 0;

      Address parentparentnodeadd = findParent(cursor->keys[0], rootStorageAddress, parentdiskadd);
      BPNode* parentparentnode = (BPNode*) parentparentnodeadd.blockAddress;

      for(position = 0; position < parentparentnode->numKeys + 1; position++) {
        if(parentparentnode->pointers[position].blockAddress == parentNode) {
          leftsibling = position - 1;
          rightsibling = position + 1;
          break;
        }
      }

      if(leftsibling >= 0){
        BPNode* leftnode = (BPNode*) parentparentnode->pointers[leftsibling].blockAddress;

        if(leftnode->numKeys >= (maxKeys + 1) / 2){
          for(int i = cursor->numKeys; i > 0; i--){
            cursor->keys[i] = cursor->keys[i - 1];
          }

          cursor->keys[0] = parentparentnode->keys[leftsibling];
          parentparentnode->keys[leftsibling] = leftnode->keys[leftnode->numKeys - 1];

          for(int i = cursor->numKeys + 1; i > 0; i--){
            cursor->pointers[i] = cursor->pointers[i - 1];
          }

          cursor->pointers[0] = leftnode->pointers[leftnode->numKeys];

          cursor->numKeys++;
          leftnode->numKeys++;

          leftnode->pointers[cursor->numKeys] = leftnode->pointers[cursor->numKeys + 1];
          return;
        }
      }

      if(rightsibling <= parentparentnode->numKeys){
        BPNode* rightnode = (BPNode*) parentparentnode->pointers[rightsibling].blockAddress;

        if(rightnode->numKeys >= (maxKeys + 1) / 2){
          cursor->keys[cursor->numKeys] = parentparentnode->keys[position];
          parentparentnode->keys[position] = rightnode->keys[0];

          for(int i = 0; i < rightnode->numKeys - 1; i++){
            rightnode->keys[i] = rightnode->keys[i + 1];
          }
          
          cursor->pointers[cursor->numKeys + 1] = rightnode->pointers[0];

          for(int i = 0; i < rightnode->numKeys; i++){
            rightnode->pointers[i] = rightnode->pointers[i + 1];
          }

          cursor->numKeys++;
          rightnode->numKeys--;
          return;
        }
      }

      //No sibiling to steal, merge
      if(leftsibling >= 0){
        BPNode* leftnode = (BPNode*) parentparentnode->pointers[leftsibling].blockAddress;

        leftnode->keys[leftnode->numKeys] = parentparentnode->keys[leftsibling];

        int j;
        for (int i = leftnode->numKeys + 1, j = 0; j < cursor->numKeys; j++){
          leftnode->keys[i] = cursor->keys[j];
        }

        Address nulladdress;
        nulladdress.blockAddress = nullptr;
        nulladdress.offset = 0;

        for (int i = leftnode->numKeys + 1, j = 0; j < cursor->numKeys + 1; j++){
          leftnode->pointers[i] = cursor->pointers[j];
          cursor->pointers[j] = nulladdress;
        }

        leftnode->numKeys += cursor->numKeys + 1;
        cursor->numKeys = 0;

        removeInternal2(parentparentnode->keys[leftsibling], parentparentnode, parentNode);
      } else if(rightsibling <= parentparentnode->numKeys){
        BPNode* rightnode = (BPNode*) parentparentnode->pointers[rightsibling].blockAddress;

        cursor->keys[cursor->numKeys] = parentparentnode->keys[rightsibling - 1];

        for(int i = cursor->numKeys + 1, j = 0; j < rightnode->numKeys; j++){
          cursor->keys[i] = rightnode->keys[j];
        }

        Address nulladdress;
        nulladdress.blockAddress = nullptr;
        nulladdress.offset = 0;

        for(int i = cursor->numKeys + 1, j = 0; j < rightnode->numKeys + 1; j++){
          cursor->pointers[i] = rightnode->pointers[j];
          rightnode->pointers[j] = nulladdress;
        }

        cursor->numKeys += rightnode->numKeys + 1;
        rightnode->numKeys = 0;

        removeInternal2(parentparentnode->keys[rightsibling - 1], parentNode, rightnode);
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
    BPlusTree(DiskStorage* disk, size_t blockSize){
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
        throw overflow_error("Error: Keys and pointers too large to fit into a node!");
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

      this->disk = disk;
    }

    // Inserts a record into the B+ Tree.
    void insert(Address recordaddress, int key){
      BPNode* root = (BPNode*) rootStorageAddress.blockAddress;
      // If no root exists, create a new B+ Tree root.
      if (root == nullptr) {
        // Create a new linked list (for duplicates) at the key.
        LLNode* llnode = new LLNode();
        llnode->dataaddress.blockAddress = recordaddress.blockAddress;
        llnode->dataaddress.offset = recordaddress.offset;
        llnode->next = nullptr;

        Address llnodeaddress;
        llnodeaddress.blockAddress = llnode;
        llnodeaddress.offset = 0;
        
        // Create new node in main memory, set it to root, and add the key and values to it.
        root = new BPNode(maxKeys);
        numNodes++;
        root->keys[0] = key;
        root->isLeaf = true; // It is both the root and a leaf.
        root->numKeys = 1;
        root->pointers[0] = llnodeaddress; // Add record's disk address to pointer.

        rootStorageAddress.blockAddress = root;
        rootStorageAddress.offset = 0;
      } else { // Else if root exists already, traverse the nodes to find the proper place to insert the key.
        BPNode* cursor = root;
        BPNode* parent = nullptr;                          // Keep track of the parent as we go deeper into the tree in case we need to update it.
        Address parentDiskAddress; // Keep track of parent's disk address as well so we can update parent in disk.
        parentDiskAddress.blockAddress = nullptr;
        parentDiskAddress.offset = 0;
        Address cursorDiskAddress; // Store current node's disk address in case we need to update it in disk.
        cursorDiskAddress.blockAddress = root;
        cursorDiskAddress.offset = 0;

        int level = 0;
        //cout << "Started going to leaf node" << endl;
        while (cursor->isLeaf == false) { // While not leaf, keep following the nodes to correct key.
          parent = cursor;
          parentDiskAddress.blockAddress = parent;

          // Check through all keys of the node to find key and pointer to follow downwards.
          for (int i = 0; i < cursor->numKeys; i++) {
            // If key is lesser than current key, go to the left pointer's node.

            if (key < cursor->keys[i]) {
              cursor = (BPNode*) cursor->pointers[i].blockAddress;
              cursorDiskAddress.blockAddress = cursor;
              break;
            }
            
            //cout << "Checking if larger than all other keys: " << i << " == " << cursor->numKeys - 1 << endl;
            if (i == cursor->numKeys - 1) { // Else if key larger than all keys in the node, go to last pointer's node (rightmost).
              cursor = (BPNode*) cursor->pointers[i + 1].blockAddress;
              cursorDiskAddress.blockAddress = cursor;
              break;
            }
          }

          level++;
        }

        // When we reach here, it means we have hit a leaf node. Let's find a place to put our new record in.
        // If this leaf node still has space to insert a key, then find out where to put it.
        if (cursor->numKeys < maxKeys) {
          int i = 0;
          
          while (key > cursor->keys[i] && i < cursor->numKeys) { // While we haven't reached the last key and the key we want to insert is larger than current key, keep moving forward.
            //cout << "Search left to right: " << key << " > " << cursor->keys[i] << " & " << i << " < " << cursor->numKeys << endl;
            i++;
          }

          // i is where our key goes in. Check if it's already there (duplicate).
          if (cursor->keys[i] == key) {
            // Create a new linked list (for duplicates) at the key.
            Address llnodeaddress;
            llnodeaddress.blockAddress = cursor->pointers[i].blockAddress;
            llnodeaddress.offset = cursor->pointers[i].offset;
            
            LLNode* prenode = (LLNode*) llnodeaddress.blockAddress + llnodeaddress.offset;
            LLNode* curnode = new LLNode();
            curnode->dataaddress.blockAddress = recordaddress.blockAddress;
            curnode->dataaddress.offset = recordaddress.offset;
            curnode->next = nullptr;
            
            while(prenode->next != nullptr){
              prenode = prenode->next;
            }

            prenode->next = curnode;
          } else {
            // Update the last pointer to point to the previous last pointer's node. Aka maintain cursor -> Y linked list.
            Address next = cursor->pointers[cursor->numKeys];

            // Now i represents the index we want to put our key in. We need to shift all keys in the node back to fit it in.
            // Swap from number of keys + 1 (empty key) backwards, moving our last key back and so on. We also need to swap pointers.
            for (int j = cursor->numKeys; j > i; j--) {
              // Just do a simple bubble swap from the back to preserve index order.
              cursor->keys[j] = cursor->keys[j - 1];
              cursor->pointers[j] = cursor->pointers[j - 1];
            }

            // Insert our new key and pointer into this node.
            cursor->keys[i] = key;

            // We need to make a new linked list to store our record.
            // Create a new linked list (for duplicates) at the key.
            LLNode* llnode = new LLNode();
            llnode->dataaddress.blockAddress = recordaddress.blockAddress;
            llnode->dataaddress.offset = recordaddress.offset;
            llnode->next = nullptr;
            
            Address llnodeaddress;
            llnodeaddress.blockAddress = llnode;
            llnodeaddress.offset = 0;
            
            // Update variables
            cursor->pointers[i] = llnodeaddress;
            cursor->numKeys++;
            cursor->pointers[cursor->numKeys] = next;
            }
        } else { // Overflow: If there's no space to insert new key, we have to split this node into two and update the parent if required.
          // Create a new leaf node to put half the keys and pointers in.
          BPNode* newLeaf = new BPNode(maxKeys);
          numNodes++;

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

          if (i < cursor->numKeys) {
            if (cursor->keys[i] == key) {
              cout << "Duplicate Overflow: " << cursor->keys[i] << endl;

              Address llnodeaddress;
              llnodeaddress.blockAddress = cursor->pointers[i].blockAddress;
              llnodeaddress.offset = cursor->pointers[i].offset;

              LLNode* prenode = (LLNode*) llnodeaddress.blockAddress + llnodeaddress.offset;
              LLNode* curnode = new LLNode();
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

          for (int j = maxKeys; j > i; j--) {
            // Bubble swap all elements (keys and pointers) backwards by one index.
            tempKeyList[j] = tempKeyList[j - 1];
            tempPointerList[j] = tempPointerList[j - 1];
          }

          // Insert the new key and pointer into the temporary lists.
          tempKeyList[i] = key;

          //To handle duplicate
          LLNode* llnode = new LLNode();
          llnode->dataaddress.blockAddress = recordaddress.blockAddress;
          llnode->dataaddress.offset = recordaddress.offset;
          llnode->next = nullptr;
          
          Address llnodeaddress;
          llnodeaddress.blockAddress = llnode;
          llnodeaddress.offset = 0;
          
          // Allocate LLNode into disk.
          tempPointerList[i] = llnodeaddress;
          newLeaf->isLeaf = true; // New node is a leaf node.
          cursor->numKeys = ceil((maxKeys + 1) / 2.0);
          newLeaf->numKeys = floor((maxKeys + 1) / 2.0);
          newLeaf->pointers[newLeaf->numKeys] = next;
          
          for (i = 0; i < cursor->numKeys; i++) {
            cursor->keys[i] = tempKeyList[i];
            cursor->pointers[i] = tempPointerList[i];
          }

          // Then, the new leaf node. Note we keep track of the i index, since we are using the remaining keys and pointers.
          for (int j = 0; j < newLeaf->numKeys; i++, j++) {
            newLeaf->keys[j] = tempKeyList[i];
            newLeaf->pointers[j] = tempPointerList[i];
          }

          // Now that we have finished updating the two new leaf nodes, we need to write them to disk.
          Address newLeafStorageAddress;
          newLeafStorageAddress.blockAddress = newLeaf;
          newLeafStorageAddress.offset = 0;
          
          // Now to set the cursors' pointer to the disk address of the leaf and save it in place
          cursor->pointers[cursor->numKeys] = newLeafStorageAddress;

          // wipe out the wrong pointers and keys from cursor
          for (int i = cursor->numKeys; i < maxKeys; i++) {
            cursor->keys[i] = 0;
          }

          for (int i = cursor->numKeys + 1; i < maxKeys + 1; i++) {
            Address nullAddress;
            nullAddress.blockAddress = nullptr;
            nullAddress.offset = 0;

            cursor->pointers[i] = nullAddress;
          }

          // If we are at root (aka root == leaf), then we need to make a new parent root.
          if (cursor == root) {
            BPNode* newRoot = new BPNode(maxKeys);
            numNodes++;
            newRoot->keys[0] = newLeaf->keys[0];
            newRoot->pointers[0] = cursorDiskAddress;
            newRoot->pointers[1] = newLeafStorageAddress;
            newRoot->isLeaf = false;
            newRoot->numKeys = 1;

            root = newRoot;
            
            rootStorageAddress.blockAddress = root;
            rootStorageAddress.offset = 0;
          } else { // If we are not at the root, we need to insert a new parent in the middle levels of the tree.
            insertInternal(newLeaf->keys[0], parentDiskAddress, newLeafStorageAddress);
          }
        }
      }
    }

    tuple<int,int> search(int lowerBoundKey, int upperBoundKey) {
      //cout << "B+Tree root: " << rootStorageAddress.blockAddress << endl;
      BPNode* cursor = (BPNode*) rootStorageAddress.blockAddress; //current target in B+Tree
      int displaycount = 0;
      int indexblockproc = 0;
      int recordblockproc = 0;
      int totalrecordfound = 0;
      float totalavgrating = 0.0;

      if (cursor != nullptr) {
        //WHen its not Leaf Node
        //cout << "isLeaf: " << cursor->isLeaf << endl; //any non-zero value to bool is true
        while(!cursor->isLeaf) {
          //Loop through all the keys in current Node
          if(displaycount < 5) {
            cout << "Index block: ";
            displayNode(cursor);
            displaycount++;
          }

          for (int i = 0; i < cursor->numKeys; i++){
            int key = getCursorKey(cursor, i);
            //cout << "Accessing " << i << " Key: " << key << endl;
            //cout << "Total keys in current Node: " << cursor->getKeysCount() << endl;

            if (lowerBoundKey < key){
                cursor = (BPNode*) cursor->pointers[i].blockAddress;
                break;
            }

            //When we reached at last key, we switch to Right Ptr (using i+1) and continue searching 
            if(cursor->getKeysCount() - 1 == i){
              cursor = (BPNode*) cursor->pointers[i + 1].blockAddress;
              break;
            }
          }

          //cout << "isLeaf: " << cursor->isLeaf << endl; //any non-zero value to bool is true
          indexblockproc++;
        }

        //This stage: indicated we have reached Leaf Node
        //Next step: loop through leaf node keys to match target
        //cout << "End of isLeaf == False while loop "<< endl;
        //displayNode(cursor);
        //cout << "Key getKeysCount(): " << cursor->getKeysCount() << endl;

        displaycount = 0;
      
        bool flag = false;
        while(!flag){
          if(displaycount < 5) {
            cout << "Index block: ";
            displayNode(cursor);
            displaycount++;
          }

          int i;
          for (i = 0; i < cursor->getKeysCount(); i++){
            int key = getCursorKey(cursor,i);
            if (key > upperBoundKey) {
              flag = true;
              cout << "Avg of AvgRating: " << (totalavgrating / totalrecordfound) << endl;
              cout << "Searching completed."<< endl;
              break;
            }
            
            if (key >= lowerBoundKey && key <= upperBoundKey){
              float avgrating = 0.0;
              int recordfound = 0;
              //cout << "Found key: " << key << " between " << lowerBoundKey << " & " << upperBoundKey << endl;

              Address llnodeaddress;
              llnodeaddress.blockAddress = cursor->pointers[i].blockAddress;
              llnodeaddress.offset = cursor->pointers[i].offset;

              tie(recordfound, avgrating) = displayLL(llnodeaddress);
              totalavgrating += avgrating;
              totalrecordfound += recordfound;
              recordblockproc += recordfound;
            }
          }

          //cout << "cursor->pointers[cursor->numKeys].blockAddress: " << cursor->pointers[cursor->numKeys].blockAddress << endl;
          if(cursor->pointers[cursor->numKeys].blockAddress != nullptr && cursor->keys[i] != upperBoundKey){
            cursor = (BPNode*) cursor->pointers[cursor->numKeys].blockAddress;
            indexblockproc++;
          } else {
            flag = true;
          }
        }
      } else {
        cout << "No tree created!" << endl;
      }

      return make_tuple(indexblockproc, recordblockproc);
    }

    // Prints out the B+ Tree in the console.
    void display(Address rootStorageAddress, int level) {
      BPNode* cursor = (BPNode*) rootStorageAddress.blockAddress;

      // If tree exists, display all nodes.
      if (cursor != nullptr) {
        cout << "level " << level << ": ";
        for (int i = 0; i < level; i++) { cout << "  "; }

        displayNode(cursor);

        if (cursor->isLeaf != true) {
          for (int i = 0; i < cursor->numKeys + 1; i++) {
            // Load node in from disk to main memory.
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
        if(node->isLeaf == true){
          int z = 0;
          int curkey = node->keys[i];

          Address llnodeaddress;
          llnodeaddress.blockAddress = node->pointers[i].blockAddress;
          llnodeaddress.offset = node->pointers[i].offset;

          LLNode* prenode = (LLNode*) llnodeaddress.blockAddress + llnodeaddress.offset;
          z++;
          while(prenode->next != nullptr){
            z++;
            prenode = prenode->next;
          }

          cout << static_cast<void*>(node->pointers[i].blockAddress) << " + " << node->pointers[i].offset << "|";
          cout << node->keys[i] << "-" << z << "|";
        } else {
          cout << static_cast<void*>(node->pointers[i].blockAddress) << " + " << node->pointers[i].offset << "|";
          cout << node->keys[i] << "|";
        }
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

    //refactor displayLL2 :WJ (pending 24.9.22)
    tuple<int, float> displayLL(Address LLHeadAddress){
      float totalavgrating = 0.0;
      int totalrecords = 0;
      LLNode* cursor = (LLNode*) LLHeadAddress.blockAddress + LLHeadAddress.offset;

      while(cursor->next != nullptr){
        Address recordAddress;
        recordAddress.blockAddress = cursor->dataaddress.blockAddress;
        recordAddress.offset = cursor->dataaddress.offset;

        Record* currecord = (Record*) disk->retrieveDataFromDisk(recordAddress, sizeof(Record));
        totalavgrating += currecord->avgRating;
        totalrecords++;
        cout << currecord->tconst << "  " << currecord->avgRating << "  " << currecord->numVotes << endl;
        cursor = cursor->next;
      }

      return make_tuple(totalrecords, totalavgrating);
    }

    int getMaxKeys() { return maxKeys; }

    int getNumNodes() { return numNodes; }

    int getBPTreeLevel(Address rootStorageAddress, int level) {
      BPNode* cursor = (BPNode*) rootStorageAddress.blockAddress;

      // If tree exists, display all nodes.
      if (cursor != nullptr) {
        if (cursor->isLeaf != true) {
          for (int i = 0; i < cursor->numKeys + 1; i++) {
            // Load node in from disk to main memory.
            //BPNode *mainMemoryNode = (BPNode *)index->loadFromDisk(cursor->pointers[i], nodeSize);
            Address cursorStorageAddress;
            cursorStorageAddress.blockAddress = cursor->pointers[i].blockAddress;
            cursorStorageAddress.offset = 0;

            return getBPTreeLevel(cursorStorageAddress, level + 1);
          }
        } else {
          return level;
        }
      }
      return 0;
    }

    Address getRootStorageAddress() { return rootStorageAddress; };

    //Display Key Value from Cursor :WJ
    int getCursorKey(BPNode* cursor, int i) { 
      return cursor->keys[i]; 
    }

    int getNumNodes() { return numNodes; }

    int getMaxKeys() { return maxKeys; }

    int remove2(int target){
      BPNode* cursor = (BPNode*) rootStorageAddress.blockAddress; //current target in B+Tree
      BPNode* parent; // Keep track of the parent as we go deeper into the tree in case we need to update it.

      cout << "rootStorageAddress: " << rootStorageAddress.blockAddress << endl;

      int leftSibling, rightSibling; // Index of left and right child to borrow from.
      int deletedNodesCount; //Count of node is deleted or two nodes are merged
      int updatedNodesCount; //number nodes of the updated B+ tree;
      int heightOfBPlusTree; //height of the updated B+ tree

      if (cursor != nullptr){
        while(!cursor->isLeaf){
          parent = cursor; // Set the parent of the node

          //cout << "line2046: " << cursor << endl;
          //cout << "line2046: " << cursor->numKeys << endl;
          for (int i = 0; i < cursor->numKeys; i++){
            cout << "cursor->pointers[ " << i << " ]: " << cursor->pointers[i].blockAddress << endl;

            leftSibling = i - 1;
            rightSibling = i + 1;

            cout << "leftSibling: " << leftSibling << endl;
            cout << "rightSibling: " << rightSibling << endl;

            int key = getCursorKey(cursor,i);

            // If key is lesser than current key, go to the left pointer's node.
            if (target < key) {
                cursor = (BPNode*) cursor->pointers[i].blockAddress;
                //printCurrentPointer(cursor,i);
                break;
            }

            // Else if key larger than all keys in the node, go to last pointer's node (rightmost).
            if(cursor->getKeysCount() - 1 == i) {
              leftSibling = i;
              rightSibling = i + 2;

              //cout << "debug: " << cursor->pointers[i].blockAddress << endl;
              //cout << "debug2: " << cursor->pointers[i+1].blockAddress << endl;

              //to-fix: need to update cursor with Next Node
              cursor = (BPNode*) cursor->pointers[i + 1].blockAddress;
              displayNode(cursor);
              //printCurrentPointer(cursor,i);
              break;
            }
          }
        }

        //Reach leaf node
        bool foundkey = false;
        int pos;  //The position of the target key if found

        cout << "numKeys: " << cursor->numKeys << endl;
        for (pos = 0; pos < cursor->numKeys; pos++) {
          cout << "[ " << pos << " ] = " << cursor->keys[pos] << endl;

          if (getCursorKey(cursor,pos) == target) {
            cout << "The end: Found key at [i]=" << pos << ";value=" << cursor->keys[pos] << endl;
            cout << "The end: Found key ptr: " << cursor->pointers[pos].blockAddress << " + " << cursor->pointers[pos].offset << endl;

            //printCurrentPointer(cursor,pos);
            foundkey = true;
            break;
          }
        }

        if (!foundkey){
          cout << "The end: No key found" << endl;

          //todo: need to return numNodesDeleted after deletion
          return 0; 
        }

        //cout << "line 2038" << endl;

        //todo: remove Linked List

        //delete the key
        deleteTargetKeyFromNode(cursor, pos);

        movePointersForward(cursor, maxKeys);

        // If current node is root, check if tree still has keys.
        if (cursor == root && cursor->numKeys == 0) {
           // Reset root pointers in the B+ Tree.
          root = nullptr;
          rootAddress = nullptr;

          //todo: need to return numNodesDeleted after deletion
          return 999; 
        }

        bool hasUnderflow = checkHasUnderflow(cursor,maxKeys);
        if(hasUnderflow){
          //Try to lend from Left Sibling
          if (leftSibling >= 0) {
            int numNodesDeleted = borrowFromLeftSibling(cursor,parent,leftSibling,maxKeys);

            if(numNodesDeleted > 0){
              return numNodesDeleted;
            }
          }

          //Try to lend from Right Sibling
          if (rightSibling <= parent->numKeys) {
            int numNodesDeleted = borrowFromRightSibling(cursor,parent,rightSibling,maxKeys);

            if(numNodesDeleted > 0){
              return numNodesDeleted;
            }
          }
          
          //No Left/Right Sibling to borrow, thus we do Merge Nodes to resolve Underflow
          // If left sibling exists, merge with it.
          if (leftSibling >= 0){
            cout << "mergeWithLeftSibling" << endl;
            mergeWithLeftSibling(cursor,parent,leftSibling);
          } else if (rightSibling <= parent->numKeys){
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

    void deleteTargetKeyFromNode(BPNode* cursor, int pos){
      for (int i = pos; i < cursor->numKeys; i++) {
        cursor->keys[i] = cursor->keys[i + 1];
        cursor->pointers[i] = cursor->pointers[i + 1];
      }
      cursor->numKeys--; //update numKeys (minus 1 key)
    }

    void movePointersForward(BPNode* cursor, int maxKeys){
        // Move the last pointer forward (if any).
        cursor->pointers[cursor->numKeys] = cursor->pointers[cursor->numKeys + 1];

        // Set all forward pointers from numKeys onwards to nullptr.
        for (int i = cursor->numKeys + 1; i < maxKeys + 1; i++) {
          Address nullAddress;
          nullAddress.blockAddress = nullptr;
          nullAddress.offset = 0;
          cursor->pointers[i] = nullAddress;
        }
    }

    bool checkHasUnderflow(BPNode* cursor, int maxKeys){
      if (cursor->numKeys >= (maxKeys + 1) / 2){
        cout << "Underflow: false" << endl;
        return false;
      }
      cout << "Underflow: true" << endl;
      return true;
    }

    int borrowFromLeftSibling(BPNode* cursor, BPNode* parent, int leftSibling, int maxKeys){
      BPNode* leftNode = (BPNode*) parent->pointers[leftSibling].blockAddress;

      if (leftNode->numKeys >= (maxKeys + 1) / 2 + 1) {
        // Shift last pointer back by one first.
        //cout << "cursor->pointers[" << cursor->numKeys + 1 << "] was : " << static_cast<void*>(cursor->pointers[cursor->numKeys + 1].blockAddress) + cursor->pointers[cursor->numKeys + 1].offset << endl;
        cursor->pointers[cursor->numKeys + 1] = cursor->pointers[cursor->numKeys];
        //cout << "cursor->pointers[" << cursor->numKeys + 1 << "] : " << static_cast<void*>(cursor->pointers[cursor->numKeys + 1].blockAddress) + cursor->pointers[cursor->numKeys + 1].offset << endl;
        //displayNode(cursor);

        // Shift all remaining keys and pointers back by one.
        for (int i = cursor->numKeys; i > 0; i--) {
          //cout << "cursor->keys[" << i << "] : " << cursor->keys[i] << " < " << cursor->keys[i - 1] << endl;
          //cout << "cursor->pointers[" << i << "] : " << static_cast<void*>(cursor->pointers[i].blockAddress) << " < " << static_cast<void*>(cursor->pointers[i - 1].blockAddress) << endl;
          cursor->keys[i] = cursor->keys[i - 1];
          cursor->pointers[i] = cursor->pointers[i - 1];

          //displayNode(cursor);
        }

        // Transfer borrowed key and pointer (rightmost of left node) over to current node.
        cursor->keys[0] = leftNode->keys[leftNode->numKeys - 1];
        cursor->pointers[0] = leftNode->pointers[leftNode->numKeys - 1];
        //cout << "cursor: ";
        //displayNode(cursor);

        cursor->numKeys++;
        leftNode->numKeys--;

        // Update left sibling (shift pointers left)
        leftNode->pointers[cursor->numKeys + 1].blockAddress = cursor;
        leftNode->pointers[cursor->numKeys + 1].offset = 0;
        //cout << "leftnode: ";
        //displayNode(leftNode);

        // Update parent node's key
        parent->keys[leftSibling] = cursor->keys[0];
        //cout << "parent: ";
        //displayNode(parent);

        //todo: need to return numNodesDeleted after deletion
        return 999;  
      }

      return 0;
    }

    int borrowFromRightSibling(BPNode* cursor, BPNode* parent, int rightSibling, int maxKeys){
      BPNode* rightNode = (BPNode*) parent->pointers[rightSibling].blockAddress;

      if (rightNode->numKeys >= (maxKeys + 1) / 2 + 1){
        // Shift last pointer back by one first.
        cursor->pointers[cursor->numKeys + 1] = cursor->pointers[cursor->numKeys];

        // No need to shift remaining pointers and keys since we are inserting on the rightmost.
        // Transfer borrowed key and pointer (leftmost of right node) over to rightmost of current node.
        cursor->keys[cursor->numKeys] = rightNode->keys[0];
        cursor->pointers[cursor->numKeys] = rightNode->pointers[0];
        cursor->numKeys++;
        rightNode->numKeys--;

        // Update right sibling (shift keys and pointers left)
        for (int i = 0; i < rightNode->numKeys; i++) {
          rightNode->keys[i] = rightNode->keys[i + 1];
          rightNode->pointers[i] = rightNode->pointers[i + 1];
        }

        // Move right sibling's last pointer left by one too.
        rightNode->pointers[cursor->numKeys] = rightNode->pointers[cursor->numKeys + 1];

        // Update parent node's key to be new lower bound of right sibling.
        parent->keys[rightSibling - 1] = rightNode->keys[0];

        //todo: need to return numNodesDeleted after deletion
        return 999;  
      }

      return 0;
    }

    void mergeWithLeftSibling(BPNode* cursor, BPNode* parent, int leftSibling) {
       BPNode* leftNode = (BPNode*) parent->pointers[leftSibling].blockAddress;

       // Transfer all keys and pointers from current node to left node.
      for (int i = leftNode->numKeys, j = 0; j < cursor->numKeys; i++, j++) {
        leftNode->keys[i] = cursor->keys[j];
        leftNode->pointers[i] = cursor->pointers[j];
      }

      // Update variables, make left node last pointer point to the next leaf node pointed to by current.
      leftNode->numKeys += cursor->numKeys;
      leftNode->pointers[leftNode->numKeys] = cursor->pointers[cursor->numKeys];

      //todo: 
      // We need to update the parent in order to fully remove the current node.
      removeInternal2(parent->keys[leftSibling], parent, cursor);
    }

    void mergeWithRightSibling(BPNode* cursor, BPNode* parent, int rightSibling) {
      BPNode* rightNode = (BPNode*) parent->pointers[rightSibling].blockAddress;
      cout << "rightNode1: " << rightNode->pointers->blockAddress << endl;
      cout << "rightNode2: " << parent->pointers[rightSibling].blockAddress << endl;
      cout << "rootAddress: " << rootAddress << endl;

      // Transfer all keys and pointers from right node into current.
      for (int i = cursor->numKeys, j = 0; j < rightNode->numKeys; i++, j++) {
        cursor->keys[i] = rightNode->keys[j];
        cursor->pointers[i] = rightNode->pointers[j];
      }

      // Update variables, make current node last pointer point to the next leaf node pointed to by right node.
      cursor->numKeys += rightNode->numKeys;
      cursor->pointers[cursor->numKeys] = rightNode->pointers[rightNode->numKeys];

      //auto kk = parent->keys[rightSibling-1];
      //cout << "parent->keys[rightSibling-1]: " << kk << endl;
      //auto m = 0;

      //todo: 
      // We need to update the parent in order to fully remove the right node.
      //void *rightNodeAddress = parent->pointers[rightSibling].blockAddress;
      //  removeInternal(parent->keys[rightSibling - 1], (BPNode *)parentDiskAddress, (BPNode *)rightNodeAddress);
      removeInternal2(parent->keys[rightSibling - 1], parent, rightNode);
    }


    //Display Key Value from Cursor :WJ
    int getCursorKey(BPNode *cursor,int i) { 
      cout << "Accessing key:"<< cursor->keys[i] << endl;
      return cursor->keys[i]; 
    }

    //refactor displayLL2 :WJ (pending 24.9.22)
    void displayLL2(Address LLHeadAddress){
      void *recordAddress = operator new(sizeof(Record));
      std::memcpy(recordAddress, LLHeadAddress.blockAddress, sizeof(Record));

      Record *record = (Record *)recordAddress;
      cout << "==== Record Details ===="<< endl;
      cout << "tconst: "<< record->tconst << endl;
      cout << "avgRating: "<< record->avgRating << endl;
      cout << "numVotes: "<< record->numVotes << endl;
      cout << "====================="<< endl;

      LLNode *LLs = (LLNode *)recordAddress;
     
   
      // int k = countLinkedListNodes(LLs);
      // cout << "displaying Key's LinkedList"<<endl;
      // cout << "coming soon...."<<endl;

      // Record *ll1 = (Record *)LLs;
      // cout << "==== Record Details ===="<< endl;
      // cout << "tconst: "<< ll1->tconst << endl;
      // cout << "avgRating: "<< ll1->avgRating << endl;
      // cout << "numVotes: "<< ll1->numVotes << endl;
      // cout << "====================="<< endl;

      auto m = 0;

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