#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <math.h>
#include "storage.h"

using namespace std;

int main()
{
    ifstream inputfile("data.tsv");
    string inputstring;

    int blocksize = 200;
    Storage dbstorage(blocksize);

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

        dbstorage.addRecord(inputtoken[0], stod(inputtoken[1]), stoi(inputtoken[2]));

        cout << dbstorage.getNumberOfBlocks() - 1 << " - " << dbstorage.blocks[dbstorage.getNumberOfBlocks() - 1].getNumberofRecords() << " : " << dbstorage.blocks[dbstorage.getNumberOfBlocks() - 1].records[dbstorage.blocks[dbstorage.getNumberOfBlocks() - 1].getNumberofRecords() - 1].tconst << " : " << &dbstorage.blocks[dbstorage.getNumberOfBlocks() - 1].records[dbstorage.blocks[dbstorage.getNumberOfBlocks() - 1].getNumberofRecords() - 1] << endl;
    }

    cout << "Block Size: " << blocksize << endl;
    cout << "Number of records per block: " << dbstorage.numofrecordsperblock << endl;

    cout << "===== Experiment 1 =====" << endl;
    cout << "Number of blocks: " << dbstorage.getNumberOfBlocks() << endl;
    cout << "Size of database: " << blocksize * dbstorage.getNumberOfBlocks() / 1000000 << " mb" << endl;

    cout << "===== Experiment 2 =====" << endl;

    inputfile.close();
}