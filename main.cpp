#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include "indexes.h"
using namespace std;

static_assert(sizeof(Doctor) == 95,       "Doctor record must be exactly 95 bytes");
static_assert(sizeof(Appointments) == 60, "Appointments record must be exactly 60 bytes");

// ----------------------------------------------------------------------------
//  File names (same layout as the architecture document)
// ----------------------------------------------------------------------------
const string DOC_DAT           = "data/doctors.dat";
const string APP_DAT           = "data/appointments.dat";
const string DOC_PRI           = "indexes/doctor_primary.ind";
const string APP_PRI           = "indexes/app_primary.ind";
const string APP_SEC_DOC       = "indexes/app_sec_docID.ind";
const string APP_SEC_DOC_LIST  = "indexes/app_sec_docID_list.dat";
const string DOC_SEC_NAME      = "indexes/doc_sec_name.ind";
const string DOC_SEC_NAME_LIST = "indexes/doc_sec_name_list.dat";
const string DOC_SEC_SPEC      = "indexes/doc_sec_spec.ind";
const string DOC_SEC_SPEC_LIST = "indexes/doc_sec_spec_list.dat";

// Field sizes (from the assignment)
const size_t ID_LEN = 15, DNAME_LEN = 30, SPEC_LEN = 20, ADDR_LEN = 30, DATE_LEN = 30;

// ============================================================================
//  1. FIELD + INPUT HELPERS
// ============================================================================
bool inputClosed = false;   // set when stdin hits EOF so the menu loop can stop

// Copy s into a fixed-size char field, padded with spaces (truncates if too long)
void setField(char* dest, size_t size, const string& s) {
    memset(dest, ' ', size);
    memcpy(dest, s.data(), min(size, s.size()));
}

// Fixed-size char field -> std::string without the trailing padding
string getField(const char* src, size_t size) {
    string s(src, size);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) s.pop_back();
    return s;
}

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

// Prompt for a field and validate it.  Returns false (after printing why) if invalid.
//   required : empty not allowed
//   isKey    : key fields may not start with '*' (that is the "deleted" marker)
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

// ============================================================================
//  2. DATA-FILE LAYER  (header record, AVAIL LIST, read / write at offset)
// ============================================================================

// Creates the file with header = -1 if it does not exist. Returns true if created.
bool initDataFile(const string& path) {
    ifstream test(path, ios::binary);
    if (test) return false;
    ofstream f(path, ios::binary);
    int h = -1;
    f.write(reinterpret_cast<const char*>(&h), sizeof(int));
    return true;
}

int readHeader(const string& path) {
    ifstream f(path, ios::binary);
    int h = -1;
    if (f) f.read(reinterpret_cast<char*>(&h), sizeof(int));
    return h;
}

void writeHeader(const string& path, int head) {
    fstream f(path, ios::in | ios::out | ios::binary);
    f.seekp(0, ios::beg);
    f.write(reinterpret_cast<const char*>(&head), sizeof(int));
}

template <typename T>
bool readRecordAt(const string& path, int offset, T& rec) {
    ifstream f(path, ios::binary);
    if (!f) return false;
    f.seekg(offset, ios::beg);
    f.read(reinterpret_cast<char*>(&rec), sizeof(T));
    return !f.fail();
}

template <typename T>
bool writeRecordAt(const string& path, int offset, const T& rec) {
    fstream f(path, ios::in | ios::out | ios::binary);
    if (!f) return false;
    f.seekp(offset, ios::beg);
    f.write(reinterpret_cast<const char*>(&rec), sizeof(T));
    return !f.fail();
}

// ADD: use the first slot of the AVAIL LIST if there is one, otherwise append.
// Returns the byte offset the record was written at (-1 on failure).
template <typename T>
int insertRecord(const string& path, const T& rec, bool& reusedSlot) {
    reusedSlot = false;
    int head = readHeader(path);

    if (head != -1) {
        T slot;
        if (readRecordAt(path, head, slot) && reinterpret_cast<char*>(&slot)[0] == '*') {
            int next;
            memcpy(&next, reinterpret_cast<char*>(&slot) + 1, sizeof(int));   // next free slot
            if (!writeRecordAt(path, head, rec)) return -1;
            writeHeader(path, next);          // AVAIL LIST now starts at the next free slot
            reusedSlot = true;
            return head;
        }
        cout << "  ! Warning: AVAIL LIST head points to a non-deleted slot. Appending instead.\n";
    }

    fstream f(path, ios::in | ios::out | ios::binary);
    if (!f) return -1;
    f.seekp(0, ios::end);
    int offset = static_cast<int>(f.tellp());
    f.write(reinterpret_cast<const char*>(&rec), sizeof(T));
    return f.fail() ? -1 : offset;
}

// DELETE: logical delete. '*' goes in byte 0 of the key, the old AVAIL head goes in
// bytes 1..4, and the header now points at this slot (push onto the AVAIL LIST).
template <typename T>
bool markDeleted(const string& path, int offset) {
    T rec;
    if (!readRecordAt(path, offset, rec)) return false;
    int oldHead = readHeader(path);
    char* raw = reinterpret_cast<char*>(&rec);
    raw[0] = '*';
    memcpy(raw + 1, &oldHead, sizeof(int));
    if (!writeRecordAt(path, offset, rec)) return false;
    writeHeader(path, offset);
    return true;
}

// ============================================================================
//  3. INDEX MAINTENANCE  (uses the teammate's builders)
// ============================================================================
void rebuildDoctorPrimary()          { primaryindex_doctor_id_doctors(DOC_DAT, DOC_PRI); }
void rebuildDoctorSecondary()        {
    secondaryindex_doctorname_doctors(DOC_DAT, DOC_SEC_NAME, DOC_SEC_NAME_LIST);
    secondaryindex_doctorspecality_doctors(DOC_DAT, DOC_SEC_SPEC, DOC_SEC_SPEC_LIST);
}
void rebuildAppointmentPrimary()     { primaryindex_appointemtid_appoinment(APP_DAT, APP_PRI); }
void rebuildAppointmentSecondary()   { secondaryindex_doctorid_appointments(APP_DAT, APP_SEC_DOC, APP_SEC_DOC_LIST); }

void rebuildDoctorIndexes()      { rebuildDoctorPrimary();      rebuildDoctorSecondary(); }
void rebuildAppointmentIndexes() { rebuildAppointmentPrimary(); rebuildAppointmentSecondary(); }
void rebuildAllIndexes()         { rebuildDoctorIndexes();      rebuildAppointmentIndexes(); }

// Lookups through the primary indexes (binary search, done by the teammate's code).
// Return the record's byte offset, or a negative number if not found.
int findDoctor(const string& id)      { return primary_index_doctor_search(DOC_PRI, id); }
int findAppointment(const string& id) { return primaryindex_Appointments_search(APP_PRI, id); }
bool doctorExists(const string& id)      { return findDoctor(id) >= 0; }
bool appointmentExists(const string& id) { return findAppointment(id) >= 0; }

// ---- Key update with SHIFTING in the primary index array --------------------
// Lower bound by binary search over the first n entries: first position whose
// key is >= key.
static int lowerBoundEntries(const vector<PrimaryEntry>& v, int n, const string& key) {
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (getField(v[mid].id, ID_LEN) < key) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

// Replaces oldKey by newKey in a primary index file and keeps it sorted by
// SHIFTING entries: close the hole left by the old key, open a hole at the new
// key's sorted position, drop the entry (same record offset) into it.
bool primaryIndexShiftUpdate(const string& indexFile, const string& oldKey, const string& newKey) {
    ifstream in(indexFile, ios::binary);
    if (!in) return false;
    vector<PrimaryEntry> v;
    PrimaryEntry e;
    while (in.read(e.id, ID_LEN) && in.read(reinterpret_cast<char*>(&e.offset), sizeof(int)))
        v.push_back(e);
    in.close();

    int n = static_cast<int>(v.size());
    int p = lowerBoundEntries(v, n, oldKey);                       // binary search
    if (p >= n || getField(v[p].id, ID_LEN) != oldKey) return false;

    PrimaryEntry moved = v[p];                                     // keeps its record offset
    setField(moved.id, ID_LEN, newKey);

    for (int i = p; i < n - 1; i++) v[i] = v[i + 1];               // shift LEFT: close the hole
    int m = n - 1;
    int q = lowerBoundEntries(v, m, newKey);                       // new sorted position
    for (int i = m; i > q; i--) v[i] = v[i - 1];                   // shift RIGHT: open a hole
    v[q] = moved;

    ofstream out(indexFile, ios::binary | ios::trunc);
    for (int i = 0; i < n; i++) {
        out.write(v[i].id, ID_LEN);
        out.write(reinterpret_cast<const char*>(&v[i].offset), sizeof(int));
    }
    cout << "  (primary index: entry moved from position " << p << " to position " << q << ")\n";
    return true;
}

// ============================================================================
//  4. RECORD BUILDERS + PRINTING
// ============================================================================
int insertDoctor(const string& id, const string& name, const string& spec,
                 const string& addr, bool& reused) {
    Doctor d;
    memset(&d, ' ', sizeof(d));
    setField(d.id, ID_LEN, id);
    setField(d.name, DNAME_LEN, name);
    setField(d.specialty, SPEC_LEN, spec);
    setField(d.address, ADDR_LEN, addr);
    return insertRecord(DOC_DAT, d, reused);
}

int insertAppointment(const string& id, const string& date, const string& doctorId, bool& reused) {
    Appointments a;
    memset(&a, ' ', sizeof(a));
    setField(a.ID, ID_LEN, id);
    setField(a.Date, DATE_LEN, date);
    setField(a.doctor_ID, ID_LEN, doctorId);
    return insertRecord(APP_DAT, a, reused);
}

void printDoctor(const Doctor& d) {
    cout << "  Doctor ID : " << getField(d.id, ID_LEN) << "\n"
         << "  Name      : " << getField(d.name, DNAME_LEN) << "\n"
         << "  Specialty : " << getField(d.specialty, SPEC_LEN) << "\n"
         << "  Address   : " << getField(d.address, ADDR_LEN) << "\n";
}

void printAppointment(const Appointments& a) {
    cout << "  Appointment ID : " << getField(a.ID, ID_LEN) << "\n"
         << "  Date           : " << getField(a.Date, DATE_LEN) << "\n"
         << "  Doctor ID      : " << getField(a.doctor_ID, ID_LEN) << "\n";
}

void printAvail(const string& path) {
    cout << "  (AVAIL LIST head in header is now " << readHeader(path) << ")\n";
}

// ============================================================================
//  5. MENU OPERATIONS
// ============================================================================

// ---- Add New Doctor --------------------------------------------------------
void addDoctor() {
    string id, name, spec, addr;
    if (!askField("Doctor ID", ID_LEN, id, true, true)) return;
    if (doctorExists(id)) {
        cout << "  ! Doctor '" << id << "' already exists. Record not added.\n";
        return;
    }
    if (!askField("Doctor Name", DNAME_LEN, name)) return;
    if (!askField("Doctor Specialty", SPEC_LEN, spec)) return;
    if (!askField("Address", ADDR_LEN, addr, false)) return;

    bool reused;
    int off = insertDoctor(id, name, spec, addr, reused);
    if (off < 0) { cout << "  ! Could not write to " << DOC_DAT << "\n"; return; }
    rebuildDoctorIndexes();
    cout << "  Doctor added at byte offset " << off
         << (reused ? " (reused a deleted slot from the AVAIL LIST).\n" : " (appended to the end of the file).\n");
    printAvail(DOC_DAT);
}

// ---- Add New Appointment ---------------------------------------------------
void addAppointment() {
    string id, date, docId;
    if (!askField("Appointment ID", ID_LEN, id, true, true)) return;
    if (appointmentExists(id)) {
        cout << "  ! Appointment '" << id << "' already exists. Record not added.\n";
        return;
    }
    if (!askField("Appointment Date", DATE_LEN, date)) return;
    if (!askField("Doctor ID", ID_LEN, docId, true, true)) return;
    if (!doctorExists(docId)) {
        cout << "  ! Doctor '" << docId << "' does not exist. Add the doctor first.\n";
        return;
    }

    bool reused;
    int off = insertAppointment(id, date, docId, reused);
    if (off < 0) { cout << "  ! Could not write to " << APP_DAT << "\n"; return; }
    rebuildAppointmentIndexes();
    cout << "  Appointment added at byte offset " << off
         << (reused ? " (reused a deleted slot from the AVAIL LIST).\n" : " (appended to the end of the file).\n");
    printAvail(APP_DAT);
}

// ---- Update Doctor Name (non-key field, but a SECONDARY key) ---------------
void updateDoctorName() {
    string id, newName;
    if (!askField("Doctor ID", ID_LEN, id, true, true)) return;
    int off = findDoctor(id);
    if (off < 0) { cout << "  ! Doctor '" << id << "' not found.\n"; return; }
    if (!askField("New Doctor Name", DNAME_LEN, newName)) return;

    Doctor d;
    if (!readRecordAt(DOC_DAT, off, d)) { cout << "  ! Read error.\n"; return; }
    setField(d.name, DNAME_LEN, newName);               // in place, exactly 30 bytes
    if (!writeRecordAt(DOC_DAT, off, d)) { cout << "  ! Write error.\n"; return; }
    rebuildDoctorSecondary();                            // name index (and its list) changed
    cout << "  Doctor name updated in place.\n";
}

// ---- Update Doctor ID (KEY update) -----------------------------------------
// Policy: the change is CASCADED to every appointment that references the doctor,
// so no appointment is left pointing at an ID that no longer exists.
void updateDoctorID() {
    string oldId, newId;
    if (!askField("Current Doctor ID", ID_LEN, oldId, true, true)) return;
    int off = findDoctor(oldId);
    if (off < 0) { cout << "  ! Doctor '" << oldId << "' not found.\n"; return; }
    if (!askField("New Doctor ID", ID_LEN, newId, true, true)) return;
    if (newId == oldId) { cout << "  New ID is the same as the old one. Nothing to do.\n"; return; }
    if (doctorExists(newId)) { cout << "  ! Doctor ID '" << newId << "' is already in use.\n"; return; }

    // 1. cascade to appointments (must happen BEFORE the secondary index is rebuilt)
    vector<string> apps = secondaryindex_doctorid_Appointments_search(APP_SEC_DOC, APP_SEC_DOC_LIST, oldId);
    for (const string& appId : apps) {
        int aoff = findAppointment(appId);
        Appointments a;
        if (aoff >= 0 && readRecordAt(APP_DAT, aoff, a)) {
            setField(a.doctor_ID, ID_LEN, newId);
            writeRecordAt(APP_DAT, aoff, a);
        }
    }

    // 2. change the key in the doctor record (same slot, same offset)
    Doctor d;
    if (!readRecordAt(DOC_DAT, off, d)) { cout << "  ! Read error.\n"; return; }
    setField(d.id, ID_LEN, newId);
    if (!writeRecordAt(DOC_DAT, off, d)) { cout << "  ! Write error.\n"; return; }

    // 3. primary index: SHIFT entries so it stays sorted
    if (!primaryIndexShiftUpdate(DOC_PRI, oldId, newId)) {
        cout << "  ! Primary index update failed, rebuilding it from the data file.\n";
        rebuildDoctorPrimary();
    }

    // 4. secondary indexes hold primary keys, so they must pick up the new ID
    rebuildDoctorSecondary();           // doc_sec_name + doc_sec_spec lists
    rebuildAppointmentSecondary();      // app_sec_docID list

    cout << "  Doctor ID changed from '" << oldId << "' to '" << newId << "'";
    if (!apps.empty()) cout << " (" << apps.size() << " appointment(s) updated too)";
    cout << ".\n";
}

// ---- Update Appointment Date (plain non-key field) -------------------------
void updateAppointmentDate() {
    string id, newDate;
    if (!askField("Appointment ID", ID_LEN, id, true, true)) return;
    int off = findAppointment(id);
    if (off < 0) { cout << "  ! Appointment '" << id << "' not found.\n"; return; }
    if (!askField("New Appointment Date", DATE_LEN, newDate)) return;

    Appointments a;
    if (!readRecordAt(APP_DAT, off, a)) { cout << "  ! Read error.\n"; return; }
    setField(a.Date, DATE_LEN, newDate);
    if (!writeRecordAt(APP_DAT, off, a)) { cout << "  ! Write error.\n"; return; }
    cout << "  Appointment date updated in place (no index change needed).\n";
}

// ---- Delete Appointment ----------------------------------------------------
void deleteAppointment() {
    string id;
    if (!askField("Appointment ID", ID_LEN, id, true, true)) return;
    int off = findAppointment(id);
    if (off < 0) { cout << "  WARNING: Appointment '" << id << "' does not exist. Nothing deleted.\n"; return; }
    if (!markDeleted<Appointments>(APP_DAT, off)) { cout << "  ! Could not delete.\n"; return; }
    rebuildAppointmentIndexes();
    cout << "  Appointment '" << id << "' deleted (slot at offset " << off << " added to the AVAIL LIST).\n";
    printAvail(APP_DAT);
}

// ---- Delete Doctor ---------------------------------------------------------
// Policy: if the doctor still has appointments, the user chooses between
// cancelling or deleting those appointments as well.
void deleteDoctor() {
    string id;
    if (!askField("Doctor ID", ID_LEN, id, true, true)) return;
    int off = findDoctor(id);
    if (off < 0) { cout << "  WARNING: Doctor '" << id << "' does not exist. Nothing deleted.\n"; return; }

    vector<string> apps = secondaryindex_doctorid_Appointments_search(APP_SEC_DOC, APP_SEC_DOC_LIST, id);
    if (!apps.empty()) {
        cout << "  Doctor '" << id << "' has " << apps.size() << " appointment(s).\n";
        if (!askYesNo("  Delete those appointments as well?")) {
            cout << "  Delete cancelled.\n";
            return;
        }
        for (const string& appId : apps) {
            int aoff = findAppointment(appId);
            if (aoff >= 0) markDeleted<Appointments>(APP_DAT, aoff);
        }
        rebuildAppointmentIndexes();
    }

    if (!markDeleted<Doctor>(DOC_DAT, off)) { cout << "  ! Could not delete.\n"; return; }
    rebuildDoctorIndexes();
    cout << "  Doctor '" << id << "' deleted (slot at offset " << off << " added to the AVAIL LIST).\n";
    printAvail(DOC_DAT);
}

// ---- Print Doctor / Appointment --------------------------------------------
void printDoctorInfo() {
    string id;
    if (!askField("Doctor ID", ID_LEN, id, true, true)) return;
    int off = findDoctor(id);
    Doctor d;
    if (off < 0 || !readRecordAt(DOC_DAT, off, d)) { cout << "  ! Doctor '" << id << "' not found.\n"; return; }
    printDoctor(d);
}

void printAppointmentInfo() {
    string id;
    if (!askField("Appointment ID", ID_LEN, id, true, true)) return;
    int off = findAppointment(id);
    Appointments a;
    if (off < 0 || !readRecordAt(APP_DAT, off, a)) { cout << "  ! Appointment '" << id << "' not found.\n"; return; }
    printAppointment(a);
}

// ---- Write Query -----------------------------------------------------------
// HOOK for the query engine (owner: teammate).  The menu only reads one line and
// hands it over.  The engine should parse it dynamically (any table / field /
// condition combination, not just the 4 examples) and use these lookups:
//   primary   : primary_index_doctor_search(DOC_PRI, id)        -> offset
//               primaryindex_Appointments_search(APP_PRI, id)   -> offset
//   secondary : secondaryindex_doctorname_search(DOC_SEC_NAME, DOC_SEC_NAME_LIST, name)
//               secondaryindex_doctorspecialty_search(DOC_SEC_SPEC, DOC_SEC_SPEC_LIST, spec)
//               secondaryindex_doctorid_Appointments_search(APP_SEC_DOC, APP_SEC_DOC_LIST, docId)
//               (all three return a vector<string> of PRIMARY keys; for "A and B"
//                intersect the two vectors, then fetch each record via the primary index)
//   records   : readRecordAt(DOC_DAT / APP_DAT, offset, rec)
void runQuery(const string& query) {
    cout << "  [Query engine not connected yet]  Received: " << query << "\n";
}

void writeQuery() {
    string q = readLine("Enter query: ");
    if (q.empty()) { cout << "  Empty query.\n"; return; }
    runQuery(q);
}

// ============================================================================
//  6. SAMPLE DATA  (assignment: 5 doctors + 10 appointments)
// ============================================================================
void loadSampleData() {
    struct D { const char *id, *name, *spec, *addr; };
    struct A { const char *id, *date, *doc; };
    const D docs[] = {
        {"D1", "Ali",  "Cardio", "Cairo"},
        {"D2", "Omar", "Neuro",  "Giza"},
        {"D3", "Mona", "Derma",  "Alex"},
        {"D4", "Sara", "Cardio", "Tanta"},   // same specialty as D1
        {"D5", "Ali",  "Ortho",  "Aswan"}    // same name as D1
    };
    const A apps[] = {
        {"A1",  "2026-11-15 09:00", "D1"}, {"A2",  "2026-11-15 10:30", "D2"},
        {"A3",  "2026-11-16 11:00", "D1"}, {"A4",  "2026-11-16 12:15", "D3"},
        {"A5",  "2026-11-17 09:45", "D4"}, {"A6",  "2026-11-17 14:00", "D5"},
        {"A7",  "2026-11-18 08:30", "D2"}, {"A8",  "2026-11-18 13:20", "D1"},
        {"A9",  "2026-11-19 10:00", "D3"}, {"A10", "2026-11-19 15:40", "D4"}
    };
    bool reused;
    for (const D& d : docs) insertDoctor(d.id, d.name, d.spec, d.addr, reused);
    for (const A& a : apps) insertAppointment(a.id, a.date, a.doc, reused);
    cout << "  Loaded 5 doctors and 10 appointments.\n";
}

// ============================================================================
//  7. MENU + MAIN
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
    filesystem::create_directories("data", ec);
    filesystem::create_directories("indexes", ec);

    bool newDoctors = initDataFile(DOC_DAT);
    bool newApps    = initDataFile(APP_DAT);
    if (newDoctors && newApps) {
        cout << "New data files created.\n";
        if (askYesNo("Load the sample data (5 doctors, 10 appointments)?")) loadSampleData();
    }
    rebuildAllIndexes();     // indexes are .ind files on disk; rebuilding at startup keeps them consistent

    while (!inputClosed) {
        showMenu();
        string line = readLine("Choose an option (1-11): ");
        if (inputClosed) break;

        int choice = 0;
        try { choice = stoi(line); } catch (...) { choice = 0; }

        switch (choice) {
            case 1:  addDoctor();            break;
            case 2:  addAppointment();       break;
            case 3:  updateDoctorName();     break;
            case 4:  updateDoctorID();       break;
            case 5:  updateAppointmentDate();break;
            case 6:  deleteAppointment();    break;
            case 7:  deleteDoctor();         break;
            case 8:  printDoctorInfo();      break;
            case 9:  printAppointmentInfo(); break;
            case 10: writeQuery();           break;
            case 11:
                cout << "Goodbye.\n";
                return 0;
            default:
                cout << "  Invalid option, please enter a number from 1 to 11.\n";
        }
    }
    return 0;
}
