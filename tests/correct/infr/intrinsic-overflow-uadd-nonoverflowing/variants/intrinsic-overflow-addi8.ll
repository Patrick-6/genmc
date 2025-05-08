; Function Attrs: nonlazybind
define i1 @main() unnamed_addr #11 {
start:
  %0 = call { i8, i1 } @llvm.uadd.with.overflow.i8(i8 250, i8 5)
  %_5.0 = extractvalue { i8, i1 } %0, 0
  %_5.1 = extractvalue { i8, i1 } %0, 1
  %1 = call i1 @llvm.expect.i1(i1 %_5.1, i1 false)
  br i1 %1, label %panic, label %nonoverflowing
nonoverflowing:                                   ; preds = %start
  %result = icmp eq i8 %_5.0, 255
  br i1 %result, label %return, label %panic
return:
  ret i1 %result
panic:                                            ; preds = %start
  unreachable
}

declare { i8, i1 } @llvm.uadd.with.overflow.i8(i8, i8)

declare i1 @llvm.expect.i1(i1, i1)