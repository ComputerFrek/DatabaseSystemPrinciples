#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <unordered_map>
#include <tuple>

#include "storage.cpp"
#include "bptree.cpp"
#include "types.h"

using namespace std;

int main(){
  int BLOCKSIZE=0;
  cout << "==========================================================================================" << endl;
  cout << "Enter Block size:" << endl;

  BLOCKSIZE = 200;

  // create the stream redirection stuff 
  streambuf* coutbuf = cout.rdbuf(); //save old buffer
  ofstream out1("output_" + to_string(BLOCKSIZE) + "B.txt");
  cout.rdbuf(out1.rdbuf());           //redirect std::cout to filename.txt!

  cout << "creating the disk on the stack for records" << endl;
  Storage disk(500000000, BLOCKSIZE);  // 500MB

  // Creating the tree 
  BPlusTree tree = BPlusTree(&disk, BLOCKSIZE);
  cout << "Max keys for a B+ tree node: " << tree.getMaxKeys() << endl;

  // Open test data
  cout << "Reading in data ... " << endl << endl;
  ifstream inputfile("datatest.tsv");
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

    cout << "Writing record: " << record.tconst << endl;
    Address tempAddress = disk.saveToDisk(&record, sizeof(Record));

    cout << "Inserting record: " << record.tconst << " to bptree " << endl;
    //build the bplustree as we insert records
    tree.insert(tempAddress, record.numVotes);
    
    //logging
    //cout << "Inserted record " << record.tconst << " - " << record.numVotes << " at block address: " << static_cast<void*>(tempAddress.blockAddress) + tempAddress.offset << " -> " << static_cast<void*>(tempAddress.blockAddress) + tempAddress.offset + sizeof(Record) << endl;
    //cout << "Inserted index " << record.tconst << " at block address: " << static_cast<void*>(testaddress.blockAddress) << " -> " << static_cast<void*>(testaddress.blockAddress) + testaddress.offset << endl;
    cout << endl;
  }
  /*
  =============================================================
  Experiment 1:
  Store the data (which is about IMDb movives and described in Part 4) on the disk (as specified in Part 1) and report the following statistics:
  - the number of blocks;
  - the size of database (in terms of MB);
  =============================================================
  */
  cout << "==================================== Experiment 1 =========================================" << endl;
  cout << "Number of blocks : " << disk.getAllocated() << endl;
  cout << "Size of database : " << ((disk.getAllocated() * BLOCKSIZE) / 1000) << endl;
  cout << "==================================== Experiment 1 End =====================================" << endl;
  cout << endl;

  /*
  =============================================================
  Experiment 2:
  Build a B+ tree on the attribute "numVotes" by inserting the records sequentially and report the following statistics:
    - the parameter n of the B+ tree;
    - the number of nodes of the B+ tree;
    - the height of the B+ tree, i.e., the number of levels of the B+ tree;
    - the content of the root node and its 1st child node;
  =============================================================
  */
  cout << "==================================== Experiment 2 =========================================" << endl;
  cout << "Parameter n of the B+ tree    : " << tree.getMaxKeys() << endl;
  cout << "Number of nodes of the B+ tree: " << tree.getNumNodes() << endl;
  cout << "Height of the B+ tree         : " << tree.getBPTreeLevel(tree.getRootStorageAddress(), 0) << endl;
  cout << "Root nodes and child nodes :" << endl;
  tree.display(tree.getRootStorageAddress(), 0);
  cout << "==================================== Experiment 2 End =====================================" << endl;
  cout << endl;

  disk.resetBlocksAccessed();

  /*
  =============================================================
  Experiment 3:
  Retrieve those movies with the “numVotes” equal to 500 and report the following statistics:
    - the number and the content of index nodes the process accesses; (for the content, it would be sufficient to report for the first 5 index nodes or data blocks only if there are more than 5, and this applies throughout Experiment 3 and Experiment 4).
    - the number and the content of data blocks the process accesses;
    - the average of “averageRating’s” of the records that are returned;
  =============================================================
  */
  cout << "==================================== Experiment 3 =========================================" << endl;
  cout << "Retrieving the attribute tconst of those movies with numVotes equal to 500:"<<endl;
  cout << "tconst  avgrating  numvotes" << endl;
  int iproc = 0;
  int rproc = 0;
  tie(iproc, rproc) = tree.search(5,5);
  cout << endl;
  cout << "Number of index nodes processed: " << iproc << endl;
  cout << "Number of record blocks processed: " << disk.resetBlocksAccessed() << endl;
  cout << "==================================== Experiment 3 End =====================================" << endl;
  cout << endl;
  
  
  /*
  =============================================================
  Experiment 4:
  Retrieve those movies with the attribute “numVotes” from 30,000 to 40,000, both inclusively and report the following statistics:
    - the number and the content of index nodes the process accesses;
    - the number and the content of data blocks the process accesses;
    - the average of “averageRating’s” of the records that are returned;
  =============================================================
  */
  cout << "==================================== Experiment 4 =========================================" << endl;
  cout << "Retrieving the attribute tconst of those movies with numVotes from 30,000 to 40,000 (inclusively)..." << endl;
  cout << "tconst  avgrating  numvotes" << endl;
  iproc = 0;
  rproc = 0;
  tie(iproc, rproc) = tree.search(1, 40000);
  cout << endl;
  cout << "Number of index nodes processed: " << iproc << endl;
  cout << "Number of record blocks processed: " << disk.resetBlocksAccessed() << endl;
  cout << "==================================== Experiment 4 End =====================================" << endl;
  cout << endl;
  
  /*
  =============================================================
  Experiment 5:
  Delete those movies with the attribute “numVotes” equal to 1,000, update the B+ tree accordingly, and report the following statistics:
- the number of times that a node is deleted (or two nodes are merged) during the process of the updating the B+ tree;
- the number nodes of the updated B+ tree;
- the height of the updated B+ tree;
- the content of the root node and its 1st child node of the updated B+ tree;
  =============================================================
  
  cout <<"=====================================Experiment 5=========================================="<<endl;
  cout<<"Deleting those movies with the attribute numVotes equal to 1000...\n";
  
  int nodesDeleted = tree.remove(1000);

  cout << "B+ Tree after deletion" << endl;
  cout <<"Number of times that a node is deleted (or two nodes are merged): "<< nodesDeleted << endl; 
  cout << "Number of nodes in updated B+ Tree --- " << tree.getNumNodes() << endl;
  cout << "Height of updated B+ tree --- " << tree.getLevels() << endl;
  cout << endl;

  tree.display(tree.getRoot(), 1);
  cout << endl;

  // reset counts for next part
  disk.resetBlocksAccessed(); 
  */
  
  return 0;
}