#ifndef STORAGE_CPP
#define STORAGE_CPP

#include <iostream>
#include <cstring>
#include <tuple>
#include <vector>
#include "types.h"

using namespace std;

class DiskStorage{

private:

  size_t _sizeOfMaxDiskStorage;   // Maximum disk capacity allocated for storage
  size_t _sizeOfAllBlcokUsed;     // Current total size used of all block(each single block memory may not fully used up).
  size_t _sizeOfActualUsed;       // Total size of records stored in storage
  size_t _sizeOfCurrentBlockUsed; // Size of current single block used
  size_t _sizeOfEachBlock;        // Size of every single block, in this project is 200 bytes


  int _numOfBlockAllocated;       // Indicate the total number of block has been allocated for now 
  int _numOfBlockAvailable;       // Currently how many block are avaliable to store new record 
  int _numOfBlockAccessed;        // Counts the total number of block has been accessed 


  unsigned char* _ptrToMemory;    // Pointer to the memory pool.
  unsigned char* _ptrToBlock;     // Current block pointer we are inserting to.

public:
  DiskStorage(size_t _sizeOfMaxDiskStorage, size_t _sizeOfEachBlock) {

    // size of block and size of disk storage will be passed by parameters
    this->_sizeOfMaxDiskStorage = _sizeOfMaxDiskStorage;
    this->_sizeOfEachBlock = _sizeOfEachBlock;

    // initialize variables
    this->_sizeOfActualUsed = 0;
    this->_sizeOfCurrentBlockUsed = 0;
    this->_sizeOfAllBlcokUsed = 0;
    this->_numOfBlockAllocated = 0;
    this->_numOfBlockAccessed = 0;

    // to calculate number of block avaliable
    this->_numOfBlockAvailable = this->_sizeOfMaxDiskStorage / this->_sizeOfEachBlock; 

    // to create blocks and pointer from memory to disk 
    this->_ptrToMemory = new unsigned char[_sizeOfMaxDiskStorage];

    cout << "Size: " << this->_sizeOfMaxDiskStorage << " - _sizeOfEachBlock: " << this->_sizeOfEachBlock << " - " <<
    static_cast<void *>(this->_ptrToMemory) << " -> " << static_cast<void *>(this->_ptrToMemory) + this->_sizeOfMaxDiskStorage << endl;


    // To initialize the memory of all blocks to be null 
    memset(_ptrToMemory, '\0', _sizeOfMaxDiskStorage); 
    this->_ptrToBlock = nullptr;
  }

  // Save new data to disk and return the disk address
  Address saveDataToDisk(void *itemaddress, size_t size){
    // Current disk address after memory allocated for a new record
    Address diskAddress = allocateNewRecord(size);

    // Current block pointer and the offset between block address and new record address
    unsigned char *currentBlockPtr = (unsigned char *)diskAddress.blockAddress;
    short int currentRecordOffset = diskAddress.offset;

    memcpy(currentBlockPtr + currentRecordOffset, (unsigned char *)itemaddress, size);

    // Update number of blocks has been accessed
    _numOfBlockAccessed++;
    return diskAddress;
  }

  // Give a block address, offset and size, returns the data there.
  void *retrieveDataFromDisk(Address givenAddress, size_t dataSize){
    void *memoryAddress = operator new(dataSize);
    memcpy(memoryAddress, (unsigned char *)givenAddress.blockAddress + givenAddress.offset, dataSize);

    // retrieve successfully, update the block access number
    _numOfBlockAccessed++;

    return memoryAddress;
  }

  // Get the total number of block has been allocated
  int getNumberOfBlockAllocated() const{
    return _numOfBlockAllocated;
  };

  // Rest the total number of blocks has been accessed
  int resetNumberOfBlocksAccessed(){
    int tempBlocksAccessed = _numOfBlockAccessed;
    _numOfBlockAccessed = 0;
    return tempBlocksAccessed;
  }

  bool isBlockAllocatedSucess(){
    // Before allocate block, check if have avaliable block
    if (_numOfBlockAvailable != 0){
      // cout << "The number of block avaliable is : " << _numOfBlockAvailable << endl;
      // If have avaliable block to store data, then set the curretn pointer to the new block has been allocated 
      _ptrToBlock = _ptrToMemory + (_numOfBlockAllocated * _sizeOfEachBlock); 

      // Update variables after block allocated successfully
      _numOfBlockAllocated++;
      _numOfBlockAvailable--;
      _sizeOfAllBlcokUsed += _sizeOfEachBlock;

      // When a new block has been allocated, intialize its current used size to be 0
      _sizeOfCurrentBlockUsed = 0; 

      // cout << "The number of blocks has been successfully allocated is: " << _numOfBlockAllocated << endl;
      // cout << "The number of blocks avaliable is  " << _numOfBlockAvailable << endl;
      // cout << "The total size has been used of all blocks is:" << _sizeOfAllBlcokUsed << endl;
      // cout << "The size of current block has been used is: " << _sizeOfCurrentBlockUsed << endl;

      return true;
    } else {
      // Display error message if there is no avaliable block
      cout << "Error: No memory left to allocate new block (" << _sizeOfAllBlcokUsed << "/" << _sizeOfMaxDiskStorage << " used)." << '\n';
      return false;
    }
  }

  Address allocateNewRecord(size_t recordSize){  
    // If record size more than block size, provide error message and handle exception
    if (recordSize > _sizeOfEachBlock){
      cout << "Error: Th input record size is larger than block size (" << recordSize << " <--> " << _sizeOfEachBlock << ")! Increase the single block size and reallocate" << '\n';
      throw invalid_argument("The input record size exceeds block size!");
    }

    // If current block finished or record size is larger than the current block unused memory size, then allocate a new block to store this record

    size_t totalSizeRequired = _sizeOfCurrentBlockUsed + recordSize;
    if (totalSizeRequired > _sizeOfEachBlock || _numOfBlockAllocated == 0){
      if (!isBlockAllocatedSucess()){
        throw logic_error("Error! Failed to allocate block!");
      }
    }

    // when successfully allocate memory for a new record into block, update variables 
    short int blockSizeUsed = _sizeOfCurrentBlockUsed;
    _sizeOfCurrentBlockUsed += recordSize;
    _sizeOfActualUsed += recordSize;


    // update address and pointer once new record allocated successfully
    Address recordAddress;
    recordAddress.blockAddress = _ptrToBlock;
    recordAddress.offset = blockSizeUsed;

    return recordAddress;
  }

  bool deleteRecord(Address recordAddress, size_t recordSize){
    try{
      // Delete the record from block
      void *addressToDelete = (char *)recordAddress.blockAddress + recordAddress.offset;
      std::memset(addressToDelete, '\0', recordSize);

      // Update size again once record deleted 
      _sizeOfActualUsed -= recordSize;


      // Create a dummy test block to test if current block is empty, and initialize the test block to null 
      unsigned char testBlock[_sizeOfEachBlock];
      memset(testBlock, '\0', _sizeOfEachBlock);

      // if block is empty, update the size of all block used and number of block allocated.
      if (memcmp(testBlock, recordAddress.blockAddress, _sizeOfEachBlock) == 0){
        _sizeOfAllBlcokUsed -= _sizeOfEachBlock;
        _numOfBlockAllocated--;
      }

      return true;
    }
    catch (...)
    {
      cout << "Error: Failed to remove record/block with address : (" << recordAddress.blockAddress << ") and offset (" << recordAddress.offset << ")." << '\n';
      return false;
    };
  }

  /*
  // Get the disk maximum size
  size_t get_sizeOfMaxDiskStorage() const{
    return _sizeOfMaxDiskStorage;
  }

  // Get the size of current block used
  size_t getBlocksizeUsed() const{
    return _sizeOfCurrentBlockUsed;
  };

  // Get the block size
  size_t getBlockSize() const{
    return _sizeOfEachBlock;
  };

  // Get the size of all blocks used
  size_t getSizeUsed() const{
    return _sizeOfAllBlcokUsed;
  }

  

  // Get the size of all records has been used
  size_t getActualsizeUsed() const{
    return _sizeOfActualUsed;
  }

  // Get the number of block has been accessed
  int getNumberOfBlocksAccessed() const{
    return _numOfBlockAccessed;
  }
  */
  

  ~DiskStorage(){};
};
#endif