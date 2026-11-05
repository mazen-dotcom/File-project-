#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <limits>
#include <algorithm>

using namespace std;

struct Appointments {
    char  ID [15] ;
    char  Date [30] ;
    char doctor_ID [15]  ;
};
struct Doctor {
    char id[15];
    char name[30];
    char specialty[20];
    char address[30];
};



// Read the AVAIL LIST head
int readHeader(fstream& file) {

    int head;

    file.clear();
    file.seekg(0, ios::beg);
    file.read((char*)&head, sizeof(int));

    return head;
}


void updateHeader(fstream& file, int newHead) {

    file.clear();
    file.seekp(0, ios::beg);

    file.write((char*)&newHead, sizeof(int));

    file.flush();
}

int main() {

    return 0;
}


