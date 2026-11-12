# Healthcare Management System

A C++17 console application for managing doctor and appointment records in fixed-width binary files. The project demonstrates fixed-field serialization, a reusable deleted-record list, primary and secondary indexes, and a small SQL-like query interface.

## Contents

- [Features](#features)
- [Architecture](#architecture)
- [Record and index storage](#record-and-index-storage)
- [Build and run](#build-and-run)
- [Using the application](#using-the-application)
- [Query language](#query-language)
- [Tests](#tests)
- [Project file guide](#project-file-guide)

## Features

- Add, update, delete, find, and print doctors and appointments.
- Reuse deleted record slots through an availability list stored in each data file.
- Maintain sorted primary indexes and linked-list secondary indexes.
- Rebuild indexes by scanning live records at startup, including records after deleted slots.
- Search using indexes when a query can use them, with a linear-search fallback.
- Query doctors, appointments, or joined doctor/appointment rows.
- Filter results with equality, inequality, and ordering comparisons, plus `AND` and `OR`.

## Architecture

The console menu in `main.cpp` delegates data operations to one `QueryEngine`. The engine owns the fixed-field buffers, data files, and indexes. Records are packed into fixed-size byte layouts before being written; indexes map keys to byte offsets in those data files.

```text
main.cpp
   |
   v
QueryEngine
   |-- Doctor / Appointments records
   |-- RecordFile / BufferFile
   |-- FixedFieldBuffer
   |-- PrimaryIndex
   `-- SecondaryIndex
```

At startup, the menu opens the data files, rebuilds all indexes from the records, and saves the rebuilt indexes. The data files are therefore authoritative; index files are derived lookup structures.

## Record and index storage

### Fixed-width records

Each data file begins with a 4-byte integer availability-list head, followed by fixed-size records:

| Record | Fields | Slot size |
| --- | --- | ---: |
| Doctor | ID (15 bytes), name (30), specialty (20), address (30) | 95 bytes |
| Appointment | ID (15 bytes), date (30), doctor ID (15) | 60 bytes |

Fields are stored as fixed-width character data padded with spaces. Record offsets begin immediately after the 4-byte header. Deleting a record marks its first byte with `*` and links its slot into the availability list; a subsequent insert can reuse that slot instead of extending the file.

### Runtime files

The console application uses these data-file paths:

- `data/doctors.dat`
- `data/appointments.dat`

The query engine writes index files relative to the process working directory:

- `doctor_primary.pidx` and `app_primary.pidx` — sorted primary keys and record byte offsets.
- `doc_sec_name.sidx` with `doc_sec_name_list.sidx` — doctor-name index and linked-list nodes.
- `doc_sec_spec.sidx` with `doc_sec_spec_list.sidx` — specialty index and linked-list nodes.
- `app_sec_docID.sidx` with `app_sec_docID_list.sidx` — appointment index by doctor ID and linked-list nodes.

These are generated binary files. Do not edit them manually. `QueryEngine::RebuildIndexes()` recreates the in-memory indexes from live data-file slots; the menu saves the results at startup and after operations that change indexed data.

## Build and run

Use a C++17 compiler such as GCC or MinGW-w64. From the repository root, build the console application:

```sh
g++ -std=c++17 -o healthcare.exe main.cpp QueryEngine.cpp Appointment.cpp BufferFile.cpp Doctor.cpp FixedFieldBuffer.cpp PrimaryIndex.cpp SecondaryIndex.cpp
```

Run it from the repository root so its relative data and index paths resolve as expected:

```powershell
.\healthcare.exe
```

On the first run with empty data files, the application asks whether to load five sample doctors and ten sample appointments.

## Using the application

The menu provides these operations:

1. Add a doctor.
2. Add an appointment for an existing doctor.
3. Update a doctor's name.
4. Change a doctor's ID and update its appointments and secondary-index references.
5. Update an appointment's date.
6. Delete an appointment.
7. Delete a doctor, optionally deleting that doctor's appointments too.
8. Print a doctor by ID.
9. Print an appointment by ID.
10. Enter a query.
11. Save indexes and exit.

Inputs are checked by the menu for required values, fixed-field length limits, and the reserved `*` deleted-record marker in IDs. Records are saved in the data files as operations run; indexes are saved when needed and on normal exit.

## Query language

The query interface accepts a small SQL-like subset. Field and table names are case-insensitive; string comparisons use the entered values as-is.

```text
SELECT <field list> FROM <table> [WHERE <condition> [AND|OR <condition> ...]];
```

Examples:

```sql
SELECT ALL FROM Doctors WHERE Doctor ID = 'D1';

SELECT Appointment ID, Appointment Date
FROM Appointments
WHERE Doctor ID = 'D1';

SELECT Doctor ID, Doctor Name, Appointment ID, Appointment Date
FROM Doctors, Appointments
WHERE Doctors.Doctor ID = Appointments.Doctor ID
  AND Doctors.Doctor Name = 'Ali';
```

Supported tables are `Doctors`, `Appointments`, and a combined doctor/appointment query by naming both tables (or `Both`). A combined query returns matching doctor/appointment pairs joined on doctor ID and appointment doctor ID. Field names include `Doctor ID`, `Doctor Name`, `Doctor Specialty`, `Address`, `Appointment ID`, and `Appointment Date`; table-qualified field names are accepted in combined queries. `ALL` (or `*`) selects every field available for the chosen table.

The parser recognizes `=`, `==`, `!=`, `<>`, `<`, `>`, `<=`, and `>=`, and splits conditions on `AND` and `OR`. This is not a full SQL parser: it does not implement joins written with `JOIN ... ON`, parentheses/grouping, sorting, or mutations through query text. Index strategy is selected from the conditions when possible; other queries use linear scanning. Query output reports the chosen strategy and either the result count or a no-match message.

## Tests

`test_query_engine.cpp` is a standalone assertion-based test program. It checks indexed doctor and appointment searches, secondary-index searches, combined doctor conditions, the fluent `From`/`Where`/`Select` API, and direct search methods.

Build the test executable separately (do not include `main.cpp`, which defines its own `main`):

```sh
g++ -std=c++17 -o test_query_engine.exe test_query_engine.cpp QueryEngine.cpp Appointment.cpp BufferFile.cpp Doctor.cpp FixedFieldBuffer.cpp PrimaryIndex.cpp SecondaryIndex.cpp
```

**Run the test executable from a disposable working directory.** The test creates or truncates `Doctors.dat` and `Appointments.dat` and writes index files in its working directory.

## Project file guide

| File | Responsibility |
| --- | --- |
| `main.cpp` | Console menu, input validation, user-facing doctor and appointment operations, sample-data initialization, and startup index rebuild. |
| `QueryEngine.h` / `QueryEngine.cpp` | Central data-management API, CRUD operations, index coordination, query parsing, search-strategy selection, query evaluation, joins, and formatted results. |
| `QueryTypes.h` | Query enums for tables, fields, operators, logical connectors, search strategies, and field validation. |
| `Doctor.h` / `Doctor.cpp` | Doctor record structure and its fixed-width pack/unpack layout. |
| `Appointment.h` / `Appointment.cpp` | Appointment record structure and its fixed-width pack/unpack layout. |
| `FixedFieldBuffer.h` / `FixedFieldBuffer.cpp` | Fixed-size byte buffer that packs and unpacks fields and reads/writes record-sized blocks. |
| `BufferFile.h` / `BufferFile.cpp` | Binary file access, record offsets, the availability-list header, deleted-slot marking, and slot reuse. |
| `RecordFile.h` | `BufferFile` specialization with block reads and writes for index files. |
| `PrimaryIndex.h` / `PrimaryIndex.cpp` | Sorted unique primary keys, binary search, key updates, record-offset lookup, and disk persistence. |
| `SecondaryIndex.h` / `SecondaryIndex.cpp` | Sorted secondary keys mapped to primary IDs using linked-list nodes, including insert, remove, search, and persistence. |
| `test_query_engine.cpp` | Standalone assertion-based checks for core query and search behavior. |
| `README.md` | Project overview, storage model, build instructions, query syntax, and source-file guide. |
