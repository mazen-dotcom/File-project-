#include "QueryEngine.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

using namespace std;
using namespace Query::Tokens;

// ============================================================================
// Helper String Functions
// ============================================================================
namespace
{
    string ToLower(const string &str)
    {
        string lower = str;
        transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
            return tolower(c);
        });
        return lower;
    }

    string Trim(const string &str)
    {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    string StripQuotes(const string &str)
    {
        string s = Trim(str);
        if (s.size() >= 2 && ((s.front() == '\'' && s.back() == '\'') || (s.front() == '"' && s.back() == '"')))
        {
            return s.substr(1, s.size() - 2);
        }
        return s;
    }

    Field ParseFieldName(const string &rawField)
    {
        string s = ToLower(Trim(rawField));
        if (s == "all" || s == "*")
            return Field::All;
        if (s == "doctor id" || s == "doctor_id" || s == "doctorid" || s == "doc id" || s == "doc_id")
            return Field::DoctorID;
        if (s == "doctor name" || s == "doctor_name" || s == "doctorname" || s == "doc name" || s == "name")
            return Field::DoctorName;
        if (s == "doctor specialty" || s == "doctor_specialty" || s == "doctorspecialty" || s == "specialty" || s == "spec")
            return Field::DoctorSpecialty;
        if (s == "address" || s == "doctor address")
            return Field::Address;
        if (s == "appointment id" || s == "appointment_id" || s == "appointmentid" || s == "app id" || s == "app_id")
            return Field::AppointmentID;
        if (s == "appointment date" || s == "appointment_date" || s == "appointmentdate" || s == "date")
            return Field::AppointmentDate;
        return Field::Invalid;
    }

    string FieldToStringName(Field field)
    {
        switch (field)
        {
            case Field::All:             return "All";
            case Field::DoctorID:        return "Doctor ID";
            case Field::DoctorName:      return "Doctor Name";
            case Field::DoctorSpecialty: return "Doctor Specialty";
            case Field::Address:         return "Address";
            case Field::AppointmentID:   return "Appointment ID";
            case Field::AppointmentDate: return "Appointment Date";
            default:                     return "Invalid";
        }
    }

    Table ParseTableName(const string &rawTable)
    {
        string s = ToLower(Trim(rawTable));
        if (s == "doctors" || s == "doctor")
            return Table::Doctors;
        if (s == "appointments" || s == "appointment")
            return Table::Appointments;
        if (s == "both")
            return Table::Both;
        return Table::Doctors;
    }

    Op ParseOp(const string &rawOp)
    {
        string s = Trim(rawOp);
        if (s == "=" || s == "==")
            return Op::Equals;
        if (s == "!=" || s == "<>")
            return Op::NotEquals;
        if (s == ">")
            return Op::GreaterThan;
        if (s == "<")
            return Op::LessThan;
        if (s == ">=")
            return Op::GreaterOrEqual;
        if (s == "<=")
            return Op::LessOrEqual;
        return Op::Equals;
    }
}

// ============================================================================
// QueryResult Implementation
// ============================================================================
string QueryResult::GetStrategyString() const
{
    switch (strategy)
    {
        case SearchStrategy::PrimaryIndex:
            return "(uses primary index)";
        case SearchStrategy::SecondaryIndex:
            return "(uses secondary index)";
        case SearchStrategy::MultipleSecondaryIndices:
            return "(uses multiple secondary indices)";
        case SearchStrategy::LinearSearch:
            return "(uses linear search)";
        default:
            return "(uses unknown strategy)";
    }
}

void QueryResult::Print(ostream &os) const
{
    os << "Execution Strategy: " << GetStrategyString() << "\n";
    if (!success)
    {
        os << "Error: " << errorMessage << "\n";
        return;
    }

    if (table == Table::Doctors)
    {
        if (doctors.empty())
        {
            os << "No doctor records found matching the query.\n";
            return;
        }

        bool showAll = (selectedFields.find(Field::All) != selectedFields.end());
        bool showID = showAll || (selectedFields.find(Field::DoctorID) != selectedFields.end());
        bool showName = showAll || (selectedFields.find(Field::DoctorName) != selectedFields.end());
        bool showSpec = showAll || (selectedFields.find(Field::DoctorSpecialty) != selectedFields.end());
        bool showAddr = showAll || (selectedFields.find(Field::Address) != selectedFields.end());

        // Header
        string separator = "+";
        if (showID) separator += string(17, '-') + "+";
        if (showName) separator += string(32, '-') + "+";
        if (showSpec) separator += string(22, '-') + "+";
        if (showAddr) separator += string(32, '-') + "+";

        os << separator << "\n|";
        if (showID) os << " " << left << setw(15) << "Doctor ID" << " |";
        if (showName) os << " " << left << setw(30) << "Doctor Name" << " |";
        if (showSpec) os << " " << left << setw(20) << "Doctor Specialty" << " |";
        if (showAddr) os << " " << left << setw(30) << "Address" << " |";
        os << "\n" << separator << "\n";

        for (const auto &doc : doctors)
        {
            os << "|";
            if (showID) os << " " << left << setw(15) << QueryEngine::TrimField(doc.id, sizeof(doc.id)) << " |";
            if (showName) os << " " << left << setw(30) << QueryEngine::TrimField(doc.name, sizeof(doc.name)) << " |";
            if (showSpec) os << " " << left << setw(20) << QueryEngine::TrimField(doc.specialty, sizeof(doc.specialty)) << " |";
            if (showAddr) os << " " << left << setw(30) << QueryEngine::TrimField(doc.address, sizeof(doc.address)) << " |";
            os << "\n";
        }
        os << separator << "\n";
        os << "Total: " << doctors.size() << " record(s) found.\n";
    }
    else if (table == Table::Appointments)
    {
        if (appointments.empty())
        {
            os << "No appointment records found matching the query.\n";
            return;
        }

        bool showAll = (selectedFields.find(Field::All) != selectedFields.end());
        bool showAppID = showAll || (selectedFields.find(Field::AppointmentID) != selectedFields.end());
        bool showDate = showAll || (selectedFields.find(Field::AppointmentDate) != selectedFields.end());
        bool showDocID = showAll || (selectedFields.find(Field::DoctorID) != selectedFields.end());

        string separator = "+";
        if (showAppID) separator += string(17, '-') + "+";
        if (showDate) separator += string(32, '-') + "+";
        if (showDocID) separator += string(17, '-') + "+";

        os << separator << "\n|";
        if (showAppID) os << " " << left << setw(15) << "Appointment ID" << " |";
        if (showDate) os << " " << left << setw(30) << "Appointment Date" << " |";
        if (showDocID) os << " " << left << setw(15) << "Doctor ID" << " |";
        os << "\n" << separator << "\n";

        for (const auto &app : appointments)
        {
            os << "|";
            if (showAppID) os << " " << left << setw(15) << QueryEngine::TrimField(app.ID, sizeof(app.ID)) << " |";
            if (showDate) os << " " << left << setw(30) << QueryEngine::TrimField(app.Date, sizeof(app.Date)) << " |";
            if (showDocID) os << " " << left << setw(15) << QueryEngine::TrimField(app.doctor_ID, sizeof(app.doctor_ID)) << " |";
            os << "\n";
        }
        os << separator << "\n";
        os << "Total: " << appointments.size() << " record(s) found.\n";
    }
}

// ============================================================================
// QueryEngine Helpers
// ============================================================================
void QueryEngine::InitializeBuffers()
{
    Doctor::InitBuffer(docBuffer);
    Appointments::InitBuffer(appBuffer);
}

string QueryEngine::ExtractStringFromAny(const std::any &val)
{
    if (!val.has_value())
        return "";
    if (val.type() == typeid(string))
        return std::any_cast<string>(val);
    if (val.type() == typeid(const char*))
        return string(std::any_cast<const char*>(val));
    if (val.type() == typeid(char*))
        return string(std::any_cast<char*>(val));
    return "";
}

string QueryEngine::TrimField(const char *data, size_t maxLen)
{
    if (!data) return "";
    size_t len = 0;
    while (len < maxLen && data[len] != '\0')
        len++;
    while (len > 0 && (data[len - 1] == ' ' || data[len - 1] == '\0'))
        len--;
    return string(data, len);
}

// ============================================================================
// QueryEngine Constructors & Destructor
// ============================================================================
QueryEngine::QueryEngine()
    : doctorfile(docBuffer),
      appointmentFile(appBuffer),
      docPrimaryIndex("doctor_primary.pidx"),
      appPrimaryIndex("app_primary.pidx"),
      appSecDocIDIndex(15, "app_sec_docID.sidx", "app_sec_docID_list.sidx"),
      docSecNameIndex(30, "doc_sec_name.sidx", "doc_sec_name_list.sidx"),
      docSecSpecIndex(20, "doc_sec_spec.sidx", "doc_sec_spec_list.sidx")
{
    InitializeBuffers();
    ResetInstructions();
    OpenDataFiles(doctorDataFileName, appointmentDataFileName);
    LoadIndexes();
}

QueryEngine::QueryEngine(const string &docDataFile, const string &appDataFile)
    : doctorfile(docBuffer),
      appointmentFile(appBuffer),
      docPrimaryIndex("doctor_primary.pidx"),
      appPrimaryIndex("app_primary.pidx"),
      appSecDocIDIndex(15, "app_sec_docID.sidx", "app_sec_docID_list.sidx"),
      docSecNameIndex(30, "doc_sec_name.sidx", "doc_sec_name_list.sidx"),
      docSecSpecIndex(20, "doc_sec_spec.sidx", "doc_sec_spec_list.sidx"),
      doctorDataFileName(docDataFile),
      appointmentDataFileName(appDataFile)
{
    InitializeBuffers();
    ResetInstructions();
    OpenDataFiles(doctorDataFileName, appointmentDataFileName);
    LoadIndexes();
}

QueryEngine::~QueryEngine()
{
    CloseDataFiles();
}

void QueryEngine::ResetInstructions()
{
    currentInstructions.from_statment = Table::Doctors;
    currentInstructions.where_statment.clear();
    currentInstructions.select_statment.clear();
}

// ============================================================================
// Insert Operations
// ============================================================================

bool QueryEngine::AddDoctor(const string &id,
                             const string &name,
                             const string &specialty,
                             const string &address)
{
    // 1. Check that the doctor isn't already there
    if (docPrimaryIndex.Search(id) != -1) {
        cout << "Error: Doctor with ID '" << id << "' already exists!" << endl;
        return false;
    }
    
    // 2. Make sure the ID isn't empty
    if (id.empty() || id.length() > 15) {
        cout << "Error: Invalid Doctor ID!" << endl;
        return false;
    }
    
    // 3. Prepare a Doctor object
    Doctor doc;
    
    // Copy ID
    memset(doc.id, ' ', sizeof(doc.id));
    memcpy(doc.id, id.c_str(), min(id.length(), sizeof(doc.id)));
    
    // Copy Name
    memset(doc.name, ' ', sizeof(doc.name));
    memcpy(doc.name, name.c_str(), min(name.length(), sizeof(doc.name)));
    
    // Copy Specialty
    memset(doc.specialty, ' ', sizeof(doc.specialty));
    memcpy(doc.specialty, specialty.c_str(), min(specialty.length(), sizeof(doc.specialty)));
    
    // Copy Address
    memset(doc.address, ' ', sizeof(doc.address));
    memcpy(doc.address, address.c_str(), min(address.length(), sizeof(doc.address)));
    
    // 4. Fill the buffer
    doc.Pack(docBuffer);
    
    // 5. Write to the file (it will use the deleted space if available, or it will add at the end)
    int offset = doctorfile.Write(-1);  // -1 means: use availHead or add at the end
    if (offset == -1) {
        cout << "Error: Failed to write to data file!" << endl;
        return false;
    }
    
    // 6. Add to the Primary Index
    if (!docPrimaryIndex.Insert(id, offset)) {
        cout << "Error: Failed to insert into primary index!" << endl;
        return false;
    }
    
    // 7. Add to the Secondary Indices
    docSecNameIndex.Insert(name.c_str(), id.c_str());
    docSecSpecIndex.Insert(specialty.c_str(), id.c_str());
    
    return true;
}

bool QueryEngine::AddAppointment(const string &id,
                                  const string &date,
                                  const string &doctorID)
{
    // 1. Make sure the appointment doesn't already exist
    if (appPrimaryIndex.Search(id) != -1) {
        cout << "Error: Appointment with ID '" << id << "' already exists!" << endl;
        return false;
    }
    
    // 2. Check if the doctor is around
    if (docPrimaryIndex.Search(doctorID) == -1) {
        cout << "Error: Doctor with ID '" << doctorID << "' does not exist!" << endl;
        return false;
    }
    
    // 3. Make sure the ID isn't empty
    if (id.empty() || id.length() > 15) {
        cout << "Error: Invalid Appointment ID!" << endl;
        return false;
    }
    
    // 4. Prepare a Appointments object
    Appointments app;
    
    // Copy ID
    memset(app.ID, ' ', sizeof(app.ID));
    memcpy(app.ID, id.c_str(), min(id.length(), sizeof(app.ID)));
    
    // Copy Date
    memset(app.Date, ' ', sizeof(app.Date));
    memcpy(app.Date, date.c_str(), min(date.length(), sizeof(app.Date)));
    
    // Copy Doctor ID
    memset(app.doctor_ID, ' ', sizeof(app.doctor_ID));
    memcpy(app.doctor_ID, doctorID.c_str(), min(doctorID.length(), sizeof(app.doctor_ID)));
    
    // 5. Fill the buffer
    app.Pack(appBuffer);
    
    // 6. Write to the file (it will use the deleted space if available, or it will add at the end)
    int offset = appointmentFile.Write(-1); // -1 means: use availHead or add at the end
    if (offset == -1) {
        cout << "Error: Failed to write to data file!" << endl;
        return false;
    }
    
    // 7. أضف إلى الـ Primary Index
    if (!appPrimaryIndex.Insert(id, offset)) {
        cout << "Error: Failed to insert into primary index!" << endl;
        return false;
    }
    
    // 8. أضف إلى الـ Secondary Index
    appSecDocIDIndex.Insert(doctorID.c_str(), id.c_str());
    
    return true;
}

// ============================================================================
// Update & Delete Operations
// ============================================================================

bool QueryEngine::UpdateDoctor(const string &doctorID,
                                const string &newName,
                                const string &newSpecialty,
                                const string &newAddress)
{
    // 1. Look for the offset from the Primary Index
    int offset = docPrimaryIndex.Search(doctorID);
    if (offset == -1)
        return false;
    
    // 2. Read the current record
    Doctor doc;
    if (!SearchDoctorByID(doctorID, doc))
        return false;
    
    // 3. Keep the old values (for Secondary Indices)
    string oldName = TrimField(doc.name, sizeof(doc.name));
    string oldSpec = TrimField(doc.specialty, sizeof(doc.specialty));
    
    // 4. Update only the required fields (if they're not empty)
    if (!newName.empty()) {
        memset(doc.name, ' ', sizeof(doc.name));
        memcpy(doc.name, newName.c_str(), min(newName.length(), sizeof(doc.name)));
    }
    if (!newSpecialty.empty()) {
        memset(doc.specialty, ' ', sizeof(doc.specialty));
        memcpy(doc.specialty, newSpecialty.c_str(), min(newSpecialty.length(), sizeof(doc.specialty)));
    }
    if (!newAddress.empty()) {
        memset(doc.address, ' ', sizeof(doc.address));
        memcpy(doc.address, newAddress.c_str(), min(newAddress.length(), sizeof(doc.address)));
    }
    
    // 5. Fill the buffer with the new data
    doc.Pack(docBuffer);
    
    // 6. Write at the same offset (Update in the same place)
    if (doctorfile.Write(offset) == -1)
        return false;
    
    // 7. Update the secondary indices if the name or major changes
    if (!newName.empty() && oldName != newName) {
        docSecNameIndex.Remove(oldName.c_str(), doctorID.c_str());
        docSecNameIndex.Insert(newName.c_str(), doctorID.c_str());
    }
    if (!newSpecialty.empty() && oldSpec != newSpecialty) {
        docSecSpecIndex.Remove(oldSpec.c_str(), doctorID.c_str());
        docSecSpecIndex.Insert(newSpecialty.c_str(), doctorID.c_str());
    }
    
    return true;
}

bool QueryEngine::DeleteDoctor(const string &doctorID)
{
    // 1. Look for the offset from the Primary Index
    int offset = docPrimaryIndex.Search(doctorID);
    if (offset == -1)
        return false;
    
    // 2. Read the record to get the data (to delete it from Secondary Indices)
    Doctor doc;
    if (!SearchDoctorByID(doctorID, doc))
        return false;
    
    string name = TrimField(doc.name, sizeof(doc.name));
    string spec = TrimField(doc.specialty, sizeof(doc.specialty));
    
    // 3. Delete from the data file (put '*' in the first byte)
    if (!doctorfile.Delete(offset))
        return false;
    
    // 4. Delete from the Primary Index
    docPrimaryIndex.Delete(doctorID);
    
    // 5. Delete from the Secondary Indices
    docSecNameIndex.Remove(name.c_str(), doctorID.c_str());
    docSecSpecIndex.Remove(spec.c_str(), doctorID.c_str());
    
    return true;
}

bool QueryEngine::UpdateAppointment(const string &appID,
                                     const string &newDate,
                                     const string &newDoctorID)
{
    // 1. Look for the offset
    int offset = appPrimaryIndex.Search(appID);
    if (offset == -1)
        return false;
    
    // 2. Read the current record
    Appointments app;
    if (!SearchAppointmentByID(appID, app))
        return false;
    
    // 3. Save the old values
    string oldDocID = TrimField(app.doctor_ID, sizeof(app.doctor_ID));
    
    // 4. Update the required fields
    if (!newDate.empty()) {
        memset(app.Date, ' ', sizeof(app.Date));
        memcpy(app.Date, newDate.c_str(), min(newDate.length(), sizeof(app.Date)));
    }
    if (!newDoctorID.empty()) {
        memset(app.doctor_ID, ' ', sizeof(app.doctor_ID));
        memcpy(app.doctor_ID, newDoctorID.c_str(), min(newDoctorID.length(), sizeof(app.doctor_ID)));
    }
    
    // 5. Pack and Write
    app.Pack(appBuffer);
    if (appointmentFile.Write(offset) == -1)
        return false;
    
    // 6. Update the secondary index if DoctorID changes
    if (!newDoctorID.empty() && oldDocID != newDoctorID) {
        appSecDocIDIndex.Remove(oldDocID.c_str(), appID.c_str());
        appSecDocIDIndex.Insert(newDoctorID.c_str(), appID.c_str());
    }
    
    return true;
}

bool QueryEngine::DeleteAppointment(const string &appID)
{
    // 1. Look for the offset
    int offset = appPrimaryIndex.Search(appID);
    if (offset == -1)
        return false;
    
    // 2. Read the current record
    Appointments app;
    if (!SearchAppointmentByID(appID, app))
        return false;
    
    string docID = TrimField(app.doctor_ID, sizeof(app.doctor_ID));
    
    // 3. Delete from the data file (put '*' in the first byte)
    if (!appointmentFile.Delete(offset))
        return false;
    
    // 4. Delete from the Indices
    appPrimaryIndex.Delete(appID);
    appSecDocIDIndex.Remove(docID.c_str(), appID.c_str());
    
    return true;
}

bool QueryEngine::OpenDataFiles(const string &docDataFile, const string &appDataFile)
{
    if (!docDataFile.empty()) doctorDataFileName = docDataFile;
    if (!appDataFile.empty()) appointmentDataFileName = appDataFile;

    // Check data directory fallback
    string docPath = doctorDataFileName;
    string appPath = appointmentDataFileName;

    ifstream testDoc(docPath, ios::binary);
    if (!testDoc) {
        string fallback = "data/" + doctorDataFileName;
        ifstream testFallback(fallback, ios::binary);
        if (testFallback) docPath = fallback;
    } else {
        testDoc.close();
    }

    ifstream testApp(appPath, ios::binary);
    if (!testApp) {
        string fallback = "data/" + appointmentDataFileName;
        ifstream testFallback(fallback, ios::binary);
        if (testFallback) appPath = fallback;
    } else {
        testApp.close();
    }

    doctorfile.Open(docPath);
    appointmentFile.Open(appPath);
    return true;
}

void QueryEngine::CloseDataFiles()
{
    doctorfile.Close();
    appointmentFile.Close();
}

bool QueryEngine::LoadIndexes()
{
    docPrimaryIndex.LoadFromDisk();
    appPrimaryIndex.LoadFromDisk();
    appSecDocIDIndex.LoadFromDisk();
    docSecNameIndex.LoadFromDisk();
    docSecSpecIndex.LoadFromDisk();
    return true;
}

bool QueryEngine::SaveIndexes()
{
    docPrimaryIndex.SaveToDisk();
    appPrimaryIndex.SaveToDisk();
    appSecDocIDIndex.SaveToDisk();
    docSecNameIndex.SaveToDisk();
    docSecSpecIndex.SaveToDisk();
    return true;
}

void QueryEngine::RebuildIndexes()
{
    docPrimaryIndex.Clear();
    appPrimaryIndex.Clear();
    appSecDocIDIndex.Clear();
    docSecNameIndex.Clear();
    docSecSpecIndex.Clear();

    // Rebuild Doctor indexes
    if (doctorfile.Open(doctorDataFileName))
    {
        // FIX: BufferFile::Read() returns -1 for BOTH deleted records ('*' marker) AND
        // end-of-file (invalid address), so using it as the while-loop condition caused the
        // loop to stop prematurely at the first deleted record, silently skipping all records
        // after it. Fix: compute the file size independently to bound the loop, and treat
        // Read()==-1 within bounds as a deleted record to skip (not a loop terminator).
        int docFileSize = 0;
        {
            ifstream sizeCheck(doctorDataFileName, ios::binary | ios::ate);
            if (sizeCheck.is_open()) {
                docFileSize = static_cast<int>(sizeCheck.tellg());
                sizeCheck.close();
            }
        }

        doctorfile.Rewind();
        int offset = doctorfile.GetHeaderSize();
        int recordSize = docBuffer.GetBufferSize();
        Doctor doc;
        while (offset + recordSize <= docFileSize)
        {
            if (doctorfile.Read(offset) != -1)
            {
                doc.Unpack(docBuffer);
                if (doc.id[0] != '*')
                {
                    string id = TrimField(doc.id, sizeof(doc.id));
                    string name = TrimField(doc.name, sizeof(doc.name));
                    string spec = TrimField(doc.specialty, sizeof(doc.specialty));

                    docPrimaryIndex.Insert(id, offset);
                    docSecNameIndex.Insert(name.c_str(), id.c_str());
                    docSecSpecIndex.Insert(spec.c_str(), id.c_str());
                }
            }
            offset += recordSize;
        }
    }

    // Rebuild Appointment indexes
    if (appointmentFile.Open(appointmentDataFileName))
    {
        // FIX: Same deleted-record / EOF ambiguity fix as above for appointments.
        int appFileSize = 0;
        {
            ifstream sizeCheck(appointmentDataFileName, ios::binary | ios::ate);
            if (sizeCheck.is_open()) {
                appFileSize = static_cast<int>(sizeCheck.tellg());
                sizeCheck.close();
            }
        }

        appointmentFile.Rewind();
        int offset = appointmentFile.GetHeaderSize();
        int recordSize = appBuffer.GetBufferSize();
        Appointments app;
        while (offset + recordSize <= appFileSize)
        {
            if (appointmentFile.Read(offset) != -1)
            {
                app.Unpack(appBuffer);
                if (app.ID[0] != '*')
                {
                    string id = TrimField(app.ID, sizeof(app.ID));
                    string docID = TrimField(app.doctor_ID, sizeof(app.doctor_ID));

                    appPrimaryIndex.Insert(id, offset);
                    appSecDocIDIndex.Insert(docID.c_str(), id.c_str());
                }
            }
            offset += recordSize;
        }
    }
}

// ============================================================================
// Core Search Methods using Specific Indexes
// ============================================================================
bool QueryEngine::SearchDoctorByID(const string &doctorID, Doctor &resultDoc)
{
    int offset = docPrimaryIndex.Search(doctorID);
    if (offset == -1)
        return false;

    if (!doctorfile.Open(doctorDataFileName))
        return false;

    if (doctorfile.Read(offset) == -1)
        return false;

    resultDoc.Unpack(docBuffer);
    return true;
}

bool QueryEngine::SearchAppointmentByID(const string &appID, Appointments &resultApp)
{
    int offset = appPrimaryIndex.Search(appID);
    if (offset == -1)
        return false;

    if (!appointmentFile.Open(appointmentDataFileName))
        return false;

    if (appointmentFile.Read(offset) == -1)
        return false;

    resultApp.Unpack(appBuffer);
    return true;
}

vector<Doctor> QueryEngine::SearchDoctorsByName(const string &doctorName)
{
    vector<Doctor> results;
    vector<int> offsets = docSecNameIndex.Search(doctorName.c_str(), docPrimaryIndex);
    if (offsets.empty())
        return results;

    if (!doctorfile.Open(doctorDataFileName))
        return results;

    for (int offset : offsets)
    {
        if (doctorfile.Read(offset) != -1)
        {
            Doctor doc;
            doc.Unpack(docBuffer);
            results.push_back(doc);
        }
    }
    return results;
}

vector<Doctor> QueryEngine::SearchDoctorsBySpecialty(const string &specialty)
{
    vector<Doctor> results;
    vector<int> offsets = docSecSpecIndex.Search(specialty.c_str(), docPrimaryIndex);
    if (offsets.empty())
        return results;

    if (!doctorfile.Open(doctorDataFileName))
        return results;

    for (int offset : offsets)
    {
        if (doctorfile.Read(offset) != -1)
        {
            Doctor doc;
            doc.Unpack(docBuffer);
            results.push_back(doc);
        }
    }
    return results;
}

vector<Doctor> QueryEngine::SearchDoctorsByNameAndSpecialty(const string &name, const string &specialty)
{
    vector<Doctor> results;
    vector<int> offsetsName = docSecNameIndex.Search(name.c_str(), docPrimaryIndex);
    vector<int> offsetsSpec = docSecSpecIndex.Search(specialty.c_str(), docPrimaryIndex);

    sort(offsetsName.begin(), offsetsName.end());
    sort(offsetsSpec.begin(), offsetsSpec.end());

    vector<int> commonOffsets;
    set_intersection(offsetsName.begin(), offsetsName.end(),
                     offsetsSpec.begin(), offsetsSpec.end(),
                     back_inserter(commonOffsets));

    if (commonOffsets.empty())
        return results;

    if (!doctorfile.Open(doctorDataFileName))
        return results;

    for (int offset : commonOffsets)
    {
        if (doctorfile.Read(offset) != -1)
        {
            Doctor doc;
            doc.Unpack(docBuffer);
            results.push_back(doc);
        }
    }
    return results;
}

vector<Appointments> QueryEngine::SearchAppointmentsByDoctorID(const string &doctorID)
{
    vector<Appointments> results;
    vector<int> offsets = appSecDocIDIndex.Search(doctorID.c_str(), appPrimaryIndex);
    if (offsets.empty())
        return results;

    if (!appointmentFile.Open(appointmentDataFileName))
        return results;

    for (int offset : offsets)
    {
        if (appointmentFile.Read(offset) != -1)
        {
            Appointments app;
            app.Unpack(appBuffer);
            results.push_back(app);
        }
    }
    return results;
}

// ============================================================================
// Chain of Functions: From -> Where -> Select using Instructions struct
// ============================================================================
QueryEngine& QueryEngine::From(Table table)
{
    ResetInstructions();
    currentInstructions.from_statment = table;
    return *this;
}

QueryEngine& QueryEngine::Where(Field field, Op op, const std::any &val, LogicOp logicOp)
{
    currentInstructions.where_statment.insert(make_tuple(field, op, val, logicOp));
    return *this;
}

QueryEngine& QueryEngine::Where(const set<tuple<Field, Op, std::any, LogicOp>> &conditions)
{
    currentInstructions.where_statment.insert(conditions.begin(), conditions.end());
    return *this;
}

QueryResult QueryEngine::Select(const set<Field> &fields)
{
    currentInstructions.select_statment = fields;
    return Execute(currentInstructions);
}

QueryResult QueryEngine::Select(initializer_list<Field> fields)
{
    set<Field> s(fields.begin(), fields.end());
    return Select(s);
}

QueryResult QueryEngine::Select(Field field)
{
    return Select(set<Field>{field});
}

QueryResult QueryEngine::Select(const vector<Field> &fields)
{
    set<Field> s(fields.begin(), fields.end());
    return Select(s);
}

// ============================================================================
// Strategy Classifier
// ============================================================================
SearchStrategy QueryEngine::DetermineStrategy(const Instructions &instructions)
{
    const auto &wheres = instructions.where_statment;
    if (wheres.empty())
        return SearchStrategy::LinearSearch;

    if (instructions.from_statment == Table::Doctors)
    {
        // Check for multiple secondary indices (Name + Specialty)
        bool hasNameEquals = false;
        bool hasSpecEquals = false;
        bool hasIDEquals = false;

        for (const auto &cond : wheres)
        {
            Field f = get<0>(cond);
            Op op = get<1>(cond);
            if (op == Op::Equals)
            {
                if (f == Field::DoctorID) hasIDEquals = true;
                if (f == Field::DoctorName) hasNameEquals = true;
                if (f == Field::DoctorSpecialty) hasSpecEquals = true;
            }
        }

        if (hasIDEquals)
            return SearchStrategy::PrimaryIndex;
        if (hasNameEquals && hasSpecEquals)
            return SearchStrategy::MultipleSecondaryIndices;
        if (hasNameEquals || hasSpecEquals)
            return SearchStrategy::SecondaryIndex;
    }
    else if (instructions.from_statment == Table::Appointments)
    {
        bool hasAppIDEquals = false;
        bool hasDocIDEquals = false;

        for (const auto &cond : wheres)
        {
            Field f = get<0>(cond);
            Op op = get<1>(cond);
            if (op == Op::Equals)
            {
                if (f == Field::AppointmentID) hasAppIDEquals = true;
                if (f == Field::DoctorID) hasDocIDEquals = true;
            }
        }

        if (hasAppIDEquals)
            return SearchStrategy::PrimaryIndex;
        if (hasDocIDEquals)
            return SearchStrategy::SecondaryIndex;
    }

    return SearchStrategy::LinearSearch;
}

// ============================================================================
// Query Parsing
// ============================================================================
Instructions QueryEngine::Parse(const string &queryString)
{
    Instructions inst;
    inst.from_statment = Table::Doctors;

    string q = Trim(queryString);
    if (!q.empty() && q.back() == ';')
        q.pop_back();
    q = Trim(q);

    string lowerQ = ToLower(q);

    // Find SELECT
    size_t selectPos = lowerQ.find("select");
    if (selectPos == string::npos)
        return inst;

    // Find FROM
    size_t fromPos = lowerQ.find("from", selectPos + 6);
    if (fromPos == string::npos)
        return inst;

    // Find WHERE
    size_t wherePos = lowerQ.find("where", fromPos + 4);

    // 1. Parse SELECT fields
    string selectPart = Trim(q.substr(selectPos + 6, fromPos - (selectPos + 6)));
    stringstream ssSelect(selectPart);
    string token;
    while (getline(ssSelect, token, ','))
    {
        Field f = ParseFieldName(token);
        if (f != Field::Invalid)
            inst.select_statment.insert(f);
    }
    if (inst.select_statment.empty())
        inst.select_statment.insert(Field::All);

    // 2. Parse FROM table
    string fromPart;
    if (wherePos != string::npos)
        fromPart = Trim(q.substr(fromPos + 4, wherePos - (fromPos + 4)));
    else
        fromPart = Trim(q.substr(fromPos + 4));

    inst.from_statment = ParseTableName(fromPart);

    // 3. Parse WHERE conditions
    if (wherePos != string::npos)
    {
        string wherePart = Trim(q.substr(wherePos + 5));

        // Split conditions by "AND" / "OR" (case-insensitive)
        size_t current = 0;
        LogicOp prevLogic = LogicOp::None;

        while (current < wherePart.size())
        {
            string lowerRemaining = ToLower(wherePart.substr(current));
            size_t nextAnd = lowerRemaining.find(" and ");
            size_t nextOr = lowerRemaining.find(" or ");

            size_t clauseEnd = string::npos;
            LogicOp nextLogic = LogicOp::None;
            size_t skipLen = 0;

            if (nextAnd != string::npos && (nextOr == string::npos || nextAnd < nextOr))
            {
                clauseEnd = current + nextAnd;
                nextLogic = LogicOp::And;
                skipLen = 5;
            }
            else if (nextOr != string::npos)
            {
                clauseEnd = current + nextOr;
                nextLogic = LogicOp::Or;
                skipLen = 4;
            }

            string clause;
            if (clauseEnd != string::npos)
            {
                clause = Trim(wherePart.substr(current, clauseEnd - current));
                current = clauseEnd + skipLen;
            }
            else
            {
                clause = Trim(wherePart.substr(current));
                current = wherePart.size();
            }

            // Parse single condition: Field Op Value
            size_t opPos = string::npos;
            string opStr = "=";
            vector<string> ops = {"!=", "<=", ">=", "=", "<", ">"};
            for (const auto &cand : ops)
            {
                size_t p = clause.find(cand);
                if (p != string::npos)
                {
                    opPos = p;
                    opStr = cand;
                    break;
                }
            }

            if (opPos != string::npos)
            {
                string rawField = Trim(clause.substr(0, opPos));
                string rawVal = Trim(clause.substr(opPos + opStr.size()));
                rawVal = StripQuotes(rawVal);

                Field f = ParseFieldName(rawField);
                Op op = ParseOp(opStr);
                inst.where_statment.insert(make_tuple(f, op, rawVal, prevLogic));
            }

            prevLogic = nextLogic;
        }
    }

    return inst;
}

// ============================================================================
// Query Execution
// ============================================================================
QueryResult QueryEngine::Execute(const string &queryString)
{
    Instructions inst = Parse(queryString);

    // Call the chain of functions: From -> Where -> Select
    this->From(inst.from_statment);
    for (const auto &cond : inst.where_statment)
    {
        this->Where(get<0>(cond), get<1>(cond), get<2>(cond), get<3>(cond));
    }
    return this->Select(inst.select_statment);
}

QueryResult QueryEngine::Execute(const Instructions &instructions)
{
    QueryResult result;
    result.table = instructions.from_statment;
    result.selectedFields = instructions.select_statment;
    if (result.selectedFields.empty())
        result.selectedFields.insert(Field::All);

    // Validate fields against target table
    for (Field f : result.selectedFields)
    {
        if (!isValidField(result.table, f))
        {
            result.success = false;
            result.errorMessage = "Field '" + FieldToStringName(f) + "' is invalid for table.";
            return result;
        }
    }

    result.strategy = DetermineStrategy(instructions);

    if (result.table == Table::Doctors)
    {
        switch (result.strategy)
        {
            case SearchStrategy::PrimaryIndex:
            {
                string doctorID;
                for (const auto &cond : instructions.where_statment)
                {
                    if (get<0>(cond) == Field::DoctorID && get<1>(cond) == Op::Equals)
                    {
                        doctorID = ExtractStringFromAny(get<2>(cond));
                        break;
                    }
                }
                Doctor doc;
                if (SearchDoctorByID(doctorID, doc))
                {
                    result.doctors.push_back(doc);
                }
                break;
            }
            case SearchStrategy::SecondaryIndex:
            {
                string name, spec;
                for (const auto &cond : instructions.where_statment)
                {
                    if (get<0>(cond) == Field::DoctorName && get<1>(cond) == Op::Equals)
                        name = ExtractStringFromAny(get<2>(cond));
                    if (get<0>(cond) == Field::DoctorSpecialty && get<1>(cond) == Op::Equals)
                        spec = ExtractStringFromAny(get<2>(cond));
                }
                if (!name.empty())
                    result.doctors = SearchDoctorsByName(name);
                else if (!spec.empty())
                    result.doctors = SearchDoctorsBySpecialty(spec);
                break;
            }
            case SearchStrategy::MultipleSecondaryIndices:
            {
                string name, spec;
                for (const auto &cond : instructions.where_statment)
                {
                    if (get<0>(cond) == Field::DoctorName && get<1>(cond) == Op::Equals)
                        name = ExtractStringFromAny(get<2>(cond));
                    if (get<0>(cond) == Field::DoctorSpecialty && get<1>(cond) == Op::Equals)
                        spec = ExtractStringFromAny(get<2>(cond));
                }
                result.doctors = SearchDoctorsByNameAndSpecialty(name, spec);
                break;
            }
            case SearchStrategy::LinearSearch:
            default:
            {
                if (doctorfile.Open(doctorDataFileName))
                {
                    // FIX: Same deleted-record / EOF ambiguity fix as RebuildIndexes.
                    int docFileSize = 0;
                    {
                        ifstream sizeCheck(doctorDataFileName, ios::binary | ios::ate);
                        if (sizeCheck.is_open()) {
                            docFileSize = static_cast<int>(sizeCheck.tellg());
                            sizeCheck.close();
                        }
                    }

                    doctorfile.Rewind();
                    int offset = doctorfile.GetHeaderSize();
                    int recordSize = docBuffer.GetBufferSize();
                    Doctor doc;
                    while (offset + recordSize <= docFileSize)
                    {
                        if (doctorfile.Read(offset) != -1)
                        {
                            doc.Unpack(docBuffer);
                            if (doc.id[0] != '*')
                            {
                                bool match = true;
                                for (const auto &cond : instructions.where_statment)
                                {
                                    Field f = get<0>(cond);
                                    Op op = get<1>(cond);
                                    string val = ExtractStringFromAny(get<2>(cond));
                                    string actual;

                                    if (f == Field::DoctorID) actual = TrimField(doc.id, sizeof(doc.id));
                                    else if (f == Field::DoctorName) actual = TrimField(doc.name, sizeof(doc.name));
                                    else if (f == Field::DoctorSpecialty) actual = TrimField(doc.specialty, sizeof(doc.specialty));
                                    else if (f == Field::Address) actual = TrimField(doc.address, sizeof(doc.address));

                                    if (op == Op::Equals && actual != val) match = false;
                                    if (op == Op::NotEquals && actual == val) match = false;
                                }
                                if (match)
                                    result.doctors.push_back(doc);
                            }
                        }
                        offset += recordSize;
                    }
                }
                break;
            }
        }
    }
    else if (result.table == Table::Appointments)
    {
        switch (result.strategy)
        {
            case SearchStrategy::PrimaryIndex:
            {
                string appID;
                for (const auto &cond : instructions.where_statment)
                {
                    if (get<0>(cond) == Field::AppointmentID && get<1>(cond) == Op::Equals)
                    {
                        appID = ExtractStringFromAny(get<2>(cond));
                        break;
                    }
                }
                Appointments app;
                if (SearchAppointmentByID(appID, app))
                {
                    result.appointments.push_back(app);
                }
                break;
            }
            case SearchStrategy::SecondaryIndex:
            {
                string docID;
                for (const auto &cond : instructions.where_statment)
                {
                    if (get<0>(cond) == Field::DoctorID && get<1>(cond) == Op::Equals)
                    {
                        docID = ExtractStringFromAny(get<2>(cond));
                        break;
                    }
                }
                if (!docID.empty())
                    result.appointments = SearchAppointmentsByDoctorID(docID);
                break;
            }
            case SearchStrategy::LinearSearch:
            default:
            {
                if (appointmentFile.Open(appointmentDataFileName))
                {
                    // FIX: Same deleted-record / EOF ambiguity fix as RebuildIndexes.
                    int appFileSize = 0;
                    {
                        ifstream sizeCheck(appointmentDataFileName, ios::binary | ios::ate);
                        if (sizeCheck.is_open()) {
                            appFileSize = static_cast<int>(sizeCheck.tellg());
                            sizeCheck.close();
                        }
                    }

                    appointmentFile.Rewind();
                    int offset = appointmentFile.GetHeaderSize();
                    int recordSize = appBuffer.GetBufferSize();
                    Appointments app;
                    while (offset + recordSize <= appFileSize)
                    {
                        if (appointmentFile.Read(offset) != -1)
                        {
                            app.Unpack(appBuffer);
                            if (app.ID[0] != '*')
                            {
                                bool match = true;
                                for (const auto &cond : instructions.where_statment)
                                {
                                    Field f = get<0>(cond);
                                    Op op = get<1>(cond);
                                    string val = ExtractStringFromAny(get<2>(cond));
                                    string actual;

                                    if (f == Field::AppointmentID) actual = TrimField(app.ID, sizeof(app.ID));
                                    else if (f == Field::AppointmentDate) actual = TrimField(app.Date, sizeof(app.Date));
                                    else if (f == Field::DoctorID) actual = TrimField(app.doctor_ID, sizeof(app.doctor_ID));

                                    if (op == Op::Equals && actual != val) match = false;
                                    if (op == Op::NotEquals && actual == val) match = false;
                                }
                                if (match)
                                    result.appointments.push_back(app);
                            }
                        }
                        offset += recordSize;
                    }
                }
                break;
            }
        }
    }

    result.success = true;
    return result;
}