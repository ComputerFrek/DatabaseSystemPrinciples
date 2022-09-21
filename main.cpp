#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <unordered_map>

#include "storage.cpp"
#include "bptree.cpp"
#include "types.h"

using namespace std;

int main(){
  int BLOCKSIZE=0;
  cout <<"=========================================================================================="<<endl;
  cout <<"Select Block size:           "<<endl;

  /*int choice = 0;
  while (choice != 1 && choice != 2){
    std::cout << "Enter a choice: " <<endl;
    std::cout << "1. 200 B " <<endl;
    std::cout << "2. 500 B" <<endl;
    cin >> choice;
    if (int(choice) == 1)
    {
      BLOCKSIZE = int(200);
    } 
    else if (int(choice) == 2)
    {
      BLOCKSIZE = int(500);
    }
    else 
    {
      cin.clear();
      std::cout << "Invalid input, input either 1 or 2" <<endl;
    }
  }*/
  BLOCKSIZE = 200;

  // create the stream redirection stuff 
  streambuf *coutbuf = std::cout.rdbuf(); //save old buffer

  // save experiment1 logging
  ofstream out1("experiment1_" + to_string(BLOCKSIZE) + "B.txt");
  cout.rdbuf(out1.rdbuf());           //redirect std::cout to filename.txt!

  /*
  =============================================================
  Experiment 1:
  Store the data (which is about IMDb movives and described in Part 4) on the disk and report the following statistics:
  - The number of blocks;
  - The size of database;
  =============================================================
  */

  // Create memory pools for the disk and the index, total 500MB
  // The split is determined empirically. We split so that we can have a contiguous disk address space for records
  cout << "creating the disk on the stack for records, index" << endl;
  Storage index(350000000, BLOCKSIZE); // 350MB
  Storage disk(150000000, BLOCKSIZE);  // 150MB


  // Creating the tree 
  BPlusTree tree = BPlusTree(BLOCKSIZE, &disk, &index);
  cout << "Max keys for a B+ tree node: " << tree.getMaxKeys() << endl;

  // Reset the number of blocks accessed to zero
  disk.resetBlocksAccessed();
  index.resetBlocksAccessed();
  cout << "Number of record blocks accessed in search operation reset to: 0" << endl;
  cout << "Number of index blocks accessed in search operation reset to: 0" << endl;

  // Open test data
  cout << "Reading in data ... " << endl << endl;
  ifstream inputfile("data.tsv");
  string inputstring;

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

    if(inputtoken[0].compare("tt0000007") == 0){
      cout << "Break" << endl;
    }

    cout << "Writing record: " << record.tconst << endl;
    Address tempAddress = disk.saveToDisk(&record, sizeof(Record));

    cout << "Inserting record: " << record.tconst << " to bptree " << endl;
    //build the bplustree as we insert records
    tree.insert(tempAddress, record.numVotes);

    //logging
    cout << "Inserted record " << record.tconst << " at block address: " << &tempAddress.blockAddress << " and offset " << &tempAddress.offset << endl;
    cout << endl;
    }
}