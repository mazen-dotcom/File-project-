#ifndef APPOINTMENT_H
#define APPOINTMENT_H
#include "FixedFieldBuffer.h"
class Appointments {
public:
    char ID[15];
    char Date[30];
    char doctor_ID[15];
    Appointments();
    static void InitBuffer(FixedFieldBuffer& buffer);
    int Pack(FixedFieldBuffer& buffer) const;
    int Unpack(FixedFieldBuffer& buffer);
};

#endif
