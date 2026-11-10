#include "PrimaryIndex.h"
#include <iomanip>

// ============================================================================
// IndexEntry Implementation
// ============================================================================

IndexEntry::IndexEntry() : rrn(-1) {
    std::memset(id, ' ', sizeof(id));
}

IndexEntry::IndexEntry(const char* key, int recordRrn) : rrn(recordRrn) {
    std::memset(id, ' ', sizeof(id));
    if (key != nullptr) {
        size_t len = std::strlen(key);
        if (len > sizeof(id)) {
            len = sizeof(id);
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

    if (Search(id) != -1) {
        return false; // Unique primary key constraint
    }

    entries.emplace_back(id, rrn);

    std::sort(entries.begin(), entries.end(), [](const IndexEntry& a, const IndexEntry& b) {
        return PrimaryIndex::CompareKeys(a.id, b.id) < 0;
    });

    return true;
}

bool PrimaryIndex::Insert(const std::string& id, int rrn) {
    return Insert(id.c_str(), rrn);
}

bool PrimaryIndex::Insert(const IndexEntry& entry) {
    return Insert(entry.id, entry.rrn);
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
