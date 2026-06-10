; RUN: opt -load-pass-plugin=%llvmshlibdir/libGOIRPasses.so -passes=go-check-insert -pass-remarks=go-check-insert -S %s 2>&1 | FileCheck %s

define i32 @test_missing_metadata(ptr %p) {
; CHECK: remark: <unknown>:0:0: Missing grade metadata; synthesizing TOP grade
; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  %v = load i32, ptr %p
  ret i32 %v
}
