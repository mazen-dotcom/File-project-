#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <fstream>
#include "QueryEngine.h"
#include "Doctor.h"
#include "Appointment.h"

using namespace std;
using namespace Query::Tokens;

void CreateSampleData()
{
    // Write sample Doctors.dat
    {
        ofstream f("Doctors.dat", ios::binary | ios::trunc);
        int head = -1;
        f.write((char*)&head, sizeof(int));

        FixedFieldBuffer buf(4, 95);
        Doctor::InitBuffer(buf);

        struct DocData { const char* id; const char* name; const char* spec; const char* addr; };
        DocData docs[] = {
            {"D1", "Ali",  "Cardio", "Cairo"},
            {"D2", "Omar", "Neuro",  "Giza"},
            {"D3", "Mona", "Derma",  "Alex"},
            {"D4", "Sara", "Cardio", "Tanta"},
            {"D5", "Ali",  "Ortho",  "Aswan"}
        };

        for (const auto& d : docs)
        {
            Doctor doc;
            memset(doc.id, ' ', sizeof(doc.id));
            memset(doc.name, ' ', sizeof(doc.name));
            memset(doc.specialty, ' ', sizeof(doc.specialty));
            memset(doc.address, ' ', sizeof(doc.address));

            memcpy(doc.id, d.id, strlen(d.id));
            memcpy(doc.name, d.name, strlen(d.name));
            memcpy(doc.specialty, d.spec, strlen(d.spec));
            memcpy(doc.address, d.addr, strlen(d.addr));

            doc.Pack(buf);
            buf.Write(f);
        }
    }

    // Write sample Appointments.dat
    {
        ofstream f("Appointments.dat", ios::binary | ios::trunc);
        int head = -1;
        f.write((char*)&head, sizeof(int));

        FixedFieldBuffer buf(3, 60);
        Appointments::InitBuffer(buf);

        struct AppData { const char* id; const char* date; const char* docID; };
        AppData apps[] = {
            {"A1",  "2026-11-15 09:00", "D1"},
            {"A2",  "2026-11-15 10:30", "D2"},
            {"A3",  "2026-11-16 11:00", "D1"},
            {"A4",  "2026-11-16 12:15", "D3"},
            {"A5",  "2026-11-17 09:45", "D4"},
            {"A6",  "2026-11-17 14:00", "D5"},
            {"A7",  "2026-11-18 08:30", "D2"},
            {"A8",  "2026-11-18 13:20", "D1"},
            {"A9",  "2026-11-19 10:00", "D3"},
            {"A10", "2026-11-19 15:40", "D4"}
        };

        for (const auto& a : apps)
        {
            Appointments app;
            memset(app.ID, ' ', sizeof(app.ID));
            memset(app.Date, ' ', sizeof(app.Date));
            memset(app.doctor_ID, ' ', sizeof(app.doctor_ID));

            memcpy(app.ID, a.id, strlen(a.id));
            memcpy(app.Date, a.date, strlen(a.date));
            memcpy(app.doctor_ID, a.docID, strlen(a.docID));

            app.Pack(buf);
            buf.Write(f);
        }
    }
}

int main()
{
    cout << "Setting up sample data files...\n";
    CreateSampleData();

    QueryEngine engine;
    cout << "Rebuilding indexes from data files...\n";
    engine.RebuildIndexes();
    engine.SaveIndexes();

    cout << "\n======================================================\n";
    cout << "TEST 1: Select all from Doctors where Doctor ID='D1'; (primary index)\n";
    cout << "======================================================\n";
    {
        string q = "Select all from Doctors where Doctor ID='D1';";
        QueryResult res = engine.Execute(q);
        res.Print();
        assert(res.strategy == SearchStrategy::PrimaryIndex);
        assert(res.doctors.size() == 1);
        assert(QueryEngine::TrimField(res.doctors[0].id, 15) == "D1");
        assert(QueryEngine::TrimField(res.doctors[0].name, 30) == "Ali");
        cout << ">> TEST 1 PASSED!\n";
    }

    cout << "\n======================================================\n";
    cout << "TEST 2: Select all from Appointments where Doctor ID='D1'; (secondary index)\n";
    cout << "======================================================\n";
    {
        string q = "Select all from Appointments where Doctor ID='D1';";
        QueryResult res = engine.Execute(q);
        res.Print();
        assert(res.strategy == SearchStrategy::SecondaryIndex);
        assert(res.appointments.size() == 3); // A1, A3, A8
        cout << ">> TEST 2 PASSED!\n";
    }

    cout << "\n======================================================\n";
    cout << "TEST 3: Select Doctor Name from Doctors where Doctor Name='Ali'; (secondary index)\n";
    cout << "======================================================\n";
    {
        string q = "Select Doctor Name from Doctors where Doctor Name='Ali';";
        QueryResult res = engine.Execute(q);
        res.Print();
        assert(res.strategy == SearchStrategy::SecondaryIndex);
        assert(res.doctors.size() == 2); // D1 (Ali Cardio) and D5 (Ali Ortho)
        cout << ">> TEST 3 PASSED!\n";
    }

    cout << "\n======================================================\n";
    cout << "TEST 4: Select all from Doctors where Doctor Name='Ali' and Doctor Specialty='Cardio'; (multiple secondary indices)\n";
    cout << "======================================================\n";
    {
        string q = "Select all from Doctors where Doctor Name='Ali' and Doctor Specialty='Cardio';";
        QueryResult res = engine.Execute(q);
        res.Print();
        assert(res.strategy == SearchStrategy::MultipleSecondaryIndices);
        assert(res.doctors.size() == 1);
        assert(QueryEngine::TrimField(res.doctors[0].id, 15) == "D1");
        cout << ">> TEST 4 PASSED!\n";
    }

    cout << "\n======================================================\n";
    cout << "TEST 5: Method Chaining from->where->select with Instructions struct\n";
    cout << "======================================================\n";
    {
        QueryResult res = engine.From(Table::Doctors)
                                .Where(Field::DoctorName, Op::Equals, string("Ali"), LogicOp::And)
                                .Where(Field::DoctorSpecialty, Op::Equals, string("Cardio"))
                                .Select({Field::DoctorName, Field::DoctorSpecialty});
        res.Print();
        assert(res.strategy == SearchStrategy::MultipleSecondaryIndices);
        assert(res.doctors.size() == 1);
        assert(engine.GetInstructions().from_statment == Table::Doctors);
        assert(engine.GetInstructions().where_statment.size() == 2);
        cout << ">> TEST 5 PASSED!\n";
    }

    cout << "\n======================================================\n";
    cout << "TEST 6: Direct Search Methods on QueryEngine\n";
    cout << "======================================================\n";
    {
        Doctor doc;
        bool found = engine.SearchDoctorByID("D2", doc);
        assert(found);
        assert(QueryEngine::TrimField(doc.name, 30) == "Omar");

        Appointments app;
        found = engine.SearchAppointmentByID("A4", app);
        assert(found);
        assert(QueryEngine::TrimField(app.doctor_ID, 15) == "D3");

        auto cardioDocs = engine.SearchDoctorsBySpecialty("Cardio");
        assert(cardioDocs.size() == 2); // D1 and D4

        auto aliCardio = engine.SearchDoctorsByNameAndSpecialty("Ali", "Cardio");
        assert(aliCardio.size() == 1);
        assert(QueryEngine::TrimField(aliCardio[0].id, 15) == "D1");

        auto d1Apps = engine.SearchAppointmentsByDoctorID("D1");
        assert(d1Apps.size() == 3);

        cout << ">> TEST 6 PASSED!\n";
    }

    cout << "\n======================================================\n";
    cout << "ALL TESTS PASSED SUCCESSFULLY!\n";
    cout << "======================================================\n";
    return 0;
}

