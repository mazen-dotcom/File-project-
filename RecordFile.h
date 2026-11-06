#ifndef RECORD_FILE_H
#define RECORD_FILE_H

#include "BufferFile.h"

class RecordFile : public BufferFile {
public:
    explicit RecordFile(FixedFieldBuffer& buffer) : BufferFile(buffer) {
    }

    /**
     * @brief Writes raw index bytes in a single block operation right after the header.
     * @param data Pointer to the buffer containing index entries.
     * @param bytes Number of bytes to write.
     * @return Number of bytes written, or -1 on error.
     */
    int WriteIndex(const char* data, int bytes) {
        if (!IsOpen()) {
            return -1;
        }
        File.clear();
        File.seekp(HeaderSize, ios::beg);
        if (bytes > 0 && data != nullptr) {
            File.write(data, bytes);
        }
        File.flush();
        if (!File) {
            return -1;
        }
        return bytes;
    }

    /**
     * @brief Reads raw index bytes in a single block operation right after the header.
     * @param data Pointer to the memory buffer where index entries will be loaded.
     * @param bytes Total number of bytes to read.
     * @return Number of bytes successfully read, or -1 on error.
     */
    int ReadIndex(char* data, int bytes) {
        if (!IsOpen()) {
            return -1;
        }
        File.clear();
        File.seekg(HeaderSize, ios::beg);
        if (bytes > 0 && data != nullptr) {
            File.read(data, bytes);
            return static_cast<int>(File.gcount());
        }
        return 0;
    }
};

#endif

