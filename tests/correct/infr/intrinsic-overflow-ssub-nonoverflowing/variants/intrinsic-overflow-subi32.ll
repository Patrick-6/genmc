; Function Attrs: nonlazybind
define i1 @main() unnamed_addr #11 {
start:
  %0 = call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 -2147483647, i32 1)
  %_5.0 = extractvalue { i32, i1 } %0, 0
  %_5.1 = extractvalue { i32, i1 } %0, 1
  %1 = call i1 @llvm.expect.i1(i1 %_5.1, i1 false)
  br i1 %1, label %panic, label %nonoverflowing
nonoverflowing:                                   ; preds = %start
  %result = icmp eq i32 %_5.0, -2147483648
  br i1 %result, label %return, label %panic
return:
  ret i1 %result
panic:                                            ; preds = %start
  unreachable
}

declare { i32, i1 } @llvm.ssub.with.overflow.i32(i32, i32)

declare i1 @llvm.expect.i1(i1, i1)