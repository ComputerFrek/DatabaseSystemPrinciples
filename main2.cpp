#include <iostream>
#include "types.h"

using namespace std;

int main(){
    cout << "Int: " << sizeof(int) << endl;
    cout << "Short int: " << sizeof(short int) << endl;
    cout << "Address: " << sizeof(Address) << endl;
    cout << "Record: " << sizeof(Record) << endl;
    cout << "Ptr: " << sizeof(void*) << endl;
    cout << "intarr: " << sizeof(int[10]) << endl;
    cout << "IntPtr: " << sizeof(int*) << endl;
}