#ifndef PRIMARY_INDEX_H
#define PRIMARY_INDEX_H

#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <iostream>

#include "RecordFile.h"
#include "FixedFieldBuffer.h"

#pragma pack(push, 1)
/**
 * @brief Represents a single key-to-RRN entry in the primary index.
 * Packed to exactly 19 bytes (15 ID + 4 RRN) to match FixedFieldBuffer and enable batch block I/O.
 */
struct IndexEntry {
    char id[15];  ///< Fixed-length primary key identifier (15 bytes)
    int rrn;      ///< Relative Record Number in data file (4 bytes)

    IndexEntry();
    IndexEntry(const char* key, int recordRrn);

    static void InitBuffer(FixedFieldBuffer& buffer);
    int Pack(FixedFieldBuffer& buffer) const;
    int Unpack(FixedFieldBuffer& buffer);

    bool operator<(const IndexEntry& other) const;
    bool operator==(const IndexEntry& other) const;
};
#pragma pack(pop)

static_assert(sizeof(IndexEntry) == 19, "IndexEntry must be packed to exactly 19 bytes.");

/**
 * @brief Manages an in-memory Primary Index using std::vector<IndexEntry>,
 *        supporting O(log N) binary search and seamless disk persistence via RecordFile.
 */
class PrimaryIndex {
private:
    std::vector<IndexEntry> entries;  ///< RAM-resident sorted index entries
    std::string indexFileName;        ///< Associated index file name (e.g., Doctors.pidx)

public:
    PrimaryIndex();
    explicit PrimaryIndex(const std::string& fileName);
    ~PrimaryIndex();

    // Core Operations
    int Search(const char* id) const;
    int Search(const std::string& id) const;

    int FindIndex(const char* id) const;
    int FindIndex(const std::string& id) const;

    bool Insert(const char* id, int rrn);
    bool Insert(const std::string& id, int rrn);
    bool Insert(const IndexEntry& entry);

    // FIX: Bug 2 - Add UpdateKey with shifting to update primary keys while maintaining sorted order
    bool UpdateKey(const std::string& oldId, const std::string& newId, int* fromPos = nullptr, int* toPos = nullptr);

    bool Delete(const char* id);
    bool Delete(const std::string& id);

    // Disk Persistence using RecordFile & FixedFieldBuffer directly
    bool SaveToDisk(const std::string& fileName = "");
    bool LoadFromDisk(const std::string& fileName = "");

    // Utility & Inspection Operations
    void Clear();
    int GetSize() const;
    bool IsEmpty() const;
    const std::vector<IndexEntry>& GetEntries() const;
    void Print() const;

    void SetFileName(const std::string& fileName);
    std::string GetFileName() const;

    // Helpers
    static int CompareKeys(const char* id1, const char* id2);
    static std::string TrimKey(const char* rawId, size_t maxLen = 15);
};

#endif // PRIMARY_INDEX_H
