#include "SecondaryIndex.h"
#include <iostream>
#include <limits>

using namespace std;

SecondaryIndex::SecondaryIndex(int keySize,const string& indexFile,const string& linkFile){

    keyLength = (keySize >= 1 && keySize <= 30)? keySize:30;

    indexFileName = indexFile;
    linkFileName = linkFile;
}

string SecondaryIndex::Normalize(const char* key) const {

    if (key == nullptr)
        return "";

    int len = 0;

    while (len < keyLength && key[len] != '\0')
        len++;

    string result(key, len);

    size_t start = result.find_first_not_of(' ');

    if (start == string::npos)
        return "";

    size_t end = result.find_last_not_of(' ');

    return result.substr(start, end - start + 1);
}


string SecondaryIndex::NormalizePrimaryID(const char* id) const {

    if (id == nullptr)
        return "";

    int len = 0;

    while (len < 15 && id[len] != '\0')
        len++;

    string result(id, len);

    size_t start = result.find_first_not_of(' ');

    if (start == string::npos)
        return "";

    size_t end = result.find_last_not_of(' ');

    return result.substr(start, end - start + 1);
}


int SecondaryIndex::FindIndex(const char* key) const {

    string target = Normalize(key);

    int left = 0;
    int right = (int)entries.size() - 1;

    while (left <= right){
        int mid = left + (right - left) / 2;
        string currentKey = Normalize(entries[mid].key);

        if (currentKey == target)
            return mid;

        else if (currentKey < target)
            left = mid + 1;

        else
            right = mid - 1;
    }

    return -1;
}



bool SecondaryIndex::Insert(const char* secondaryKey,const char* primaryID) {

    string key = Normalize(secondaryKey);

    string id = NormalizePrimaryID(primaryID);

    if (key.empty() || id.empty())
        return false;

    if (key.size() > (size_t)keyLength || key.size() > sizeof(SecondaryEntry::key))
        return false;

    if (id.size() > 15)
        return false;

    int position = FindIndex(secondaryKey);

    if (position != -1) {
        int current = entries[position].head;

        int steps = 0;

        while (current != -1) {

            if (current < 0 || current >= (int)nodes.size())
                return false;

            if (steps++ >= (int)nodes.size())
                return false;

            if (NormalizePrimaryID(nodes[current].primaryID) == id)
                return false;

            current = nodes[current].next;
        }


        SecondaryNode newNode;

        memcpy(newNode.primaryID,id.data(), id.size());

        newNode.next = entries[position].head;

        nodes.push_back(newNode);

        entries[position].head = (int)nodes.size() - 1;

    }


    else {

        SecondaryEntry newEntry;

        memcpy(newEntry.key, key.data(), key.size());

        SecondaryNode newNode;

        memcpy( newNode.primaryID,id.data(),id.size());

        newNode.next = -1;

        nodes.push_back(newNode);

        newEntry.head = (int)nodes.size()-1;

        auto it = lower_bound(
            entries.begin(),
            entries.end(),
            key,
            [this](const SecondaryEntry& entry, const string& val) {

                return Normalize(entry.key) < val;

            }
        );

        entries.insert(it,newEntry);
    }

    return true;
}



bool SecondaryIndex::Remove(const char* secondaryKey,const char* primaryID){

    string id = NormalizePrimaryID(primaryID);

    if (id.empty())
        return false;

    int position = FindIndex(secondaryKey);

    if (position == -1)
        return false;
        
    
    int current = entries[position].head;

    int previous = -1;

    int steps = 0;

    while (current != -1) {

        if (current < 0 || current >= (int)nodes.size())
            return false;

        if (steps++ >= (int)nodes.size())
            return false;

        if (NormalizePrimaryID(nodes[current].primaryID) == id) {
            int nextNode = nodes[current].next;

            if (previous == -1) {

                entries[position].head = nextNode;

            }
            else {

                nodes[previous].next = nextNode;

            }

            if (entries[position].head == -1) {

                entries.erase(entries.begin() + position);

            }

            return true;
        }

        previous = current;
        current = nodes[current].next;

    }
    return false;
}


vector<int> SecondaryIndex::Search(const char* key,const PrimaryIndex& primaryIndex) const{

    vector<int> results;

    int position = FindIndex(key);

    if (position == -1)
        return results;

    int current = entries[position].head;

    int steps = 0;

    while (current != -1) {

        if (current < 0 || current >= (int)nodes.size())
            break;
        if (steps++ >= (int)nodes.size())
            break;


        string id = NormalizePrimaryID(nodes[current].primaryID);

        if (!id.empty()) {
            char idBuffer[16] = {};

            memcpy( idBuffer,id.data(),id.size() );

            int rrn = primaryIndex.Search(idBuffer);
            if (rrn != -1) {
                results.push_back(rrn);
            }
        }

        current = nodes[current].next;

    }

    return results;
}


bool SecondaryIndex::SaveToDisk() {

    ofstream indexFile(indexFileName,ios::binary | ios::trunc );

    if (!indexFile)
        return false;
    
    ofstream linkFile(linkFileName,ios::binary | ios::trunc);

    if (!linkFile)
        return false;

    if (!entries.empty()) {

        indexFile.write(
            reinterpret_cast<const char*>(entries.data()),
            static_cast<streamsize>(
                entries.size() * sizeof(SecondaryEntry)
            )
        );

    }


    if (!nodes.empty()) {

        linkFile.write(
            reinterpret_cast<const char*>(nodes.data()),
            static_cast<streamsize>(
                nodes.size() * sizeof(SecondaryNode)
            )
        );

    }

    return indexFile.good() && linkFile.good();
}


bool SecondaryIndex::LoadFromDisk() {

    ifstream indexFile(indexFileName, ios::binary);

    ifstream linkFile(linkFileName, ios::binary);

    if (!indexFile || !linkFile)
        return false;

    indexFile.seekg(0, ios::end);

    streamoff indexSize = indexFile.tellg();

    indexFile.seekg(0, ios::beg);

    linkFile.seekg(0, ios::end);

    streamoff linkSize = linkFile.tellg();

    linkFile.seekg(0, ios::beg);


    if (indexSize < 0 || linkSize < 0)
        return false;

    if (indexSize % sizeof(SecondaryEntry) != 0)
        return false;

    if (linkSize % sizeof(SecondaryNode) != 0)
        return false;

    size_t entryCount = indexSize / sizeof(SecondaryEntry);

    size_t nodeCount = linkSize / sizeof(SecondaryNode);

    if (nodeCount > (size_t)numeric_limits<int>::max())
        return false;

    vector<SecondaryEntry> loadedEntries(entryCount);

    vector<SecondaryNode> loadedNodes(nodeCount);


    if (entryCount > 0) {
        indexFile.read(
            reinterpret_cast<char*>(loadedEntries.data()),
            static_cast<streamsize>(indexSize)
        );
        if (!indexFile)
            return false;
    }

    if (nodeCount > 0) {
        linkFile.read(
            reinterpret_cast<char*>(loadedNodes.data()),
            static_cast<streamsize>(linkSize)
        );

        if (!linkFile)
            return false;
    }

    for (const auto& entry : loadedEntries) {
        if (entry.head != -1 &&
            (entry.head < 0 ||
             entry.head >= (int)nodeCount)) {

            return false;
        }

    }


    for (const auto& node : loadedNodes) {
        if (node.next != -1 &&
            (node.next < 0 ||
             node.next >= (int)nodeCount)) {

            return false;
        }

    }


    entries = loadedEntries;
    nodes = loadedNodes;



    sort(
        entries.begin(),
        entries.end(),

        [this](const SecondaryEntry& a,
               const SecondaryEntry& b) {

            return Normalize(a.key) < Normalize(b.key);

        }
    );


    return true;
}



void SecondaryIndex::Clear() {
    entries.clear();
    nodes.clear();

}


int SecondaryIndex::GetSize() const {
    return (int)entries.size();
}


void SecondaryIndex::Print() const {
    cout << "\n========== SECONDARY INDEX ==========\n";

    for (const auto& entry : entries) {

        cout << "Key: "<< Normalize(entry.key)<< endl;
        cout << "Linked List: ";

        int current = entry.head;

        int steps = 0;

        while (
            current != -1 && current >= 0 &&current < (int)nodes.size() && steps++ < (int)nodes.size()) {
            cout << NormalizePrimaryID(nodes[current].primaryID);
            current = nodes[current].next;

            if (current != -1)
                cout << " -> ";
        }
        cout << endl << endl;
    }
    cout << "=====================================\n";

}