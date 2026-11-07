#include <iostream>
#include <fstream>
using namespace std;

struct Doctor {
    char id[15];
    char name[30];
    char specialty[20];
    char address[30];
};


struct Appointment {
    char id[15];
    char date[30];
    char doctorID[15];
};


void initializeFile(const char* filename) {

    fstream file(filename, ios::in | ios::out | ios::binary);

    if (!file) {
        ofstream newFile(filename, ios::binary);

        int header = -1;

        newFile.write((char*)&header, sizeof(int));

        newFile.close();
    }
    else {
        file.seekg(0, ios::end);
        if (file.tellg() == 0) {
            int header = -1;
            file.seekp(0, ios::beg);
            file.write((char*)&header, sizeof(int));
        }
        file.close();
    }
}

void addDoctor(Doctor d) {

    fstream file("Doctors.dat",ios::in | ios::out | ios::binary);

    if (!file) {
        cout << "Cannot open Doctors.dat\n";
        return;
    }

    file.seekg(0, ios::end);
    int fileSize = file.tellg();

    Doctor temp;
    bool exists = false;

    for (int pos = 4; pos + sizeof(Doctor) <= fileSize;
         pos += sizeof(Doctor)) {

        file.seekg(pos, ios::beg);
        file.read((char*)&temp, sizeof(Doctor));

        if (temp.id[0] != '*' &&
            strcmp(temp.id, d.id) == 0) {
            exists = true;
            break;
            }
         }

    if (exists) {
        cout << "Doctor already exists!\n";
        file.close();
        return;
    }

    int header;

    file.seekg(0, ios::beg);
    file.read((char*)&header, sizeof(int));

    if (header != -1) {
        int next;

        file.seekg(header + 1, ios::beg);
        file.read((char*)&next, sizeof(int));

        file.seekp(0, ios::beg);
        file.write((char*)&next, sizeof(int));

        file.seekp(header, ios::beg);
        file.write((char*)&d, sizeof(Doctor));

        cout << "Doctor added using AVAIL LIST!\n";
    }

    else {
        file.clear();
        file.seekp(0, ios::end);
        file.write((char*)&d, sizeof(Doctor));

        cout << "Doctor added at the end of the file!\n";
    }
    file.close();
}

void addAppointment(Appointment a) {

    fstream file("Appointments.dat",ios::in | ios::out | ios::binary);

    if (!file) {
        return;
    }

    file.seekg(0, ios::end);
    int fileSize = file.tellg();

    Appointment temp;
    bool exists = false;

    for (int pos = 4; pos + sizeof(Appointment) <= fileSize;
         pos += sizeof(Appointment)) {

        file.seekg(pos, ios::beg);
        file.read((char*)&temp, sizeof(Appointment));

        // Ignore deleted appointments
        if (temp.id[0] != '*' &&
            strcmp(temp.id, a.id) == 0) {

            exists = true;
            break;
            }
         }

    if (exists) {
        cout << "Appointment already exists!\n";
        file.close();
        return;
    }


    int header;

    file.seekg(0, ios::beg);
    file.read((char*)&header, sizeof(int));


    if (header != -1) {

        int next;

        file.seekg(header + 1, ios::beg);
        file.read((char*)&next, sizeof(int));

        file.seekp(0, ios::beg);
        file.write((char*)&next, sizeof(int));

        file.seekp(header, ios::beg);
        file.write((char*)&a, sizeof(Appointment));
    }
    else {
        file.clear();
        file.seekp(0, ios::end);
        file.write((char*)&a, sizeof(Appointment));

    }

    file.close();
}


void deleteDoctor(const char* doctorID) {

    fstream file("Doctors.dat",
                 ios::in | ios::out | ios::binary);

    if (!file) {
        cout << "Cannot open Doctors.dat\n";
        return;
    }


    file.seekg(0, ios::end);
    int fileSize = file.tellg();

    Doctor d;
    int position = -1;


    for (int pos = 4; pos + sizeof(Doctor) <= fileSize;
         pos += sizeof(Doctor)) {

        file.seekg(pos, ios::beg);
        file.read((char*)&d, sizeof(Doctor));

        if (d.id[0] != '*' &&
            strcmp(d.id, doctorID) == 0) {

            position = pos;
            break;
            }
         }

    if (position == -1) {
        cout << "Warning: Doctor does not exist!\n";
        file.close();
        return;
    }
    int header;

    file.seekg(0, ios::beg);
    file.read((char*)&header, sizeof(int));

    char marker = '*';

    file.seekp(position, ios::beg);
    file.write(&marker, sizeof(char));


    file.seekp(position + 1, ios::beg);
    file.write((char*)&header, sizeof(int));


    file.seekp(0, ios::beg);
    file.write((char*)&position, sizeof(int));

    file.close();

    cout << "Doctor deleted successfully!\n";
}


void deleteAppointment(const char* appointmentID) {

    fstream file("Appointments.dat",ios::in | ios::out | ios::binary);

    if (!file) {
        cout << "Cannot open Appointments.dat\n";
        return;
    }
    

    file.seekg(0, ios::end);
    int fileSize = file.tellg();

    Appointment a;
    int position = -1;
    

    for (int pos = 4; pos + sizeof(Appointment) <= fileSize;
         pos += sizeof(Appointment)) {

        file.seekg(pos, ios::beg);
        file.read((char*)&a, sizeof(Appointment));

        
        if (a.id[0] != '*' &&
            strcmp(a.id, appointmentID) == 0) {

            position = pos;
            break;
            }
         }


    if (position == -1) {
        cout << "Warning: Appointment does not exist!\n";
        file.close();
        return;
    }


    int header;

    file.seekg(0, ios::beg);
    file.read((char*)&header, sizeof(int));


    char marker = '*';

    file.seekp(position, ios::beg);
    file.write(&marker, sizeof(char));
    

    file.seekp(position + 1, ios::beg);
    file.write((char*)&header, sizeof(int));


    file.seekp(0, ios::beg);
    file.write((char*)&position, sizeof(int));

    file.close();

    cout << "Appointment deleted successfully!\n";
}




int main() {



    return 0;
}
