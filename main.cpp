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
    cout << dbstorage.numofrecordsperblock << endl;
    cout << dbstorage.blocks.size() << endl;

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

        cout << "Blockid - recordid: " << dbstorage.getNumberOfBlocks() - 1 << " - " << dbstorage.blocks[dbstorage.getNumberOfBlocks() - 1].getNumberofRecords() << " : " << dbstorage.blocks[dbstorage.getNumberOfBlocks() - 1].records[dbstorage.blocks[dbstorage.getNumberOfBlocks() - 1].getNumberofRecords() - 1].tconst << endl;
    }

    inputfile.close();
}