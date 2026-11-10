#include "BufferFile.h"
#include <iostream>
#include <cstring>
using namespace std;
BufferFile::BufferFile(FixedFieldBuffer& buffer)
    : Buffer(buffer), HeaderSize(sizeof(int)), availHead(-1) {
}
BufferFile::~BufferFile() {
    Close();
}
bool BufferFile::IsOpen() const {
    return File.is_open();
}
bool BufferFile::Open(const string& fileName) {
    Close();
    FileName = fileName;
    File.open(FileName.c_str(), ios::in | ios::out | ios::binary);
    if (!File.is_open()) {
        ofstream createFile(FileName.c_str(), ios::out | ios::binary);
        if (!createFile)
            return false;
        createFile.close();
        File.close();
        File.clear();
        File.open(FileName.c_str(), ios::in | ios::out | ios::binary);
    }
    if (!File.is_open())
        return false;
    File.seekg(0, ios::end);
    streamoff fileSize = File.tellg();
    if (fileSize == 0) {
        availHead = -1;
        File.seekp(0 , ios::beg);
        if (!WriteHeader()) {
            Close();
            return false;
        }
    } else {
        if (fileSize < HeaderSize) {
            Close();
            return false;
        }
        File.seekg(0,ios::beg);
        if(ReadHeader() == -2){
            Close();
            return false;
        }
    }

    return true;
}
void BufferFile::Close() {
    if (File.is_open())
        File.close();
    File.clear();
}
int BufferFile::ReadHeader() {
    if (!IsOpen())
        return -2;
    File.clear();
    File.seekg(0, ios::beg);
    File.read(reinterpret_cast<char*>(&availHead), sizeof(int));
    if (!File) {
        File.clear();
        return -2;
    }
    return availHead;
}
bool BufferFile::WriteHeader() {
    if (!IsOpen())
        return false;

    File.clear();
    File.seekp(0, ios::beg);
    File.write(reinterpret_cast<const char*>(&availHead), sizeof(int));
    File.flush();

    return static_cast<bool>(File);
}
bool BufferFile::IsValidRecordAddress(int recAddr) {
    if (recAddr < HeaderSize)
        return false;

    File.clear();
    File.seekg(0, ios::end);
    streamoff fileSize = File.tellg();
    if (fileSize < HeaderSize)
        return false;
    if ((recAddr - HeaderSize) % Buffer.GetBufferSize() != 0)
        return false;
    return recAddr + Buffer.GetBufferSize() <= fileSize;
}
int BufferFile::Read(int recAddr) {
    if (!IsOpen())
        return -1;
    if (recAddr == -1)
        recAddr = HeaderSize;
    if (!IsValidRecordAddress(recAddr))
        return -1;
    File.clear();
    File.seekg(recAddr, ios::beg);

    if (Buffer.Read(File) == -1)
        return -1;
    if (Buffer.GetBuffer()[0] == '*')
        return -1;

    return recAddr;
}
int BufferFile::Write(int recAddr) {
    if (!IsOpen())
        return -1;

    if (recAddr == -1) {
        if (availHead != -1) {
            int reusedAddress = availHead;
            File.clear();
            File.seekg(reusedAddress + 1, ios::beg);
            int nextAvailable = -1;
            File.read(reinterpret_cast<char*>(&nextAvailable), sizeof(int));
            if (!File)
                return -1;

            availHead = nextAvailable;
            if (!WriteHeader())
                return -1;

            recAddr = reusedAddress;
        } else {
            File.clear();
            File.seekp(0, ios::end);
            recAddr = static_cast<int>(File.tellp());
        }
    } else {
        if (!IsValidRecordAddress(recAddr))
            return -1;
    }

    File.clear();
    File.seekp(recAddr, ios::beg);
    if (Buffer.Write(File) == -1)
        return -1;

    File.flush();
    if (!File)
        return -1;

    return recAddr;
}

int BufferFile::Append() {
    return Write(-1);
}
bool BufferFile::Delete(int recAddr) {
    if (!IsOpen() || !IsValidRecordAddress(recAddr))
        return false;

    File.clear();
    File.seekg(recAddr, ios::beg);
    char firstByte;
    File.read(&firstByte, 1);
    if (!File || firstByte == '*') {
        File.clear();
        return false;
    }
    char* deletedRecord = new char[Buffer.GetBufferSize()];
    memset(deletedRecord, ' ', Buffer.GetBufferSize());
    deletedRecord[0] = '*';
    memcpy(deletedRecord + 1, &availHead, sizeof(int));
    File.clear();
    File.seekp(recAddr,ios::beg);
    File.write(deletedRecord, Buffer.GetBufferSize());
    File.flush();
    delete[] deletedRecord;

    if (!File)
        return false;

    availHead = recAddr;
    return WriteHeader();
}
void BufferFile::Rewind() {
    if (!IsOpen())
        return;

    File.clear();
    File.seekg(HeaderSize, ios::beg);
}
int BufferFile::GetAvailHead() const {
    return availHead;
}
int BufferFile::GetHeaderSize() const {
    return HeaderSize;
}
