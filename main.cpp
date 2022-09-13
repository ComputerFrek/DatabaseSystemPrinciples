#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <math.h>
#include "storage.cpp"
#include "record.h"

using namespace std;

int main()
{
    ifstream inputfile("datatest.tsv");
    string inputstring;

    size_t datastoragesize = 150000000;
    size_t indexstoragesize = 350000000;
    size_t blocksize = 200;
    Storage dbstorage(datastoragesize, blocksize);
    Storage instorage(indexstoragesize, blocksize);

    while(getline(inputfile, inputstring)){
        if(inputstring.rfind("tconst", 0) == 0){
            continue;
        }

        vector <string> inputtoken;
        istringstream iss(inputstring);
        string word;

        while(getline(iss, word, '\t')){
            inputtoken.push_back(word);
        }

        //cout << "Rcdptr: " << dbstorage.addRecord(inputtoken[0], stod(inputtoken[1]), stoi(inputtoken[2])) << endl;
        dbstorage.addRecord(inputtoken[0], stod(inputtoken[1]), stoi(inputtoken[2]));
    }

    /*
    for(int i=0;i<dbstorage.recordspointers.size();i++){
        Record* testptr = dbstorage.recordspointers[i];
        cout << "Record: " << testptr->tconst << " - " << testptr->avgRating << " - " << testptr->numVotes << " : " << testptr << endl;
    }*/

    
    //cout << "Block Size: " << blocksize << endl;
    //cout << "Number of records per block: " << dbstorage.numofrecordsperblock << endl;

    cout << "===== Experiment 1 =====" << endl;
    //cout << "Number of blocks: " << dbstorage.getNumberOfDataBlocks() << endl;
    //cout << "Size of database: " << blocksize * dbstorage.getNumberOfDataBlocks() / 1000000 << " mb" << endl;

    cout << "===== Experiment 2 =====" << endl;

    inputfile.close();
}