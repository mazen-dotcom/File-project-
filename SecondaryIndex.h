#ifndef SECONDARY_INDEX_H
#define SECONDARY_INDEX_H

#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <algorithm>

#include "PrimaryIndex.h"

#pragma pack(push, 1)

struct SecondaryEntry {

    char key[30];
    int head;

    SecondaryEntry() {
        memset(key, 0, sizeof(key));
        head = -1;
    }
};

struct SecondaryNode {
    char primaryID[15];
    int next;

    SecondaryNode() {
        memset(primaryID, 0, sizeof(primaryID));
        next = -1;
    }
};

#pragma pack(pop)


static_assert(sizeof(SecondaryEntry) == 34,"SecondaryEntry must be 34 bytes");

static_assert(sizeof(SecondaryNode) == 19,"SecondaryNode must be 19 bytes");


class SecondaryIndex {

private:

    //Stores distinct secondary keys.
    std::vector<SecondaryEntry> entries;

    // Stores linked list nodes.
    std::vector<SecondaryNode> nodes;

    int keyLength;

    std::string indexFileName;
    std::string linkFileName;

    std::string Normalize(const char* key) const;

    std::string NormalizePrimaryID(const char* id) const;

    int FindIndex(const char* key) const;


public:

    SecondaryIndex(int keySize = 30, const std::string& indexFile = "", const std::string& linkFile = "");

    // Insert a secondary key and its Primary ID.
    bool Insert( const char* secondaryKey,const char* primaryID);
    bool Insert(const std::string& secondaryKey, const std::string& primaryID);

    // Remove a Primary ID from a secondary key.
    bool Remove(const char* secondaryKey,const char* primaryID);
    bool Remove(const std::string& secondaryKey, const std::string& primaryID);

    // Search using Secondary Index
    // and return RRNs using Primary Index.
    std::vector<int> Search( const char* key,const PrimaryIndex& primaryIndex) const;
    std::vector<int> Search(const std::string& key, const PrimaryIndex& primaryIndex) const;

    int FindIndex(const std::string& key) const;

    bool SaveToDisk();

    bool LoadFromDisk();

    void Clear();

    int GetSize() const;

    void Print() const;

    const std::string& GetIndexFileName() const { return indexFileName; }
    const std::string& GetLinkFileName() const { return linkFileName; }
    void SetFileNames(const std::string& indexFile, const std::string& linkFile) {
        indexFileName = indexFile;
        linkFileName = linkFile;
    }
};

#endif