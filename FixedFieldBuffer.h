#ifndef FIXED_FIELD_BUFFER_H
#define FIXED_FIELD_BUFFER_H
#include <iostream>
using namespace std;
class FixedFieldBuffer {
private:
    char* Buffer;
    int BufferSize;
    int MaxBytes;
    int NextByte;
    int* FieldSizes;
    int NumFields;
    int MaxFields;
    int CurrentField;

public:
    FixedFieldBuffer(int maxFields, int recordSize);
    ~FixedFieldBuffer();

    FixedFieldBuffer(const FixedFieldBuffer&) = delete;
    FixedFieldBuffer& operator=(const FixedFieldBuffer&) = delete;

    void Init(int maxFields, int recordSize);
    bool AddField(int fieldSize);
    int Pack(const void* field, int size = -1);
    int Unpack(void* field, int maxBytes = -1);
    int Read(istream& stream);
    int Write(ostream& stream);
    void Clear();
    char* GetBuffer();
    const char* GetBuffer() const;
    int GetBufferSize() const;
};

#endif
