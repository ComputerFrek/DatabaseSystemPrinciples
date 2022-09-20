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

int main() {
  int BLOCKSIZE=0;
  cout <<"=========================================================================================="<<endl;
  cout <<"Select Block size:           "<<endl;

  //int choice = 0;
  /*while (choice != 1 && choice != 2){
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
  Storage disk(150000000, BLOCKSIZE);  // 150MB
  Storage index(350000000, BLOCKSIZE); // 350MB

  // Creating the tree 
  BPlusTree tree = BPlusTree(BLOCKSIZE, &disk, &index);
  cout << "Max keys for a B+ tree node: " << tree.getMaxKeys() << endl;

  // Reset the number of blocks accessed to zero
  disk.resetBlocksAccessed();
  index.resetBlocksAccessed();
  cout << "Number of record blocks accessed in search operation reset to: 0" << endl;
  cout << "Number of index blocks accessed in search operation reset to: 0" << endl;    

  // Open test data
  cout << "Reading in data ... " << endl;
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

        if(inputtoken[0].compare("tt0003839") == 0){
            cout << "Test" << endl;
        }

        Address tempAddress = disk.saveToDisk(&record, sizeof(Record));

        //build the bplustree as we insert records
        tree.insert(tempAddress, record.numVotes);

        //logging
        cout << "Inserted record " << record.tconst << " at block address: " << &tempAddress.blockAddress << " and offset " << &tempAddress.offset << endl;
    }

  /*
  =============================================================
  Experiment 1:
  Store the data (which is about IMDb movives and described in Part 4) on the disk (as specified in Part 1) and report the following statistics:
  - the number of blocks;
  - the size of database (in terms of MB);
  =============================================================
  */
  cout <<"=====================================Experiment 1=========================================="<<endl;
  cout << "Number of records per record block --- " << BLOCKSIZE / sizeof(Record) << endl;
  cout << "Number of keys per index block --- " << tree.getMaxKeys() << endl;
  cout << "Number of record blocks --- " << disk.getAllocated() << endl;
  cout << "Number of index blocks --- " << index.getAllocated() << endl;
  cout << "Size of actual record data stored --- " << disk.getActualSizeUsed() << endl;
  cout << "Size of actual index data stored --- " << index.getActualSizeUsed() << endl;
  cout << "Size of record blocks --- " << disk.getSizeUsed() << endl;
  cout << "Size of index blocks --- " << index.getSizeUsed() << endl;
  cout << "Total number of blocks   : " << disk.getAllocated() + index.getAllocated() << endl;
  cout << "Actual size of database : " << disk.getActualSizeUsed() + index.getActualSizeUsed() << endl;
  cout << "Size of database (size of all blocks): " << disk.getSizeUsed()+index.getSizeUsed() << endl;
  
  // finish saving experiment1 logging
  cout.rdbuf(coutbuf); //reset to standard output again
  
  // reset counts for next part
  index.resetBlocksAccessed();
  disk.resetBlocksAccessed();

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
  // save experiment2 logging
  ofstream out2("experiment2_" + to_string(BLOCKSIZE) + "B.txt");
  cout.rdbuf(out2.rdbuf());           //redirect std::cout to filename.txt!

  // call experiment 2
  cout << "=====================================Experiment 2==========================================" << endl;
  cout << "Parameter n of the B+ tree    : " << tree.getMaxKeys() << endl;
  cout << "Number of nodes of the B+ tree: " << tree.getNumNodes() << endl;
  cout << "Height of the B+ tree         : " << tree.getLevels() << endl;
  cout << "Root nodes and child nodes :" << endl;
  tree.display(tree.getRoot(),1);
  cout << endl;

  // finish saving experiment2 logging
  cout.rdbuf(coutbuf); //reset to standard output again

  // reset counts for next part
  index.resetBlocksAccessed();
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

  // save experiment3 logging
  ofstream out3("experiment3_" + to_string(BLOCKSIZE) + "B.txt");
  cout.rdbuf(out3.rdbuf());           //redirect std::cout to filename.txt!

  // call experiment 3
  cout <<"=====================================Experiment 3=========================================="<<endl;
  cout <<"Retrieving the attribute tconst of those movies with numVotes equal to 500..."<<endl;     
  tree.search(500,500);
  cout << endl;
  cout <<"Number of index blocks the process accesses: "<<index.resetBlocksAccessed()<<endl; 
  cout <<"Number of record blocks the process accesses: "<<disk.resetBlocksAccessed()<<endl;
  cout << "\nNo more records found for range " << 8.0 << " to " << 8.0 << endl;
  
  // finish saving experiment3 logging
  cout.rdbuf(coutbuf); //reset to standard output again        
  
  /*
  =============================================================
  Experiment 4:
  Retrieve those movies with the attribute “numVotes” from 30,000 to 40,000, both inclusively and report the following statistics:
    - the number and the content of index nodes the process accesses;
    - the number and the content of data blocks the process accesses;
    - the average of “averageRating’s” of the records that are returned;
  =============================================================
  */

  // save experiment4 logging
  ofstream out4("experiment4_" + to_string(BLOCKSIZE) + "B.txt");
  cout.rdbuf(out4.rdbuf());           //redirect std::cout to filename.txt!

  // call experiment 4
  cout <<"=====================================Experiment 4=========================================="<<endl;
  cout <<"Retrieving the attribute tconst of those movies with numVotes from 30,000 to 40,000 (inclusively)..."<<endl;
  tree.search(30000,40000);
  cout << endl;
  cout <<"Number of index blocks the process accesses: "<<index.resetBlocksAccessed()<<endl; 
  cout <<"Number of data blocks the process accesses: "<<disk.resetBlocksAccessed()<<endl;
  
  // finish saving experiment4 logging
  std::cout.rdbuf(coutbuf); //reset to standard output again

  /*
  =============================================================
  Experiment 5:
  Delete those movies with the attribute “numVotes” equal to 1,000, update the B+ tree accordingly, and report the following statistics:
- the number of times that a node is deleted (or two nodes are merged) during the process of the updating the B+ tree;
- the number nodes of the updated B+ tree;
- the height of the updated B+ tree;
- the content of the root node and its 1st child node of the updated B+ tree;
  =============================================================
  */

  // save experiment5 logging
  ofstream out5("experiment5_" + to_string(BLOCKSIZE) + "B.txt");
  std::cout.rdbuf(out5.rdbuf());           //redirect std::cout to filename.txt!

  // call experiment 5
  std::cout <<"=====================================Experiment 5=========================================="<<endl;
  std::cout<<"Deleting those movies with the attribute numVotes equal to 1000...\n";
  
  int nodesDeleted = tree.remove(1000);

  std::cout << "B+ Tree after deletion" << endl;
  std::cout <<"Number of times that a node is deleted (or two nodes are merged): "<< nodesDeleted << endl; 
  std::cout << "Number of nodes in updated B+ Tree --- " << tree.getNumNodes() << endl;
  std::cout << "Height of updated B+ tree --- " << tree.getLevels() << endl;
  std::cout << endl;
  tree.display(tree.getRoot(), 1);
  std::cout << endl;

  // finish saving experiment5 logging
  std::cout.rdbuf(coutbuf); //reset to standard output again

  // reset counts for next part
  index.resetBlocksAccessed();
  disk.resetBlocksAccessed(); 
  
  return 0;
}