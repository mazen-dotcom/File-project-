#ifndef BUFFER_FILE_H
#define BUFFER_FILE_H
#include "FixedFieldBuffer.h"
#include <fstream>
#include <string>
using namespace std;
class BufferFile {
private:
    fstream File;
    FixedFieldBuffer& Buffer;
    int HeaderSize;
    int availHead;
    string FileName;
    bool IsOpen() const;
    bool IsValidRecordAddress(int recAddr);

public:
    explicit BufferFile(FixedFieldBuffer& buffer);
    ~BufferFile();
    bool Open(const string& fileName);
    void Close();
    int ReadHeader();
    bool WriteHeader();
    int Read(int recAddr = -1);
    int Write(int recAddr = -1);
    int Append();
    bool Delete(int recAddr);
    void Rewind();
    int GetAvailHead() const;
    int GetHeaderSize() const;
};

#endif
