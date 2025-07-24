struct inner {
    int a;
    int b;
};

struct MyList {
    int created;
    struct inner y;
};

struct MyList list;

// Goal is to generate a memcpy with a constant expression Argument in LLVM-IR:
// call void @llvm.memcpy.p0.p0.i64(ptr align 4 getelementptr inbounds (%struct.MyList, ptr @list, i32 0, i32 1), ptr align 4 %elem, i64 8, i1 false)
int main() {
    struct inner elem = {0, 1};

    list.y = elem;

    return 0;
}