#include <cassert>

class TestClass {
    public:
        static constexpr int constr_does_not_write_this = 0;
        int const constr_writes_this = 1;
};

int main() {
    TestClass t;
    assert(0 && "static constexpr variable is not initialized by ctor");
    return 0;
}