#ifndef STORAGE_CPP
#define STORAGE_CPP

#include <iostream>
#include <vector>
#include <tuple>
#include <cstring>

#include "types.h"

// We need to create a general memory pool that can be used for both the relational data and the index.
// This pool should be able to assign new blocks if necessary.

// Constructors
class Storage{
  private:
    std::size_t maxPoolSize;    // Maximum size allowed for pool.
    std::size_t blockSize;      // Size of each block in pool in bytes.
    std::size_t sizeUsed;       // Current size used up for storage (total block size).
    std::size_t actualSizeUsed; // Actual size used based on records stored in storage.
    std::size_t blockSizeUsed;  // Size used up within the curent block we are pointing to.

    int allocated;      // Number of currently allocated blocks.
    int blocksAccessed; // Counts number of blocks accessed.

    void *pool;  // Pointer to the memory pool.
    void *block; // Current block pointer we are inserting to.
  public:
    Storage(std::size_t maxPoolSize, std::size_t blockSize){
      this->maxPoolSize = maxPoolSize;
      this->blockSize = blockSize;
      this->sizeUsed = 0;
      this->actualSizeUsed = 0;
      this->allocated = 0;

      // Create pool of blocks.
      this->pool = operator new(maxPoolSize);
      std::memset(pool, '\0', maxPoolSize); // Initialize pool all to null.
      this->block = nullptr;
      this->blockSizeUsed = 0;

      this->blocksAccessed = 0;
    }

    bool allocateBlock()
    {
      // Only allocate a new block if we don't exceed maxPoolSize.
      if (sizeUsed + blockSize <= maxPoolSize)
      {
        // Update variables
        sizeUsed += blockSize;
        block = (char *)pool + allocated * blockSize; // Set current block pointer to new block.
        blockSizeUsed = 0;                    // Reset offset to 0.
        allocated += 1;
        return true;
      }
      else
      {
        std::cout << "Error: No memory left to allocate new block (" << sizeUsed << "/" << maxPoolSize << " used)." << '\n';
        return false;
      }
    }

    Address allocate(std::size_t sizeRequired)
    {
      // If record size exceeds block size, throw an error.
      if (sizeRequired > blockSize)
      {
        std::cout << "Error: Size required larger than block size (" << sizeRequired << " vs " << blockSize << ")! Increase block size to store data." << '\n';
        throw std::invalid_argument("Requested size too large!");
      }

      // If no current block, or record can't fit into current block, make a new block.
      if (allocated == 0 || (blockSizeUsed + sizeRequired > blockSize))
      {
        bool isSuccessful = allocateBlock();
        if (!isSuccessful)
        {
          throw std::logic_error("Failed to allocate new block!");
        }
      }

      // Update variables
      short int offset = blockSizeUsed;

      blockSizeUsed += sizeRequired;
      actualSizeUsed += sizeRequired;

      // Return the new memory space to put in the record.
      Address recordAddress = {block, offset};

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
        actualSizeUsed -= sizeToDelete;

        // If block is empty, just remove the size of the block (but don't deallocate block!).
        // Create a new test block full of NULL to test against the actual block to see if it's empty.
        unsigned char testBlock[blockSize];
        memset(testBlock, '\0', blockSize);

        // Block is empty, remove size of block.
        if (memcmp(testBlock, address.blockAddress, blockSize) == 0)
        {
          sizeUsed -= blockSize;
          allocated--;
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
    void* loadFromDisk(Address address, std::size_t size)
    {
      void *mainMemoryAddress = operator new(size);
      std::memcpy(mainMemoryAddress, (char *)address.blockAddress + address.offset, size);

      // Update blocks accessed
      blocksAccessed++;

      return mainMemoryAddress;
    }

    // Saves something into the disk. Returns disk address.
    Address saveToDisk(void *itemAddress, std::size_t size)
    {
      Address diskAddress = allocate(size);
      std::memcpy((char *)diskAddress.blockAddress + diskAddress.offset, itemAddress, size);

      // Update blocks accessed
      blocksAccessed++;

      return diskAddress;
    }

    // Update data in disk if I have already saved it before.
    Address saveToDisk(void *itemAddress, std::size_t size, Address diskAddress)
    {
      std::memcpy((char *)diskAddress.blockAddress + diskAddress.offset, itemAddress, size);

      // Update blocks accessed
      blocksAccessed++;

      return diskAddress;
    }

    size_t getMaxPoolSize() const
    {
      return maxPoolSize;
    }

    // Returns the size of a block in memory pool.
    size_t getBlockSize() const
    {
      return blockSize;
    };

    // Returns the size used in the current block.
    size_t getBlockSizeUsed() const
    {
      return blockSizeUsed;
    };

    // Returns current size used in memory pool (total blocks size).
    size_t getSizeUsed() const
    {
      return sizeUsed;
    }

    // Returns actual size of all records stored in memory pool.
    size_t getActualSizeUsed() const
    {
      return actualSizeUsed;
    }

    // Returns number of currently allocated blocks.
    int getAllocated() const {
      return allocated;
    };

    int getBlocksAccessed() const {
      return blocksAccessed;
    }

    int resetBlocksAccessed() {
      int tempBlocksAccessed = blocksAccessed;
      blocksAccessed = 0;
      return tempBlocksAccessed;
    }

    ~Storage(){};
};
#endif