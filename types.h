#ifndef TYPES_H
#define TYPES_H

// Defines an address of a record stored as a block address with an offset.
struct Address { // 16bytes?
  void* blockAddress; //8bytes?
  short int offset; //
};

// Defines a single movie record (read from data file).
struct Record { //20bytes?
  char tconst[10];     // ID of the movie. 10bytes
  float avgRating;     // Average rating of this movie. 4 bytes
  int numVotes;        // Number of votes of this movie. 4 bytes
};

class LinkedListNode {
  public:
    Address dataaddress;
    LinkedListNode* next;
};

#endif