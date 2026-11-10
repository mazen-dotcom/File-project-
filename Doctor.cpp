#include "Doctor.h"
#include <cstring>
using namespace std; 
Doctor::Doctor() {
    memset(id, ' ', sizeof(id));
    memset(name, ' ', sizeof(name));
    memset(specialty, ' ', sizeof(specialty));
    memset(address, ' ', sizeof(address));
}
void Doctor::InitBuffer(FixedFieldBuffer& buffer) {
    buffer.Init(4, 95);
    buffer.AddField(15);
    buffer.AddField(30);
    buffer.AddField(20);
    buffer.AddField(30);
}
int Doctor::Pack(FixedFieldBuffer& buffer) const {
    buffer.Clear();
    if (buffer.Pack(id) == -1){
        return -1;
    }    
    if (buffer.Pack(name) == -1) {
        return -1;
    }
    if (buffer.Pack(specialty) == -1) {
        return -1;
    }
    if (buffer.Pack(address) == -1){
        return -1;
    }

    return buffer.GetBufferSize();
}
int Doctor::Unpack(FixedFieldBuffer& buffer) {
    if (buffer.Unpack(id, sizeof(id)) == -1){
        return -1;
    }
    if (buffer.Unpack(name, sizeof(name)) == -1) {
        return -1;
    }
    if (buffer.Unpack(specialty, sizeof(specialty)) == -1){
        return -1;
    }
    if (buffer.Unpack(address, sizeof(address)) == -1){
        return -1;
    }

    return buffer.GetBufferSize();
}
