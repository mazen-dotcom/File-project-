#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
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
struct index {
    char id[15];
    int offset;
};
struct nameindex {
    char name[30];
    int offset;
};
struct specialtyindex {
    char specialty[20];
    int offset;
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
void bubblesortForname(vector<nameindex>& v) {
    bool swapped = false;
    for (int i = 0; i < v.size()-1; i++) {
        for (int j = 0; j < v.size()-i-1; j++) {
            string name1(v[j].name, 30);
            string name2(v[j+1].name, 30);
            if (name1 > name2) {
                swapped = true;
                swap(v[j], v[j+1]);
            }//end if
        }//end loop
        if (!swapped) {
            break;
        }
    }//loop end
}
void bubblesortForspecality(vector<specialtyindex>& v) {
    bool swapped = false;
    for (int i = 0; i < v.size()-1; i++) {
        for (int j = 0; j < v.size()-i-1; j++) {
            string specialty1(v[j].specialty, 20);
            string specialty2(v[j+1].specialty, 20);
            if (specialty1 > specialty2) {
                swapped = true;
                swap(v[j], v[j+1]);
            }//end if
        }// loop end
        if (!swapped) {
            break;
        }
    }// end loop
}
// ---------------------------search function
int  primary_index_doctor_search(string  IndexFile, string doctor_ID) { /* return -1 =>not found
                                                                            return -2 => fail to open file*/
    ifstream file(IndexFile.c_str(), ios::binary);
    if (!file) {
        cout << "error in opening the index file in search!" << endl;
        return -2;
    }
    file.seekg(0, ios::end);// move the pointer to the end of the file

    int fileSize = file.tellg();//get the last byte offset to get the size of the file

    int doctors_num = fileSize / 19; //calc num of doctors_id in the file
    int low=0;
    int high=doctors_num-1;
    char id[15];
    int recorddoffset;
    while (low <= high) {  // normal binary search
        int mid = (low+high)/2;
        int postion =mid*19; // to get its exact byte offset of the mid
        file.seekg(postion,ios::beg);
        file.read((char*)&id, sizeof(id));
        file.read((char*)&recorddoffset, sizeof(int));
        string unpadded_id(id,15);
            while (!unpadded_id.empty() && (unpadded_id.back() == ' ' || unpadded_id.back() == '\0')) { // remove the padding spaces before comapring
                unpadded_id.pop_back();
       }// loop end
            if (unpadded_id==doctor_ID) {
                return recorddoffset;// return the byte offset of this id
            }else if (unpadded_id<doctor_ID) {
                low = mid + 1;
            }else if (unpadded_id>doctor_ID) {
                high = mid - 1;
            }


        }// loop end
    return  -1;

}
// -------------------------------------primary search on Appointments index
int primaryindex_Appointments_search(string  IndexFile, string Appointments_ID) {
    /* return -1 =>not found
                                                                            return -2 => fail to open file*/
    ifstream file(IndexFile.c_str(), ios::binary);
    if (!file) {
        cout << "error in opening the index file in search!" << endl;
        return -2;
    }
    file.seekg(0, ios::end);// move the pointer to the end of the file

    int fileSize = file.tellg();//get the last byte offset to get the size of the file

    int Appointments_num = fileSize / 19; //calc num of doctors_id in the file
    int low=0;
    int high=Appointments_num-1;
    char id[15];
    int recorddoffset;
    while (low <= high) {  // normal binary search
        int mid = (low+high)/2;
        int postion =mid*19; // to get its exact byte offset of the mid
        file.seekg(postion,ios::beg);
        file.read((char*)&id, sizeof(id));
        file.read((char*)&recorddoffset, sizeof(int));
        string unpadded_id(id,15);
        while (!unpadded_id.empty() && (unpadded_id.back() == ' ' || unpadded_id.back() == '\0')) { // remove the padding spaces before comapring
            unpadded_id.pop_back();
        }// loop end
        if (unpadded_id==Appointments_ID) {
            return recorddoffset;// return the byte offset of this id
        }else if (unpadded_id<Appointments_ID) {
            low = mid + 1;
        }else if (unpadded_id>Appointments_ID) {
            high = mid - 1;
        }


    }// loop end
    return  -1;
}
// ---------------------------------------secondary search on doctor name
vector<string>  secondaryindex_doctorname_search(string  IndexFile,string  listfile, string Doctors_name) {
    ifstream file(IndexFile.c_str(), ios::binary);
    if (!file) {
        cout << "error in opening the index file in search!" << endl;
    }else {
        file.seekg(0, ios::end);
        int fileSize = file.tellg();
       int doctors_name_num = fileSize / 34;
        int low=0;
        int high=doctors_name_num-1;
        char name[30];
        bool found = false; // use it to check if i found them or not
        int head_of_list;
        int postion;
        while (low <= high) {
            int mid = (low+high)/2;
             postion =mid*34;
            file.seekg(postion,ios::beg);
            file.read((char*)&name, sizeof(name));
            file.read((char*)&head_of_list, sizeof(head_of_list));
            string unpadded_name(name,30);
            while (!unpadded_name.empty() && (unpadded_name.back() == ' ' || unpadded_name.back() == '\0')) { // remove the padding spaces before comapring
                unpadded_name.pop_back();
            }// loop end
            if (unpadded_name==Doctors_name) {
                found = true;
                break;
            }else if (unpadded_name<Doctors_name) {
                low = mid + 1;
            }else if (unpadded_name>Doctors_name) {
                high = mid - 1;
            }//end if
        }//loop end
         if (found)
         {
             ifstream list(listfile,ios::binary);
        if (!list) {
            cout << "error in opening the list file in search!" << endl;
        }else {
            // string doctor_id[15];
            // int nextoffset;
            list.seekg(head_of_list, ios::beg);
            vector<string> v;
            index temp;
            char id[15];
            int  nextoffset=0;
            while (nextoffset != -1) {
                if (nextoffset==0) {
                    list.read((char*)&id, sizeof(id));
                    list.read((char*)&nextoffset, sizeof(nextoffset));
                    string unpadded_id(id,15);
                    while (!unpadded_id.empty() && (unpadded_id.back() == ' ' || unpadded_id.back() == '\0')) {
                        // remove the padding spaces before comapring
                        unpadded_id.pop_back();
                    }
                    v.push_back(unpadded_id);

                }else {
                    list.seekg(nextoffset, ios::beg);
                    list.read((char*)&id, sizeof(id));
                    list.read((char*)&nextoffset, sizeof(nextoffset));
                    string unpadded_id(id,15);
                    while (!unpadded_id.empty() && (unpadded_id.back() == ' ' || unpadded_id.back() == '\0')) {
                        // remove the padding spaces before comapring
                        unpadded_id.pop_back();
                    }
                    v.push_back(unpadded_id);
                }//end if

            }//loop end
            return v;
        }//end if
            }else {
                cout<<"the value does not found";
            }
    }//end if
}
vector<string> secondaryindex_doctorspecialty_search(string  IndexFile,string  listfile, string Doctors_specialty) {
        ifstream file(IndexFile.c_str(), ios::binary);
    if (!file) {
        cout << "error in opening the index file in search!" << endl;
    }else {
        file.seekg(0, ios::end);
        int fileSize = file.tellg();
       int doctors_name_num = fileSize / 24;
        int low=0;
        int high=doctors_name_num-1;
        char name[20];
        bool found = false; // use it to check if i found them or not
        int head_of_list;
        int postion;
        while (low <= high) {
            int mid = (low+high)/2;
             postion =mid*24;
            file.seekg(postion,ios::beg);
            file.read((char*)&name, sizeof(name));
            file.read((char*)&head_of_list, sizeof(head_of_list));
            string unpadded_name(name,20);
            while (!unpadded_name.empty() && (unpadded_name.back() == ' ' || unpadded_name.back() == '\0')) { // remove the padding spaces before comapring
                unpadded_name.pop_back();
            }// loop end
            if (unpadded_name==Doctors_specialty) {
                found = true;
                break;
            }else if (unpadded_name<Doctors_specialty) {
                low = mid + 1;
            }else if (unpadded_name>Doctors_specialty) {
                high = mid - 1;
            }//end if
        }//loop end
         if (found)
         {
             ifstream list(listfile,ios::binary);
        if (!list) {
            cout << "error in opening the list file in search!" << endl;
        }else {
            // string doctor_id[15];
            // int nextoffset;
            list.seekg(head_of_list, ios::beg);
            vector<string> v;
            index temp;
            char id[15];
            int  nextoffset=0;
            while (nextoffset != -1) {
                if (nextoffset==0) {
                    list.read((char*)&id, sizeof(id));
                    list.read((char*)&nextoffset, sizeof(nextoffset));
                    string unpadded_id(id,15);
                    while (!unpadded_id.empty() && (unpadded_id.back() == ' ' || unpadded_id.back() == '\0')) {
                        // remove the padding spaces before comapring
                        unpadded_id.pop_back();
                    }
                    v.push_back(unpadded_id);

                }else {
                    list.seekg(nextoffset, ios::beg);
                    list.read((char*)&id, sizeof(id));
                    list.read((char*)&nextoffset, sizeof(nextoffset));
                    string unpadded_id(id,15);
                    while (!unpadded_id.empty() && (unpadded_id.back() == ' ' || unpadded_id.back() == '\0')) {
                        // remove the padding spaces before comapring
                        unpadded_id.pop_back();
                    }
                    v.push_back(unpadded_id);
                }//end if

            }//loop end
            return v;
        }//end if
            }else {
                cout<<"the value does not found";
            }
    }//end if
}
// ----------------------------------------------secondary index of doctor_id in apporpment index
vector<string> secondaryindex_doctorid_Appointments_search(string  IndexFile,string  listfile, string Doctors_id) {
           ifstream file(IndexFile.c_str(), ios::binary);
    if (!file) {
        cout << "error in opening the index file in search!" << endl;
    }else {
        file.seekg(0, ios::end);
        int fileSize = file.tellg();
       int doctors_name_num = fileSize / 19;
        int low=0;
        int high=doctors_name_num-1;
        char name[15];
        bool found = false; // use it to check if i found them or not
        int head_of_list;
        int postion;
        while (low <= high) {
            int mid = (low+high)/2;
             postion =mid*19;
            file.seekg(postion,ios::beg);
            file.read((char*)&name, sizeof(name));
            file.read((char*)&head_of_list, sizeof(head_of_list));
            string unpadded_name(name,15);
            while (!unpadded_name.empty() && (unpadded_name.back() == ' ' || unpadded_name.back() == '\0')) { // remove the padding spaces before comapring
                unpadded_name.pop_back();
            }// loop end
            if (unpadded_name==Doctors_id) {
                found = true;
                break;
            }else if (unpadded_name<Doctors_id) {
                low = mid + 1;
            }else if (unpadded_name>Doctors_id) {
                high = mid - 1;
            }//end if
        }//loop end
         if (found)
         {
             ifstream list(listfile,ios::binary);
        if (!list) {
            cout << "error in opening the list file in search!" << endl;
        }else {
            // string doctor_id[15];
            // int nextoffset;
            list.seekg(head_of_list, ios::beg);
            vector<string> v;
            index temp;
            char id[15];
            int  nextoffset=0;
            while (nextoffset != -1) {
                if (nextoffset==0) {
                    list.read((char*)&id, sizeof(id));
                    list.read((char*)&nextoffset, sizeof(nextoffset));
                    string unpadded_id(id,15);
                    while (!unpadded_id.empty() && (unpadded_id.back() == ' ' || unpadded_id.back() == '\0')) {
                        // remove the padding spaces before comapring
                        unpadded_id.pop_back();
                    }
                    v.push_back(unpadded_id);

                }else {
                    list.seekg(nextoffset, ios::beg);
                    list.read((char*)&id, sizeof(id));
                    list.read((char*)&nextoffset, sizeof(nextoffset));
                    string unpadded_id(id,15);
                    while (!unpadded_id.empty() && (unpadded_id.back() == ' ' || unpadded_id.back() == '\0')) {
                        // remove the padding spaces before comapring
                        unpadded_id.pop_back();
                    }
                    v.push_back(unpadded_id);
                }//end if

            }//loop end
            return v;
        }//end if
            }else {
                cout<<"the value does not found";
            }
    }//end if
}
// -----------------------------------------creating primary index over doctor id in doctors files
    string  primaryindex_doctor_id_doctors(string  doctors_file,string  NameOfIndex_file) {
        ifstream file(doctors_file, ios::binary); // open data file
        ofstream primaryindex(NameOfIndex_file, ios::binary); // create the index file
        if (!file || !primaryindex) {
            return  "error in opening the index file in search!" ;
        }
        int header;
        file.read((char*)&header, sizeof(header));// skip the first 4 byte of the header
        Doctor d;

    while (true) {
        int offset = file.tellg();// get the byteoffset for first record

        if (!file.read((char*)&d, sizeof(d)))
            break;

        if (d.id[0] == '*')
            continue;

        primaryindex.write(d.id, 15);
        primaryindex.write((char*)&offset, sizeof(int));
    }
        primaryindex.close();
        file.close();
        ifstream unsortindex(NameOfIndex_file, ios::binary); //open the file again to sort them
        if (!unsortindex) {
            cout << "Sort index not found!" << endl;
        }
        vector<index> v;
        index temp;
        while (unsortindex.read(temp.id, 15)) {
            if (!unsortindex.read((char*)&temp.offset, 4))
                 break;

        v.push_back(temp);
    }
        unsortindex.close();
        bubblesort(v);
        ofstream sortedindex(NameOfIndex_file, ios::binary); // open them again for edit

        for (int i = 0; i < v.size(); i++) {
            sortedindex.write((char*)&v[i].id, sizeof(v[i].id));
            sortedindex.write((char*)&v[i].offset, sizeof(v[i].offset));
        }
        sortedindex.close();
        return  NameOfIndex_file;
    }
    // -------------------------------------------------- creating  secondary index for doctor name
    void  secondaryindex_doctorname_doctors(string  doctors_file,string  NameOfIndex_file, string nameoflistfile) {
        ifstream file2(doctors_file, ios::binary);
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
        ofstream doctornames(NameOfIndex_file,ios::binary);
        ofstream doctorlists(nameoflistfile,ios::binary);
        for (auto it = group.begin(); it != group.end(); it++) {
            int head = (int)doctorlists.tellp();
            int nextOffset = -1;
            for (int i=0; i<it->second.size(); i++) {
                string primaryindex = it->second[i];

                int nodeOffset = doctorlists.tellp();
                int nextOffset = -1;

                if (i + 1 < (int)it->second.size()) {
                    nextOffset = nodeOffset + 19;
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

    }
    // ----------------------------------------------------creating secondary index doctor specality doctor
    void secondaryindex_doctorspecality_doctors (string doctors_file,string  NameOfIndex_file, string nameoflistfile) {
        ifstream file4(doctors_file, ios::binary);
        if (!file4) {
            cout << "File not found!" << endl;
        }
        ofstream doctorspecialtyindex(NameOfIndex_file, ios::binary);
        ofstream doctorspecialtylists(nameoflistfile, ios::binary);
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

                int nodeOffset = doctorspecialtylists.tellp();
                int nextOffset = -1;

                if (i + 1 < (int)it->second.size()) {
                    nextOffset = nodeOffset + 19;
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

    }
    // -------------------------------------------------creating primary index over Appointments
    string primaryindex_appointemtid_appoinment(string Appointments_file,string nameofindexfile) {
        ifstream  appointments(Appointments_file, ios::binary);
        ofstream  appointmentsprimaryindex(nameofindexfile, ios::binary);
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
    while (true) {
        int offset = appointments.tellg();

        if (!appointments.read((char*)&a, sizeof(a)))
            break;

        if (a.ID[0] == '*')
            continue;

        appointmentsprimaryindex.write(a.ID, 15);
        appointmentsprimaryindex.write((char*)&offset, sizeof(int));
    }
        appointmentsprimaryindex.close();
        ifstream unsortedappointmentsprimaryindex(nameofindexfile, ios::binary);
        if (!unsortedappointmentsprimaryindex) {
            cout << "File not found!" << endl;
        }

        vector<index> v1;
        index temp1;
        while (unsortedappointmentsprimaryindex.read(temp1.id, 15)) {
            if (!unsortedappointmentsprimaryindex.read((char*)&temp1.offset, 4))
                break;

            v1.push_back(temp1);
    }
        bubblesort(v1);
        unsortedappointmentsprimaryindex.close();
        ofstream sortedappointmentsprimaryindex(nameofindexfile, ios::binary);
        if (!sortedappointmentsprimaryindex) {
            cout << "File not found!" << endl;
        }
        for (int i=0; i<v1.size(); i++) {
            sortedappointmentsprimaryindex.write((char*)&v1[i].id,15);
            sortedappointmentsprimaryindex.write((char*)&v1[i].offset,4);
        }
        sortedappointmentsprimaryindex.close();
        return nameofindexfile;

    }
    // --------------------------------------------creating secondary index appointmemt over doctor id
    void secondaryindex_doctorid_appointments(string Appointments_file,string nameofindexfile, string nameoflist_file) {
        ifstream file5(Appointments_file, ios::binary);
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
            string doctor_ID(a1.doctor_ID,15);

            if (a1.ID[0] == '*') {
                continue;
            }
            anothergroup2[doctor_ID].push_back(id);
        }
        ofstream  appointmentsdoctoridindex(nameofindexfile, ios::binary);
        ofstream  appointmentsdoctoridlists(nameoflist_file, ios::binary);
        if (!appointmentsdoctoridindex) {
            cout << "File not found!" << endl;
        }
        if (!appointmentsdoctoridlists) {
            cout << "File not found!" << endl;
        }
        for (auto it = anothergroup2.begin(); it != anothergroup2.end(); it++) {
            int head = (int)appointmentsdoctoridlists.tellp();

            for (int i=0; i<it->second.size(); i++) {
                string primaryindex = it->second[i];
                int nodeOffset = appointmentsdoctoridlists.tellp();
                int nextOffset = -1;

                if (i + 1 < (int)it->second.size()) {
                    nextOffset = nodeOffset + 19;
                }

                appointmentsdoctoridlists.write(primaryindex.c_str(), 15);
                appointmentsdoctoridlists.write((char*)&nextOffset, 4);
            }
            appointmentsdoctoridindex.write(it->first.c_str(), 15);
            appointmentsdoctoridindex.write((char*)&head, sizeof(int));
        }
        appointmentsdoctoridindex.close();
        appointmentsdoctoridlists.close();
        file5.close();

    }
int main() {





    return  0;

    /*
     * i need to convert all of them into sperate functions => done
     * and adding the search function => done 
     */



}