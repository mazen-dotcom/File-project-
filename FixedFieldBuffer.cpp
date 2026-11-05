#include "FixedFieldBuffer.h"
#include <cstring>
using namespace std;
FixedFieldBuffer::FixedFieldBuffer(int maxFields, int recordSize) {
    Buffer = 0;
    FieldSizes = 0;
    BufferSize = 0;
    MaxBytes = 0;
    NextByte = 0;
    NumFields = 0;
    MaxFields = 0;
    CurrentField = 0;
    Init(maxFields, recordSize);
}
FixedFieldBuffer::~FixedFieldBuffer() {
    delete[] Buffer;
    delete[] FieldSizes;
}
void FixedFieldBuffer::Init(int maxFields, int recordSize) {
    delete[] Buffer;
    delete[] FieldSizes;
    MaxFields = maxFields;
    BufferSize = recordSize;
    MaxBytes = recordSize;
    Buffer = new char[BufferSize];
    FieldSizes = new int[MaxFields];
    NumFields = 0;
    NextByte = 0;
    CurrentField = 0;
    Clear();
}
bool FixedFieldBuffer::AddField(int fieldSize) {
    if (NumFields >= MaxFields || fieldSize <= 0)
        return false;
    int total = fieldSize;
    for (int i = 0; i < NumFields; i++)
        total += FieldSizes[i];

    if (total > BufferSize)
        return false;
    FieldSizes[NumFields] = fieldSize;
    NumFields++;
    return true;
}

int FixedFieldBuffer::Pack(const void* field, int size) {
    if (CurrentField >= NumFields || field == 0)
        return -1;

    int fieldSize = FieldSizes[CurrentField];
    int bytesToCopy = fieldSize;

    if (size >= 0 && size < bytesToCopy){
        bytesToCopy = size;
    }
    memset(Buffer + NextByte, ' ', fieldSize);
    memcpy(Buffer + NextByte, field, bytesToCopy);

    NextByte += fieldSize;
    CurrentField++;
    return fieldSize;
}

int FixedFieldBuffer::Unpack(void* field, int maxBytes) {
    if (CurrentField >= NumFields || field == 0)
        return -1;

    int fieldSize = FieldSizes[CurrentField];
    int bytesToCopy = fieldSize;

    if (maxBytes >= 0 && maxBytes < bytesToCopy)
    {
        bytesToCopy = maxBytes;
    }

    memcpy(field, Buffer + NextByte, bytesToCopy);

    NextByte += fieldSize;
    CurrentField++;
    return fieldSize;
}

int FixedFieldBuffer::Read(istream& stream) {
    Clear();
    stream.read(Buffer, BufferSize);
    if (!stream)
        return -1;

    return BufferSize;
}

int FixedFieldBuffer::Write(std::ostream& stream) {
    stream.write(Buffer, BufferSize);
    if (!stream)
        return -1;

    return BufferSize;
}
void FixedFieldBuffer::Clear() {
    if (Buffer != 0)
        memset(Buffer, ' ', BufferSize);

    NextByte = 0;
    CurrentField = 0;
}
char* FixedFieldBuffer::GetBuffer() {
    return Buffer;
}

const char* FixedFieldBuffer::GetBuffer() const {
    return Buffer;
}

int FixedFieldBuffer::GetBufferSize() const {
    return BufferSize;
}
