#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct Record {
    char tconst[10];
    double avgRating;
    int numVotes;
};

int main()
{
    ifstream inputfile("data.tsv");
    string inputstring;

    int z = 0;
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

        for(int i=0; i<inputtoken.size(); i++){
            cout << inputtoken[i] << " ";
        }
        cout << endl;

        Record crecord;
        strcpy(crecord.tconst, inputtoken[0].c_str());
        crecord.avgRating = stod(inputtoken[1]);
        crecord.numVotes = stoi(inputtoken[2]);

        cout << "tconst: " << crecord.tconst << ", avgRating: " << crecord.avgRating << ", numVotes: " << crecord.numVotes << ", sizeof: " << sizeof(crecord) << endl;

        if (z < 10){
            z++;
        } else {
            break;
        }
    }

    inputfile.close();
}