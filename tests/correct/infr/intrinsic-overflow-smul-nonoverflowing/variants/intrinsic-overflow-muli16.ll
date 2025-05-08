; Function Attrs: nonlazybind
define i1 @main() unnamed_addr #11 {
start:
  %0 = call { i16, i1 } @llvm.smul.with.overflow.i16(i16 16383, i16 2)
  %_5.0 = extractvalue { i16, i1 } %0, 0
  %_5.1 = extractvalue { i16, i1 } %0, 1
  %1 = call i1 @llvm.expect.i1(i1 %_5.1, i1 false)
  br i1 %1, label %panic, label %nonoverflowing
nonoverflowing:                                   ; preds = %start
  %result = icmp eq i16 %_5.0, 32766
  br i1 %result, label %return, label %panic
return:
  ret i1 %result
panic:                                            ; preds = %start
  unreachable
}

declare { i16, i1 } @llvm.smul.with.overflow.i16(i16, i16)

declare i1 @llvm.expect.i1(i1, i1)