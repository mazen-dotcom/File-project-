#include "PrimaryIndex.h"
#include <iomanip>
#include <filesystem>

// ============================================================================
// IndexEntry Implementation
// ============================================================================

IndexEntry::IndexEntry() : rrn(-1) {
    std::memset(id, ' ', sizeof(id));
}

IndexEntry::IndexEntry(const char* key, int recordRrn) : rrn(recordRrn) {
    std::memset(id, ' ', sizeof(id));
    if (key != nullptr) {
        // FIX: Bug 3 - Bound key copy loop to sizeof(id) to prevent buffer overrun on non-null-terminated raw 15-byte char fields
        size_t len = 0;
        while (len < sizeof(id) && key[len] != '\0') {
            len++;
        }
        std::memcpy(id, key, len);
    }
}

void IndexEntry::InitBuffer(FixedFieldBuffer& buffer) {
    buffer.Init(2, 19);
    buffer.AddField(15);
    buffer.AddField(sizeof(int));
}

int IndexEntry::Pack(FixedFieldBuffer& buffer) const {
    buffer.Clear();
    if (buffer.Pack(id, sizeof(id)) == -1) {
        return -1;
    }
    if (buffer.Pack(&rrn, sizeof(int)) == -1) {
        return -1;
    }
    return buffer.GetBufferSize();
}

int IndexEntry::Unpack(FixedFieldBuffer& buffer) {
    if (buffer.Unpack(id, sizeof(id)) == -1) {
        return -1;
    }
    if (buffer.Unpack(&rrn, sizeof(int)) == -1) {
        return -1;
    }
    return buffer.GetBufferSize();
}

bool IndexEntry::operator<(const IndexEntry& other) const {
    return PrimaryIndex::CompareKeys(this->id, other.id) < 0;
}

bool IndexEntry::operator==(const IndexEntry& other) const {
    return PrimaryIndex::CompareKeys(this->id, other.id) == 0;
}

// ============================================================================
// PrimaryIndex Implementation
// ============================================================================

PrimaryIndex::PrimaryIndex() : indexFileName("") {
}

PrimaryIndex::PrimaryIndex(const std::string& fileName) : indexFileName(fileName) {
}

PrimaryIndex::~PrimaryIndex() {
    Clear();
}

std::string PrimaryIndex::TrimKey(const char* rawId, size_t maxLen) {
    if (!rawId) {
        return "";
    }
    size_t len = 0;
    while (len < maxLen && rawId[len] != '\0') {
        len++;
    }
    while (len > 0 && (rawId[len - 1] == ' ' || rawId[len - 1] == '\0')) {
        len--;
    }
    return std::string(rawId, len);
}

int PrimaryIndex::CompareKeys(const char* id1, const char* id2) {
    std::string s1 = TrimKey(id1, 15);
    std::string s2 = TrimKey(id2, 15);
    if (s1 < s2) return -1;
    if (s1 > s2) return 1;
    return 0;
}

int PrimaryIndex::FindIndex(const char* id) const {
    if (!id || entries.empty()) {
        return -1;
    }

    int low = 0;
    int high = static_cast<int>(entries.size()) - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = CompareKeys(entries[mid].id, id);

        if (cmp == 0) {
            return mid;
        } else if (cmp < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return -1;
}

int PrimaryIndex::FindIndex(const std::string& id) const {
    return FindIndex(id.c_str());
}

int PrimaryIndex::Search(const char* id) const {
    int idx = FindIndex(id);
    if (idx != -1) {
        return entries[idx].rrn;
    }
    return -1;
}

int PrimaryIndex::Search(const std::string& id) const {
    return Search(id.c_str());
}

bool PrimaryIndex::Insert(const char* id, int rrn) {
    if (!id || rrn < 0) {
        return false;
    }

    // FIX: Bug 4 - Reject empty keys, keys starting with '*', and keys longer than 15 characters
    if (id[0] == '\0' || id[0] == '*') {
        return false;
    }

    std::string trimmed = TrimKey(id, 15);
    if (trimmed.empty() || trimmed[0] == '*') {
        return false;
    }

    size_t keyLen = 0;
    while (keyLen <= 15 && id[keyLen] != '\0') {
        keyLen++;
    }
    if (keyLen > 15) {
        return false;
    }

    // FIX: Bug 6 - Replace emplace_back + std::sort with binary search for insertion position followed by shifting (O(N))
    int low = 0;
    int high = static_cast<int>(entries.size()) - 1;
    int insertPos = static_cast<int>(entries.size());

    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = CompareKeys(entries[mid].id, id);
        if (cmp == 0) {
            return false; // Unique primary key constraint
        } else if (cmp > 0) {
            insertPos = mid;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    entries.emplace_back();
    for (int i = static_cast<int>(entries.size()) - 1; i > insertPos; --i) {
        entries[i] = entries[i - 1];
    }
    entries[insertPos] = IndexEntry(id, rrn);

    return true;
}

bool PrimaryIndex::Insert(const std::string& id, int rrn) {
    // FIX: Bug 4 - Explicitly validate std::string key before delegating
    if (id.empty() || id.length() > 15 || id[0] == '*') {
        return false;
    }
    return Insert(id.c_str(), rrn);
}

bool PrimaryIndex::Insert(const IndexEntry& entry) {
    // FIX: Bug 4 - Explicitly validate entry fields before delegating
    if (entry.id[0] == '\0' || entry.id[0] == '*' || entry.rrn < 0) {
        return false;
    }
    return Insert(entry.id, entry.rrn);
}

// FIX: Bug 2 - Implement UpdateKey with shifting to update primary keys while maintaining sorted order
bool PrimaryIndex::UpdateKey(const std::string& oldId, const std::string& newId,
                             int* fromPos, int* toPos) {
    if (oldId.empty() || newId.empty() || newId.length() > 15 || newId[0] == '*') {
        return false;
    }

    std::string trimmedNew = TrimKey(newId.c_str(), 15);
    if (trimmedNew.empty() || trimmedNew[0] == '*') {
        return false;
    }

    // Binary search oldId (return false if missing)
    int oldPos = FindIndex(oldId);
    if (oldPos == -1) {
        return false;
    }

    // Return false if newId already exists
    if (FindIndex(newId) != -1) {
        return false;
    }

    // Keep the entry's offset
    int savedRrn = entries[oldPos].rrn;
    if (fromPos != nullptr) {
        *fromPos = oldPos;
    }

    // SHIFT entries left to close the old slot (explicit loop, not erase)
    for (size_t i = oldPos; i + 1 < entries.size(); ++i) {
        entries[i] = entries[i + 1];
    }
    entries.pop_back();

    // Binary-search the new sorted position
    int low = 0;
    int high = static_cast<int>(entries.size()) - 1;
    int newPos = static_cast<int>(entries.size());

    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = CompareKeys(entries[mid].id, newId.c_str());
        if (cmp > 0) {
            newPos = mid;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    // SHIFT entries right to open that slot (explicit loop, not insert)
    entries.emplace_back();
    for (int i = static_cast<int>(entries.size()) - 1; i > newPos; --i) {
        entries[i] = entries[i - 1];
    }

    // Place the entry and report toPos
    entries[newPos] = IndexEntry(newId.c_str(), savedRrn);
    if (toPos != nullptr) {
        *toPos = newPos;
    }

    return true;
}

bool PrimaryIndex::Delete(const char* id) {
    if (!id || entries.empty()) {
        return false;
    }

    int idx = FindIndex(id);
    if (idx == -1) {
        return false;
    }

    entries.erase(entries.begin() + idx);
    return true;
}

bool PrimaryIndex::Delete(const std::string& id) {
    return Delete(id.c_str());
}

bool PrimaryIndex::SaveToDisk(const std::string& fileName) {
    std::string targetFile = fileName.empty() ? indexFileName : fileName;
    if (targetFile.empty()) {
        return false;
    }

    // Ensure parent directory exists if targetFile includes a folder path (e.g., "indexes/")
    if (auto p = std::filesystem::path(targetFile).parent_path(); !p.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(p, ec);
    }

    // FIX: Bug 1 - Truncate target file before RecordFile::Open so trailing ghost entries from earlier states are eliminated
    {
        std::ofstream truncStream(targetFile.c_str(), std::ios::binary | std::ios::trunc);
        if (!truncStream.is_open()) {
            return false;
        }
    }

    // Configure FixedFieldBuffer for the index entry size (15 ID + 4 RRN = 19 bytes)
    FixedFieldBuffer buffer(2, 19);
    IndexEntry::InitBuffer(buffer);

    // Open dedicated index file via RecordFile
    RecordFile file(buffer);
    if (!file.Open(targetFile)) {
        return false;
    }

    // Calculate total bytes and flush entire vector in a single block operation
    int totalBytes = static_cast<int>(entries.size() * sizeof(IndexEntry));
    if (totalBytes > 0) {
        int bytesWritten = file.WriteIndex(reinterpret_cast<const char*>(entries.data()), totalBytes);
        if (bytesWritten != totalBytes) {
            file.Close();
            return false;
        }
    } else {
        file.WriteIndex(nullptr, 0);
    }

    file.Close();
    return true;
}

bool PrimaryIndex::LoadFromDisk(const std::string& fileName) {
    std::string targetFile = fileName.empty() ? indexFileName : fileName;
    if (targetFile.empty()) {
        return false;
    }

    // Gracefully initialize empty index if file does not exist yet
    std::ifstream checkStream(targetFile.c_str(), std::ios::binary | std::ios::ate);
    if (!checkStream.is_open()) {
        entries.clear();
        return true;
    }

    std::streamoff fileSize = checkStream.tellg();
    checkStream.close();

    FixedFieldBuffer buffer(2, 19);
    IndexEntry::InitBuffer(buffer);

    RecordFile file(buffer);
    if (!file.Open(targetFile)) {
        entries.clear();
        return false;
    }

    int headerSize = file.GetHeaderSize();
    // FIX: Bug 5 - Treat a 0-byte file (before Open) or header-only file (fileSize == headerSize) as a valid empty index
    if (fileSize == 0 || fileSize == headerSize) {
        file.Close();
        entries.clear();
        return true;
    }

    if (fileSize < headerSize) {
        file.Close();
        entries.clear();
        return false;
    }

    // Payload size = fileSize - HeaderSize
    int payloadSize = static_cast<int>(fileSize - headerSize);
    int numEntries = payloadSize / sizeof(IndexEntry);

    entries.clear();
    if (numEntries > 0) {
        entries.resize(numEntries);
        int bytesRead = file.ReadIndex(reinterpret_cast<char*>(entries.data()), payloadSize);
        if (bytesRead != payloadSize) {
            file.Close();
            entries.clear();
            return false;
        }

        // Maintain sorted invariant for binary search
        std::sort(entries.begin(), entries.end(), [](const IndexEntry& a, const IndexEntry& b) {
            return PrimaryIndex::CompareKeys(a.id, b.id) < 0;
        });
    }

    file.Close();
    return true;
}

void PrimaryIndex::Clear() {
    entries.clear();
}

int PrimaryIndex::GetSize() const {
    return static_cast<int>(entries.size());
}

bool PrimaryIndex::IsEmpty() const {
    return entries.empty();
}

const std::vector<IndexEntry>& PrimaryIndex::GetEntries() const {
    return entries;
}

void PrimaryIndex::Print() const {
    std::cout << "--- Primary Index Dump (" << entries.size() << " entries) ---" << std::endl;
    for (size_t i = 0; i < entries.size(); ++i) {
        std::cout << "[" << std::setw(3) << i << "] ID: '" 
                  << TrimKey(entries[i].id, 15) << "', RRN: " 
                  << entries[i].rrn << std::endl;
    }
    std::cout << "-----------------------------------------------" << std::endl;
}

void PrimaryIndex::SetFileName(const std::string& fileName) {
    indexFileName = fileName;
}

std::string PrimaryIndex::GetFileName() const {
    return indexFileName;
}
