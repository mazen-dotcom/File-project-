#ifndef DOCTOR_H
#define DOCTOR_H
#include "FixedFieldBuffer.h"
class Doctor {
public:
    char id[15];
    char name[30];
    char specialty[20];
    char address[30];
    Doctor();
    static void InitBuffer(FixedFieldBuffer& buffer);
    int Pack(FixedFieldBuffer& buffer) const;
    int Unpack(FixedFieldBuffer& buffer);
};

#endif
