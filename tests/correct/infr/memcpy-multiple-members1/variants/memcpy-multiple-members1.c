#include <stdint.h>
#include <string.h>
#include <assert.h>

// { ptr, { i32 }, i32 }
struct PtrComponents_u64 {
    void* ptr;
    struct { int32_t inner; } outer;
    int32_t third;
};

// { [1 x i64], [2 x i32] }
struct PtrRepr_u64 {
    int64_t first[1];
    int32_t second[2];
};

int main() {
    struct PtrComponents_u64 _5;
    struct PtrRepr_u64 _4;

    //Init individually => Avoid gcc generating additional memcpy's when using constant structs
    _5.ptr = NULL;
    _5.outer.inner = 1;
    _5.third = 2;

    memcpy(&_4, &_5, 16); // Copy 16 bytes

    assert(_4.first[0] == 0 && _4.second[0] == 1 && _4.second[1] == 2);
    return 0;
}