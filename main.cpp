// ============================================================================
//  IS321 - Assignment 1 : Healthcare Management System
//  main.cpp  -  Welcome screen + Add / Update / Delete / Print / Query
//
//  Build (all .cpp files of the project together, C++17):
//    g++ -std=c++17 -o healthcare main.cpp QueryEngine.cpp Appointment.cpp
//        BufferFile.cpp Doctor.cpp FixedFieldBuffer.cpp PrimaryIndex.cpp SecondaryIndex.cpp
//  (one command line - the second line above just continues the first)
//
//  Design: ONE QueryEngine object owns the data files (RecordFile / BufferFile with the
//  AVAIL LIST header), the primary indexes and the secondary indexes. Every menu option
//  goes through it, so the query engine and the menu always see the same in-memory indexes.
// ============================================================================
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>
#include "QueryEngine.h"
using namespace std;

const string DOC_DAT = "data/doctors.dat";
const string APP_DAT = "data/appointments.dat";

const size_t ID_LEN = sizeof(Doctor::id), DNAME_LEN = sizeof(Doctor::name),
             SPEC_LEN = sizeof(Doctor::specialty), ADDR_LEN = sizeof(Doctor::address),
             DATE_LEN = sizeof(Appointments::Date);

// ============================================================================
//  1. INPUT HELPERS
// ============================================================================
bool inputClosed = false;   // stdin reached EOF -> stop the menu loop

string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string readLine(const string& promptText) {
    cout << promptText;
    string s;
    if (!getline(cin, s)) { inputClosed = true; return ""; }
    return trim(s);
}

bool askYesNo(const string& question) {
    string a = readLine(question + " (y/n): ");
    return !a.empty() && (a[0] == 'y' || a[0] == 'Y');
}

// Prompt for one field and validate it (the engine would silently truncate).
//   required: may not be empty      isKey: may not start with '*' (the deleted marker)
bool askField(const string& label, size_t maxLen, string& out,
              bool required = true, bool isKey = false) {
    out = readLine(label + ": ");
    if (inputClosed) return false;
    if (required && out.empty()) {
        cout << "  ! " << label << " cannot be empty. Operation cancelled.\n";
        return false;
    }
    if (out.size() > maxLen) {
        cout << "  ! " << label << " is too long (max " << maxLen << " characters, got "
             << out.size() << "). Operation cancelled.\n";
        return false;
    }
    if (isKey && !out.empty() && out[0] == '*') {
        cout << "  ! " << label << " cannot start with '*' (reserved as the deleted marker).\n";
        return false;
    }
    return true;
}

void setField(char* dest, size_t size, const string& s) {
    memset(dest, ' ', size);
    memcpy(dest, s.data(), min(size, s.size()));
}

void printAvail(const RecordFile& f) {
    cout << "  (AVAIL LIST head in the file header is now " << f.GetAvailHead() << ")\n";
}

// ============================================================================
//  3. MENU OPERATIONS
// ============================================================================

// ---- Add New Doctor ---------------------------------------------------------
void addDoctor(QueryEngine& db) {
    string id, name, spec, addr;
    if (!askField("Doctor ID", ID_LEN, id, true, true)) return;
    if (!askField("Doctor Name", DNAME_LEN, name)) return;
    if (!askField("Doctor Specialty", SPEC_LEN, spec)) return;
    if (!askField("Address", ADDR_LEN, addr, false)) return;

    bool willReuse = db.GetDoctorFile().GetAvailHead() != -1;
    if (!db.AddDoctor(id, name, spec, addr)) { cout << "  ! Doctor not added.\n"; return; }
    db.SaveIndexes();
    cout << "  Doctor added at byte offset " << db.GetDocPrimaryIndex().Search(id)
         << (willReuse ? " (reused a deleted slot from the AVAIL LIST).\n"
                       : " (appended to the end of the file).\n");
    printAvail(db.GetDoctorFile());
}

// ---- Add New Appointment ----------------------------------------------------
void addAppointment(QueryEngine& db) {
    string id, date, docId;
    if (!askField("Appointment ID", ID_LEN, id, true, true)) return;
    if (!askField("Appointment Date", DATE_LEN, date)) return;
    if (!askField("Doctor ID", ID_LEN, docId, true, true)) return;

    bool willReuse = db.GetAppointmentFile().GetAvailHead() != -1;
    if (!db.AddAppointment(id, date, docId)) { cout << "  ! Appointment not added.\n"; return; }
    db.SaveIndexes();
    cout << "  Appointment added at byte offset " << db.GetAppPrimaryIndex().Search(id)
         << (willReuse ? " (reused a deleted slot from the AVAIL LIST).\n"
                       : " (appended to the end of the file).\n");
    printAvail(db.GetAppointmentFile());
}

// ---- Update Doctor Name (non-key field, but a SECONDARY key) ----------------
void updateDoctorName(QueryEngine& db) {
    string id, newName;
    if (!askField("Doctor ID", ID_LEN, id, true, true)) return;
    Doctor doc;
    if (!db.SearchDoctorByID(id, doc)) { cout << "  ! Doctor '" << id << "' not found.\n"; return; }
    if (!askField("New Doctor Name", DNAME_LEN, newName)) return;

    // In-place rewrite; the engine also moves the doctor from the old name's list to the new one.
    if (!db.UpdateDoctor(id, newName, "", "")) { cout << "  ! Update failed.\n"; return; }
    db.SaveIndexes();
    cout << "  Doctor name updated in place (name index updated).\n";
}

// ---- Update Doctor ID (KEY update) ------------------------------------------
// This is the exception to the one-call pattern because QueryEngine has no primary-key
// update operation; the existing index, file, appointment, and secondary-index APIs do
// each part of the change.
void updateDoctorID(QueryEngine& db) {
    string oldId, newId;
    if (!askField("Current Doctor ID", ID_LEN, oldId, true, true)) return;
    Doctor doc;
    if (!db.SearchDoctorByID(oldId, doc)) { cout << "  ! Doctor '" << oldId << "' not found.\n"; return; }
    if (!askField("New Doctor ID", ID_LEN, newId, true, true)) return;
    if (newId == oldId) { cout << "  New ID is the same as the old one. Nothing to do.\n"; return; }

    int offset = db.GetDocPrimaryIndex().Search(oldId);
    string name = QueryEngine::TrimField(doc.name, DNAME_LEN);
    string spec = QueryEngine::TrimField(doc.specialty, SPEC_LEN);
    vector<Appointments> apps = db.SearchAppointmentsByDoctorID(oldId);

    int from = -1, to = -1;
    if (!db.GetDocPrimaryIndex().UpdateKey(oldId, newId, &from, &to)) {
        cout << "  ! Primary index update failed.\n";
        return;
    }

    // Rewrite the record at its original offset after moving its primary-index key.
    setField(doc.id, ID_LEN, newId);
    doc.Pack(db.GetDocBuffer());
    if (db.GetDoctorFile().Write(offset) == -1) {
        db.GetDocPrimaryIndex().UpdateKey(newId, oldId);
        cout << "  ! Could not write the doctor record.\n";
        return;
    }

    cout << "  (primary index: entry shifted from position " << from << " to position " << to << ")\n";

    for (const Appointments& a : apps)
        db.UpdateAppointment(QueryEngine::TrimField(a.ID, ID_LEN), "", newId);

    // Secondary indexes are bound to the primary key, so replace the old ID there too.
    db.GetDocSecNameIndex().Remove(name.c_str(), oldId.c_str());
    db.GetDocSecNameIndex().Insert(name.c_str(), newId.c_str());
    db.GetDocSecSpecIndex().Remove(spec.c_str(), oldId.c_str());
    db.GetDocSecSpecIndex().Insert(spec.c_str(), newId.c_str());

    db.SaveIndexes();
    cout << "  Doctor ID changed from '" << oldId << "' to '" << newId << "'";
    if (!apps.empty()) cout << " (" << apps.size() << " appointment(s) updated too)";
    cout << ".\n";
}

// ---- Update Appointment Date (plain non-key field) --------------------------
void updateAppointmentDate(QueryEngine& db) {
    string id, newDate;
    if (!askField("Appointment ID", ID_LEN, id, true, true)) return;
    Appointments app;
    if (!db.SearchAppointmentByID(id, app)) { cout << "  ! Appointment '" << id << "' not found.\n"; return; }
    if (!askField("New Appointment Date", DATE_LEN, newDate)) return;
    if (!db.UpdateAppointment(id, newDate, "")) { cout << "  ! Update failed.\n"; return; }
    cout << "  Appointment date updated in place (no index change needed).\n";
}

// ---- Delete Appointment -----------------------------------------------------
void deleteAppointment(QueryEngine& db) {
    string id;
    if (!askField("Appointment ID", ID_LEN, id, true, true)) return;
    Appointments app;
    if (!db.SearchAppointmentByID(id, app)) { cout << "  WARNING: Appointment '" << id << "' does not exist. Nothing deleted.\n"; return; }
    int off = db.GetAppPrimaryIndex().Search(id);
    if (!db.DeleteAppointment(id)) { cout << "  ! Could not delete.\n"; return; }
    db.SaveIndexes();
    cout << "  Appointment '" << id << "' deleted (slot at offset " << off << " added to the AVAIL LIST).\n";
    printAvail(db.GetAppointmentFile());
}

// ---- Delete Doctor ----------------------------------------------------------
// Policy: if the doctor still has appointments, the user chooses between cancelling and
// deleting those appointments as well.
void deleteDoctor(QueryEngine& db) {
    string id;
    if (!askField("Doctor ID", ID_LEN, id, true, true)) return;
    Doctor doc;
    if (!db.SearchDoctorByID(id, doc)) { cout << "  WARNING: Doctor '" << id << "' does not exist. Nothing deleted.\n"; return; }
    int off = db.GetDocPrimaryIndex().Search(id);

    vector<Appointments> apps = db.SearchAppointmentsByDoctorID(id);
    if (!apps.empty()) {
        cout << "  Doctor '" << id << "' has " << apps.size() << " appointment(s).\n";
        if (!askYesNo("  Delete those appointments as well?")) { cout << "  Delete cancelled.\n"; return; }
        for (const Appointments& a : apps) db.DeleteAppointment(QueryEngine::TrimField(a.ID, ID_LEN));
    }
    if (!db.DeleteDoctor(id)) { cout << "  ! Could not delete.\n"; db.SaveIndexes(); return; }
    db.SaveIndexes();
    cout << "  Doctor '" << id << "' deleted (slot at offset " << off << " added to the AVAIL LIST).\n";
    printAvail(db.GetDoctorFile());
}

// ---- Print Doctor / Appointment ---------------------------------------------
void printDoctorInfo(QueryEngine& db) {
    string id;
    if (!askField("Doctor ID", ID_LEN, id, true, true)) return;
    db.From(Query::Tokens::Table::Doctors)
      .Where(Query::Tokens::Field::DoctorID, Query::Tokens::Op::Equals, string(id))
      .Select(Query::Tokens::Field::All).Print();
}

void printAppointmentInfo(QueryEngine& db) {
    string id;
    if (!askField("Appointment ID", ID_LEN, id, true, true)) return;
    db.From(Query::Tokens::Table::Appointments)
      .Where(Query::Tokens::Field::AppointmentID, Query::Tokens::Op::Equals, string(id))
      .Select(Query::Tokens::Field::All).Print();
}

// ---- Write Query ------------------------------------------------------------
void writeQuery(QueryEngine& db) {
    string q = readLine("Enter query: ");
    if (q.empty()) { cout << "  Empty query.\n"; return; }
    QueryResult r = db.Execute(q);
    r.Print();
}

// ============================================================================
//  4. SAMPLE DATA (assignment: 5 doctors + 10 appointments)
// ============================================================================
void loadSampleData(QueryEngine& db) {
    struct D { const char *id, *name, *spec, *addr; };
    struct A { const char *id, *date, *doc; };
    const D docs[] = {
        {"D1", "Ali",  "Cardio", "Cairo"}, {"D2", "Omar", "Neuro", "Giza"},
        {"D3", "Mona", "Derma",  "Alex"},  {"D4", "Sara", "Cardio", "Tanta"},   // same specialty as D1
        {"D5", "Ali",  "Ortho",  "Aswan"}                                       // same name as D1
    };
    const A apps[] = {
        {"A1",  "2026-11-15 09:00", "D1"}, {"A2",  "2026-11-15 10:30", "D2"},
        {"A3",  "2026-11-16 11:00", "D1"}, {"A4",  "2026-11-16 12:15", "D3"},
        {"A5",  "2026-11-17 09:45", "D4"}, {"A6",  "2026-11-17 14:00", "D5"},
        {"A7",  "2026-11-18 08:30", "D2"}, {"A8",  "2026-11-18 13:20", "D1"},
        {"A9",  "2026-11-19 10:00", "D3"}, {"A10", "2026-11-19 15:40", "D4"}
    };
    for (const D& d : docs) db.AddDoctor(d.id, d.name, d.spec, d.addr);
    for (const A& a : apps) db.AddAppointment(a.id, a.date, a.doc);
    db.SaveIndexes();
    cout << "  Loaded 5 doctors and 10 appointments.\n";
}

// ============================================================================
//  5. MENU + MAIN
// ============================================================================
void showMenu() {
    cout << "\n==============================================\n"
         << "      HEALTHCARE MANAGEMENT SYSTEM\n"
         << "==============================================\n"
         << "  1. Add New Doctor\n"
         << "  2. Add New Appointment\n"
         << "  3. Update Doctor Name (Doctor ID)\n"
         << "  4. Update Doctor ID [Key Update Test]\n"
         << "  5. Update Appointment Date (Appointment ID)\n"
         << "  6. Delete Appointment (Appointment ID)\n"
         << "  7. Delete Doctor (Doctor ID)\n"
         << "  8. Print Doctor Info (Doctor ID)\n"
         << "  9. Print Appointment Info (Appointment ID)\n"
         << " 10. Write Query\n"
         << " 11. Exit\n"
         << "----------------------------------------------\n";
}

int main() {
    error_code ec;
    filesystem::create_directories("data", ec);     // BufferFile::Open needs the folder to exist
    filesystem::create_directories("indexes", ec);  // index files folder

    QueryEngine db(DOC_DAT, APP_DAT);      // opens both data files and loads the saved indexes
    db.RebuildIndexes();
    db.SaveIndexes();
    bool fresh = db.GetDocPrimaryIndex().IsEmpty() && db.GetAppPrimaryIndex().IsEmpty();

    if (fresh) {
        cout << "No data found.\n";
        if (askYesNo("Load the sample data (5 doctors, 10 appointments)?")) loadSampleData(db);
    }

    while (!inputClosed) {
        showMenu();
        string line = readLine("Choose an option (1-11): ");
        if (inputClosed) break;

        int choice = 0;
        try { choice = stoi(line); } catch (...) { choice = 0; }

        switch (choice) {
            case 1:  addDoctor(db);            break;
            case 2:  addAppointment(db);       break;
            case 3:  updateDoctorName(db);     break;
            case 4:  updateDoctorID(db);       break;
            case 5:  updateAppointmentDate(db);break;
            case 6:  deleteAppointment(db);    break;
            case 7:  deleteDoctor(db);         break;
            case 8:  printDoctorInfo(db);      break;
            case 9:  printAppointmentInfo(db); break;
            case 10: writeQuery(db);           break;
            case 11:
                db.SaveIndexes();
                cout << "Indexes saved. Goodbye.\n";
                return 0;
            default:
                cout << "  Invalid option, please enter a number from 1 to 11.\n";
        }
    }
    db.SaveIndexes();
    return 0;
}