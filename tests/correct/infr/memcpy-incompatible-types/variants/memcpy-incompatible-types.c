#include <stdint.h>
#include <string.h>
#include <assert.h>

// Simulating and empty struct
struct EmptyStruct {
};

// struct { ptr, {} }
struct PtrComponents_u64 {
    void* ptr;
    struct EmptyStruct empty;
};

// struct { [1 x i64] }
struct PtrRepr_u64 {
    int64_t value[1];
};

int main() {
    struct PtrComponents_u64 _5;
    struct PtrRepr_u64 _4;

    //Init individually => Avoid gcc generating additional memcpy's when using constant structs
    _5.ptr = NULL;
    _4.value[0] = 3;

    memcpy(&_4, &_5, 8); // copying 8 bytes

    assert(_4.value[0] == 0);
    return 0;
}
