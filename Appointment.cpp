#include "Appointment.h"
#include <cstring>
using namespace std;
Appointments::Appointments() {
    memset(ID, ' ', sizeof(ID));
    memset(Date, ' ', sizeof(Date));
    memset(doctor_ID, ' ', sizeof(doctor_ID));
}
void Appointments::InitBuffer(FixedFieldBuffer& buffer) {
    buffer.Init(3, 60);
    buffer.AddField(15);
    buffer.AddField(30);
    buffer.AddField(15);
}
int Appointments::Pack(FixedFieldBuffer& buffer) const {
    buffer.Clear();

    if (buffer.Pack(ID) == -1){
        return -1;
    }
    if (buffer.Pack(Date) == -1){
        return -1;
    }
    if (buffer.Pack(doctor_ID) == -1){
        return -1;
    }

    return buffer.GetBufferSize();
}

int Appointments::Unpack(FixedFieldBuffer& buffer) {
    if (buffer.Unpack(ID, sizeof(ID)) == -1){
        return -1;
    }
    if (buffer.Unpack(Date, sizeof(Date)) == -1){
        return -1;
    }
    if (buffer.Unpack(doctor_ID, sizeof(doctor_ID)) == -1){
        return -1;
    }

    return buffer.GetBufferSize();
}
