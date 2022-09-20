#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <math.h>
#include "storage.cpp"
#include "record.h"
#include "bptree.cpp"

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

    BPTree bptree(blocksize, &dbstorage, &instorage);

    int i = 1;

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
        
        Record record;
        strcpy(record.tconst, inputtoken[0].c_str());
        record.avgRating = stod(inputtoken[1]);
        record.numVotes = stoi(inputtoken[2]);

        void* recptr = nullptr;
        //cout << "Rcdptr: " << dbstorage.addRecord(inputtoken[0], stod(inputtoken[1]), stoi(inputtoken[2])) << endl;
        recptr = dbstorage.writeToDisk(&record, sizeof(record));

        cout << "tconst: " << record.tconst << " - numvotes: " << record.numVotes << endl;
        if(record.numVotes == 1572){
            cout << "Split root" << endl;
        }
        bptree.inserttotree(stoi(inputtoken[2]), recptr);

        /*
        for(int i = 0; i < bptree.maxkeyspernode; i++){
            cout << bptree.root->keyptr[i].key << " : " << bptree.root->keyptr[i].dataptr << endl;
            if(bptree.root->keyptr[i+1].dataptr == nullptr){
                cout << endl;
                break;
            }
        }
        */
    }

    /*
    for(int i=0;i<dbstorage.datapointers.size();i++){
        Record* testptr = (Record*) dbstorage.datapointers[i];
        cout << "Record: " << testptr->tconst << " - " << testptr->avgRating << " - " << testptr->numVotes << " : " << testptr << endl;
    }
    */

    //cout << "Block Size: " << blocksize << endl;
    //cout << "Number of records per block: " << dbstorage.numofrecordsperblock << endl;

    cout << "===== Experiment 1 =====" << endl;
    //cout << "Number of blocks: " << dbstorage.getNumberOfDataBlocks() << endl;
    //cout << "Size of database: " << blocksize * dbstorage.getNumberOfDataBlocks() / 1000000 << " mb" << endl;

    cout << "===== Experiment 2 =====" << endl;
    bptree.displaytree(bptree.root, 0);

    inputfile.close();
}