#ifndef RECORD_HEADER
#define RECORD_HEADER

struct Record {
    char tconst[10]; // char: 1 byte * 10
    double avgRating; // double: 8 byte
    int numVotes; // int: 4 byte
};

#endif