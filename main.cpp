#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
using namespace std;

struct Doctor {
    char id[15];
    char name[30];
    char specialty[20];
    char address[30];
};
struct index {
    char id[15];
    char offset[4];
};
void bubblesort(vector<index>& v) {
    bool swapped = false;
    for (int i = 0; i < v.size()-1; i++) {
        for (int j = 0; j < v.size()-i-1; j++) {
            string id1(v[j].id, 15);
            string id2(v[j+1].id, 15);
            if (id1 > id2) {
                swapped = true;
                swap(v[j], v[j+1]);
            }
        }
        if (!swapped) {
            break;
        }
    }
}
int main() {
    ifstream file("Doctors.dat", ios::binary);
    ofstream primaryindex("index.dat", ios::binary );
    if (!file) {
        cout << "File not found!";
        return 0;
    }




    int header;
    file.read((char*)&header, sizeof(header));



    Doctor d;
    int offset = file.tellg();
    int count =0;
    while (file.read((char*)&d, sizeof(d))) {
       if (count != 0) {
           offset = file.tellg();
       }
        count++;
        string id(d.id,15);
        string name(d.name,30);
        string specialty(d.specialty,20);
        string address(d.address,30);
        if (d.id[0] == '*') {
            continue;
        }

        primaryindex.write(d.id, 15);
        primaryindex.write((char*)&offset, sizeof(offset));;

    }

    cout << "Loop finished!" << endl;
    primaryindex.close();
    file.close();
    ifstream unsortindex("index.dat", ios::binary);
    if (!unsortindex) {
        cout << "Sort index not found!" << endl;
    }
    vector<index> v;
    index temp;
    while (unsortindex.read((char*)&temp, sizeof(temp))) {
        v.push_back(temp);
    }
    unsortindex.close();
    bubblesort(v);
    ofstream sortedindex("index.dat", ios::binary);

    for (int i = 0; i < v.size(); i++) {
        sortedindex.write((char*)&v[i].id, sizeof(v[i].id));
        sortedindex.write((char*)&v[i].offset, sizeof(v[i].offset));
    }
    sortedindex.close();
    // ----------------------------------------
    ifstream file2("Doctors.dat", ios::binary);
    if (!file2) {
        cout << "File not found!" << endl;
    }
    map<string,vector<string>> group;
    int header1;
    file2.read((char*)&header1, sizeof(header1));

    Doctor d2;

    while (file2.read((char*)&d2, sizeof(d2))) {

        string id(d2.id,15);
        string name(d2.name,30);
        string specialty(d2.specialty,20);
        string address(d2.address,30);
        if (d2.id[0] == '*') {
            continue;
        }
        group[name].push_back(id);

    }
    ofstream doctornames("DoctorNameIndex.dat",ios::binary);
    ofstream doctorlists("DoctorNameList.dat",ios::binary);
    for (auto it = group.begin(); it != group.end(); it++) {
        int head = (int)doctorlists.tellp();
        for (int i=0; i<it->second.size(); i++) {
            string primaryindex = it->second[i];


            int nextOffset = -1;

            if (i + 1 < (int)it->second.size()) {
                if (nextOffset ==-1) {
                    nextOffset =19;
                }else {
                    nextOffset = nextOffset +19;
                }


            }

            doctorlists.write(primaryindex.c_str(), 15);
            doctorlists.write((char*)&nextOffset, 4);
        }
        doctornames.write(it->first.c_str(), 30);
        doctornames.write((char *)&head, sizeof(int));
    }
    doctorlists.close();
    doctornames.close();
    file2.close();


    // ---------------------------------------
    ofstream doctorspecialtyindex("DoctorSpecialIndex.dat", ios::binary);
    ofstream doctorspecialtylists("DoctorspecialtyList.dat", ios::binary);
    ifstream file4("Doctors.dat", ios::binary);
    if (!file4) {
        cout << "File not found!" << endl;
    }
    map<string,vector<string>> anothergroup;
    int header2;
    file4.read((char*)&header2, sizeof(header2));

    Doctor d3;

    while (file4.read((char*)&d3, sizeof(d3))) {

        string id(d3.id,15);
        string name(d3.name,30);
        string specialty(d3.specialty,20);
        string address(d3.address,30);
        if (d3.id[0] == '*') {
            continue;
        }
        anothergroup[specialty].push_back(id);
    }
    for (auto it = anothergroup.begin(); it != anothergroup.end(); it++) {
        int head = (int)doctorspecialtylists.tellp();
        for (int i=0; i<it->second.size(); i++) {
            string primaryindex = it->second[i];


            int nextOffset = -1;

            if (i + 1 < (int)it->second.size()) {
                if (nextOffset ==-1) {
                    nextOffset =19;
                }else {
                    nextOffset = nextOffset +19;
                }


            }

            doctorspecialtylists.write(primaryindex.c_str(), 15);
            doctorspecialtylists.write((char*)&nextOffset, 4);
        }
        doctorspecialtyindex.write(it->first.c_str(), 20);
        doctorspecialtyindex.write((char *)&head, sizeof(int));
    }
    file4.close();
    doctorspecialtyindex.close();
    doctorspecialtylists.close();
    // --------------------------------------
    // test case
    ifstream input("DoctorSpecialIndex.dat", ios::binary);

    char name[20];
    int head;

    while (input.read(name, 20)) {
        if (!input.read((char*)&head, sizeof(head)))
            break;

        cout << "Name: ";
        cout.write(name, 20);

        cout << " | Head offset: " << head << endl;
    }

    input.close();

    // ----------------------------------Appointments primary key
    struct Appointments {
        char  ID [15] ;
        char  Date [30] ;
        char doctor_ID [15]  ;
    };
    ifstream  appointments("Appointments.dat", ios::binary);
    ofstream  appointmentsprimaryindex("AppointmentsPrimaryIndex.dat", ios::binary);
    if (!appointments) {
        cout << "File not found!" << endl;
    }
    if (!appointments) {
        cout << "Appointments not found!" << endl;
    }
    int header3 = appointments.tellg();
    appointments.read((char*)&header3, sizeof(header3));

    Appointments a;
    int offset1 = appointments.tellg();
    int count1 =0 ;
    while (appointments.read((char*)&a, sizeof(a))) {
        if (count1 != 0) {
            offset1 = appointments.tellg();
        }
        string id(a.ID,15);
        string date(a.Date,30);
        string doctor_ID(a.doctor_ID,30);

        if (a.ID[0] == '*') {
            continue;
        }
        appointmentsprimaryindex.write(id.c_str(), 15);
        appointmentsprimaryindex.write((char*)&offset1, 4);

    }
    appointmentsprimaryindex.close();
    ifstream unsortedappointmentsprimaryindex("AppointmentsPrimaryIndex.dat", ios::binary);
    if (!unsortedappointmentsprimaryindex) {
        cout << "File not found!" << endl;
    }

    vector<index> v1;
    index temp1;

    while (unsortedappointmentsprimaryindex.read((char*)&temp1, sizeof(temp1))) {
        v1.push_back(temp1);
    }
    bubblesort(v1);
    unsortedappointmentsprimaryindex.close();
    ofstream sortedappointmentsprimaryindex("AppointmentsPrimaryIndex.dat", ios::binary);
    if (!sortedappointmentsprimaryindex) {
        cout << "File not found!" << endl;
    }
    for (int i=0; i<v1.size(); i++) {
        sortedappointmentsprimaryindex.write((char*)&v1[i].id,15);
        sortedappointmentsprimaryindex.write((char*)&v1[i].offset,4);
    }
    sortedappointmentsprimaryindex.close();
    // --------------------------------------------
    ifstream file5("index.dat", ios::binary);
    if (!file5) {
        cout << "File not found!" << endl;
    }
    Appointments a1;
    int header4;
    file5.read((char*)&header4, sizeof(header4));
    map<string,vector<string>> anothergroup2;
    while (file5.read((char*)&a1, sizeof(a1))) {
        string id(a1.ID,15);
        string date(a1.Date,30);
        string doctor_ID(a1.doctor_ID,30);

        if (a1.ID[0] == '*') {
            continue;
        }
        anothergroup2[doctor_ID].push_back(id);
    }
    ofstream  appointmentsdoctornameindex("appointmentsdoctorname.dat", ios::binary);
    ofstream  appointmentsdoctornamelists("appointmentsdoctornamelist.dat", ios::binary);
    if (!appointmentsdoctornamelists) {
        cout << "File not found!" << endl;
    }
    if (!appointmentsdoctornamelists) {
        cout << "File not found!" << endl;
    }
    for (auto it = anothergroup2.begin(); it != anothergroup2.end(); it++) {
        int head = (int)appointmentsdoctornamelists.tellp();
        for (int i=0; i<it->second.size(); i++) {
            string primaryindex = it->second[i];
            int nextOffset = -1;
            if (i + 1 < (int)it->second.size()) {
                if (nextOffset ==-1) {
                    nextOffset = 19;
                }
                else {
                    nextOffset = nextOffset +19;
                }
            }
            appointmentsdoctornamelists.write(primaryindex.c_str(), 15);
            appointmentsdoctornamelists.write((char*)&nextOffset, 4);
        }
        appointmentsdoctornameindex.write(it->first.c_str(), 15);
        appointmentsdoctornameindex.write((char*)&head, sizeof(int));
    }
    appointmentsdoctornameindex.close();
    appointmentsdoctornamelists.close();
    file5.close();
    return  0;
}