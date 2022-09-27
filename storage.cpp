#ifndef STORAGE_CPP
#define STORAGE_CPP

#include <iostream>
#include <vector>
#include <tuple>
#include <cstring>

#include "types.h"

using namespace std;

class Storage{
  private:
    size_t maxstoragesize;    // Maximum size allowed for storage.
    size_t blocksize;      // Size of each block in storage in bytes.
    size_t blocksizeused;       // Current size used up for storage (total block size).
    size_t actualsizeused; // Actual size used based on records stored in storage.
    size_t curblocksizeused;  // Size used up within the curent block we are pointing to.

    int blocksallocated;  // Number of currently allocated blocks.
    int blocksaccessed;   // Counts number of blocks accessed.
    int blocksavailable;  // Number of available blocks.

    unsigned char* poolptr;  // Pointer to the memory pool.
    unsigned char* blockptr; // Current block pointer we are inserting to.
  public:
    Storage(size_t maxstoragesize, size_t blocksize) {
      this->maxstoragesize = maxstoragesize;
      this->blocksize = blocksize;
      this->blocksizeused = 0;
      this->actualsizeused = 0;
      this->blocksallocated = 0;
      this->curblocksizeused = 0;
      this->blocksaccessed = 0;
      this->blocksavailable = this->maxstoragesize / this->blocksize;

      // Create pool of blocks.
      this->poolptr = new unsigned char[maxstoragesize];
      
      //cout << "Size: " << this->maxstoragesize << " - blocksize: " << this->blocksize <<" - " << static_cast<void*>(this->poolptr)  << " -> " << static_cast<void*>(this->poolptr) + this->maxstoragesize << endl;
      /*cout << "Blockptr: " << endl;
      for(int i=0; i <= blocksavailable; i++){
        cout << "Block " << i << " : " << static_cast<void*>(this->poolptr) + (i * 200) << endl;
      }*/
      memset(poolptr, '\0', maxstoragesize); // Initialize pool all to null.
      this->blockptr = nullptr;
    }

    bool allocateBlock() {
      // Only allocate a new block if we don't exceed maxstoragesize.
      if (blocksavailable != 0) {
        //cout << "blockavailable: " << blocksavailable << endl;
        //cout << "blocksallocated * blocksize: " << blocksallocated * blocksize << " - poolptr + blocksallocated * blocksize: " << static_cast<void*>(poolptr) + (blocksallocated * blocksize) << endl;
        //cout << "blockptr was: " << static_cast<void*>(blockptr) << endl;
        blockptr = poolptr + (blocksallocated * blocksize); // Set current block pointer to new block.
        //cout << "blockptr: " << static_cast<void*>(blockptr) << endl;

        blocksallocated++;
        blocksavailable--;
        blocksizeused += blocksize;
        curblocksizeused = 0; // Reset offset to 0.
        //cout << "blocksallocated: " << blocksallocated << " blocksavailable: " << blocksavailable << " blocksizeused: " << blocksizeused << " curblocksizeused: " << curblocksizeused << endl;

        return true;
      } else {
        cout << "Error: No memory left to allocate new block (" << blocksizeused << "/" << maxstoragesize << " used)." << '\n';
        return false;
      }
    }

    Address allocate(size_t sizeRequired) {
      // If record size exceeds block size, throw an error.
      if (sizeRequired > blocksize) {
        cout << "Error: Size required larger than block size (" << sizeRequired << " vs " << blocksize << ")! Increase block size to store data." << '\n';
        throw invalid_argument("Requested size too large!");
      }

      // If no current block, or record can't fit into current block, make a new block.
      //cout << "blocksallocated: " << blocksallocated << " curblocksizeused + sizeRequired = " << curblocksizeused + sizeRequired << " > " << blocksize << endl;
      if (blocksallocated == 0 || (curblocksizeused + sizeRequired > blocksize)) {
        bool isSuccessful = allocateBlock();
        if (!isSuccessful) {
          throw logic_error("Failed to allocate new block!");
        }
      }

      // Update variables
      short int offset = curblocksizeused;
      //cout << "offset: " << offset << endl;

      //cout << "curblocksizeused was: " << curblocksizeused << " + " << sizeRequired << endl;
      curblocksizeused += sizeRequired;
      //cout << "curblocksizeused: " << curblocksizeused << endl;

      //cout << "actualsizeused was: " << actualsizeused << " + " << sizeRequired << endl;
      actualsizeused += sizeRequired;
      //cout << "actualsizeused: " << actualsizeused << endl;

      // Return the new memory space to put in the record.
      Address recordAddress;
      recordAddress.blockAddress = blockptr;
      recordAddress.offset = offset;
      //cout << "recordAddress: " << static_cast<void*>(recordAddress.blockAddress) << " - " << recordAddress.offset << endl;

      return recordAddress;
    }

    bool deallocate(Address address, std::size_t sizeToDelete)
    {
      try
      {
        // Remove record from block.
        void *addressToDelete = (char *)address.blockAddress + address.offset;
        std::memset(addressToDelete, '\0', sizeToDelete);

        // Update actual size used.
        actualsizeused -= sizeToDelete;

        // If block is empty, just remove the size of the block (but don't deallocate block!).
        // Create a new test block full of NULL to test against the actual block to see if it's empty.
        unsigned char testBlock[blocksize];
        memset(testBlock, '\0', blocksize);

        // Block is empty, remove size of block.
        if (memcmp(testBlock, address.blockAddress, blocksize) == 0)
        {
          blocksizeused -= blocksize;
          blocksallocated--;
        }

        return true;
      }
      catch (...)
      {
        std::cout << "Error: Could not remove record/block at given address (" << address.blockAddress << ") and offset (" << address.offset << ")." << '\n';
        return false;
      };
    }

    // Give a block address, offset and size, returns the data there.
    void* loadFromDisk(Address address, size_t size) {
      void* mainMemoryAddress = operator new(size);

      //cout << "Reading src loc: " << static_cast<void*>(address.blockAddress) << " - " << static_cast<void*>(address.blockAddress) + address.offset << " -> " << static_cast<void*>(address.blockAddress) + address.offset + size << " : " << size << " - " << address.offset << endl;
      //cout << "Reading dst loc: " << static_cast<void*>(mainMemoryAddress) << " -> " << static_cast<void*>(mainMemoryAddress) + size << " : " << size << endl;
      memcpy(mainMemoryAddress, (unsigned char*) address.blockAddress + address.offset, size);

      // Update blocks accessed
      blocksaccessed++;

      return mainMemoryAddress;
    }

    // Saves something into the disk. Returns disk address.
    Address saveToDisk(void* itemaddress, size_t size) {
      Address diskaddress = allocate(size);
      //cout << "diskaddress: " << static_cast<void*>(diskaddress.blockAddress) << " - " << diskaddress.offset << endl;

      unsigned char* curblockptr = (unsigned char*) diskaddress.blockAddress;
      short int curoffset = diskaddress.offset;

      //cout << "Writing src loc: " << static_cast<void*>(itemaddress) << " -> " << static_cast<void*>(itemaddress) + size << " : " << size << endl;
      //cout << "Writing dst loc: " << static_cast<void*>(curblockptr) << " - " << static_cast<void*>(curblockptr) + curoffset << " -> " << static_cast<void*>(curblockptr) + curoffset + size << " : " << curoffset << " - " << size << endl;
      memcpy(curblockptr + curoffset, (unsigned char*) itemaddress, size);
      
      // Update blocks accessed
      blocksaccessed++;

      return diskaddress;
    }

    // Update data in disk if I have already saved it before.
    Address saveToDisk(void* itemaddress, size_t size, Address diskaddress) {
      //cout << "diskaddress: " << static_cast<void*>(diskaddress.blockAddress) << " - " << diskaddress.offset << endl;

      unsigned char* curblockptr = (unsigned char*) diskaddress.blockAddress;
      short int curoffset = diskaddress.offset;

      //cout << "Writing src loc: " << static_cast<void*>(itemaddress) << " -> " << static_cast<void*>(itemaddress) + size << " : " << size << endl;
      //cout << "Writing dst loc: " << static_cast<void*>(curblockptr) << " - " << static_cast<void*>(curblockptr) + curoffset << " -> " << static_cast<void*>(curblockptr) + curoffset + size << " : " << size << " - " << curoffset << endl;
      
      memcpy(curblockptr + curoffset, itemaddress, size);

      // Update blocks accessed
      blocksaccessed++;

      return diskaddress;
    }

    size_t getmaxstoragesize() const
    {
      return maxstoragesize;
    }

    // Returns the size of a block in memory pool.
    size_t getblocksize() const
    {
      return blocksize;
    };

    // Returns the size used in the current block.
    size_t getblocksizeused() const
    {
      return curblocksizeused;
    };

    // Returns current size used in memory pool (total blocks size).
    size_t getsizeused() const
    {
      return blocksizeused;
    }

    // Returns actual size of all records stored in memory pool.
    size_t getActualsizeused() const
    {
      return actualsizeused;
    }

    // Returns number of currently blocksallocated blocks.
    int getAllocated() const {
      return blocksallocated;
    };

    int getBlocksAccessed() const {
      return blocksaccessed;
    }

    int resetBlocksAccessed() {
      int tempBlocksAccessed = blocksaccessed;
      blocksaccessed = 0;
      return tempBlocksAccessed;
    }

    ~Storage(){};
};
#endif