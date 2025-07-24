// Note that i1 in LLVM is still a full byte in memory
#include <stdint.h>
#include <string.h>
#include <assert.h>

// Struct: { ptr, { i32 }, i32, i1, i1 }
struct PtrComponents_u64 {
    void* ptr;
    struct { int32_t inner; } outer;
    int32_t third;
    uint8_t fourth;
    uint8_t fifth;
};

// Struct: { [1 x i64], [2 x i32], i1, i1 }
struct PtrRepr_u64 {
    int64_t first[1];
    int32_t second[2];
    uint8_t third;
    uint8_t fourth;
};

int main() {
    struct PtrComponents_u64 _5;
    struct PtrRepr_u64 _4;

    //Init individually => Avoid gcc generating additional memcpy's when using constant structs
    _5.ptr = NULL;
    _5.outer.inner = 1;
    _5.third = 2;
    _5.fourth = 3;
    _5.fifth = 4;
    _4.first[0] = 99;

    memcpy(&_4, &_5, 20); //Note: PtrComponents_u64 takes 20bytes in memory due to alignment

    assert(_4.first[0] == 0 && _4.second[0] == 1 && _4.second[1] == 2 && _4.third == 3 && _4.fourth == 4);
    return 0;
}