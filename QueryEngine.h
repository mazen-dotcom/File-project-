#ifndef QUERY_ENGINE_H
#define QUERY_ENGINE_H

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <tuple>
#include <any>
#include <typeinfo>
#include <algorithm>

#include "QueryTypes.h"
#include "Doctor.h"
#include "Appointment.h"
#include "RecordFile.h"
#include "PrimaryIndex.h"
#include "SecondaryIndex.h"
#include "FixedFieldBuffer.h"

// Comparator for std::any inside namespace std to allow std::tuple containing
// std::any to be used seamlessly in std::set without modifying the standard interface.
namespace std
{
    inline bool operator<(const any &lhs, const any &rhs)
    {
        if (!lhs.has_value() && !rhs.has_value())
            return false;
        if (!lhs.has_value())
            return true;
        if (!rhs.has_value())
            return false;
        if (lhs.type() == typeid(string) && rhs.type() == typeid(string))
        {
            return any_cast<string>(lhs) < any_cast<string>(rhs);
        }
        if (lhs.type() == typeid(const char *) && rhs.type() == typeid(const char *))
        {
            return string(any_cast<const char *>(lhs)) < string(any_cast<const char *>(rhs));
        }
        return lhs.type().before(rhs.type());
    }
}

struct Instructions
{
    Query::Tokens::Table from_statment;
    std::set<std::tuple<Query::Tokens::Field, Query::Tokens::Op, std::any, Query::Tokens::LogicOp>> where_statment;
    std::set<Query::Tokens::Field> select_statment;
};

struct QueryResult
{
    bool success = true;
    std::string errorMessage;
    Query::Tokens::SearchStrategy strategy = Query::Tokens::SearchStrategy::LinearSearch;
    Query::Tokens::Table table = Query::Tokens::Table::Doctors;
    std::set<Query::Tokens::Field> selectedFields;

    std::vector<Doctor> doctors;
    std::vector<Appointments> appointments;
    std::vector<std::string> formattedOutput;

    std::string GetStrategyString() const;
    void Print(std::ostream &os = std::cout) const;
};

class QueryEngine
{
private:
    FixedFieldBuffer docBuffer{4, 95};
    FixedFieldBuffer appBuffer{3, 60};

    RecordFile doctorfile;
    RecordFile appointmentFile;

    PrimaryIndex docPrimaryIndex{"doctor_primary.pidx"};
    PrimaryIndex appPrimaryIndex{"app_primary.pidx"};

    SecondaryIndex appSecDocIDIndex{15, "app_sec_docID.sidx", "app_sec_docID_list.sidx"};
    SecondaryIndex docSecNameIndex{30, "doc_sec_name.sidx", "doc_sec_name_list.sidx"};
    SecondaryIndex docSecSpecIndex{20, "doc_sec_spec.sidx", "doc_sec_spec_list.sidx"};

    Instructions currentInstructions;
    std::string doctorDataFileName = "Doctors.dat";
    std::string appointmentDataFileName = "Appointments.dat";

    void InitializeBuffers();
    static std::string ExtractStringFromAny(const std::any &val);

public:
    static std::string TrimField(const char *data, size_t maxLen);
    QueryEngine();
    explicit QueryEngine(const std::string &docDataFile, const std::string &appDataFile);
    ~QueryEngine();

    // Data File & Index Operations
    bool OpenDataFiles(const std::string &docDataFile = "", const std::string &appDataFile = "");
    void CloseDataFiles();
    bool LoadIndexes();
    bool SaveIndexes();
    void RebuildIndexes();

    // Specific Index Search Methods
    bool SearchDoctorByID(const std::string &doctorID, Doctor &resultDoc);
    bool SearchAppointmentByID(const std::string &appID, Appointments &resultApp);
    std::vector<Doctor> SearchDoctorsByName(const std::string &doctorName);
    std::vector<Doctor> SearchDoctorsBySpecialty(const std::string &specialty);
    std::vector<Doctor> SearchDoctorsByNameAndSpecialty(const std::string &name, const std::string &specialty);
    std::vector<Doctor> SearchDoctorsByNameOrSpecialty(const std::string &name, const std::string &specialty);
    std::vector<Appointments> SearchAppointmentsByDoctorID(const std::string &doctorID);

    // Chain of Functions: From -> Where -> Select using Instructions struct
    QueryEngine &From(Query::Tokens::Table table);
    QueryEngine &Where(Query::Tokens::Field field, Query::Tokens::Op op, const std::any &val, Query::Tokens::LogicOp logicOp = Query::Tokens::LogicOp::None);
    QueryEngine &Where(const std::set<std::tuple<Query::Tokens::Field, Query::Tokens::Op, std::any, Query::Tokens::LogicOp>> &conditions);
    QueryResult Select(const std::set<Query::Tokens::Field> &fields = {Query::Tokens::Field::All});
    QueryResult Select(std::initializer_list<Query::Tokens::Field> fields);
    QueryResult Select(Query::Tokens::Field field);
    QueryResult Select(const std::vector<Query::Tokens::Field> &fields);

    // Query Execution & Parsing
    QueryResult Execute(const std::string &queryString);
    QueryResult Execute(const Instructions &instructions);
    static Instructions Parse(const std::string &queryString);
    static Query::Tokens::SearchStrategy DetermineStrategy(const Instructions &instructions);

    // Accessors
    const Instructions &GetInstructions() const { return currentInstructions; }
    void ResetInstructions();

    // Insert Operations
    bool AddDoctor(const std::string &id,
                   const std::string &name,
                   const std::string &specialty,
                   const std::string &address);

    bool AddAppointment(const std::string &id,
                        const std::string &date,
                        const std::string &doctorID);

    // Update & Delete Operations
    bool UpdateDoctor(const std::string &doctorID,
                      const std::string &newName = "",
                      const std::string &newSpecialty = "",
                      const std::string &newAddress = "");

    bool DeleteDoctor(const std::string &doctorID);

    bool UpdateAppointment(const std::string &appID,
                           const std::string &newDate = "",
                           const std::string &newDoctorID = "");

    bool DeleteAppointment(const std::string &appID);

    PrimaryIndex &GetDocPrimaryIndex() { return docPrimaryIndex; }
    PrimaryIndex &GetAppPrimaryIndex() { return appPrimaryIndex; }
    SecondaryIndex &GetAppSecDocIDIndex() { return appSecDocIDIndex; }
    SecondaryIndex &GetDocSecNameIndex() { return docSecNameIndex; }
    SecondaryIndex &GetDocSecSpecIndex() { return docSecSpecIndex; }
    RecordFile &GetDoctorFile() { return doctorfile; }
    RecordFile &GetAppointmentFile() { return appointmentFile; }
    FixedFieldBuffer &GetDocBuffer() { return docBuffer; }
    FixedFieldBuffer &GetAppBuffer() { return appBuffer; }
};

#endif // QUERY_ENGINE_H
