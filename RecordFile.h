#ifndef RECORD_FILE_H
#define RECORD_FILE_H
#include "BufferFile.h"
class RecordFile : public BufferFile {
public:
    explicit RecordFile(FixedFieldBuffer& buffer) : BufferFile(buffer) {
    }
};

#endif
