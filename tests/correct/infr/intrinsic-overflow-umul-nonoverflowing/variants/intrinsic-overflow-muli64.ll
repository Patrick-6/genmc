; Function Attrs: nonlazybind
define i1 @main() unnamed_addr #11 {
start:
  %0 = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 9223372036854775807, i64 2)
  %_5.0 = extractvalue { i64, i1 } %0, 0
  %_5.1 = extractvalue { i64, i1 } %0, 1
  %1 = call i1 @llvm.expect.i1(i1 %_5.1, i1 false)
  br i1 %1, label %panic, label %nonoverflowing
nonoverflowing:                                   ; preds = %start
  %result = icmp eq i64 %_5.0, 18446744073709551614
  br i1 %result, label %return, label %panic
return:
  ret i1 %result
panic:                                            ; preds = %start
  unreachable
}

declare { i64, i1 } @llvm.umul.with.overflow.i64(i64, i64)

declare i1 @llvm.expect.i1(i1, i1)