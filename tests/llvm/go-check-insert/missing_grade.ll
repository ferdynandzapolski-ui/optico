; RUN: opt -load-pass-plugin=libGOIRPasses.so -passes=go-check-insert -pass-remarks=go-check-insert -S %s 2>&1 | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define i32 @test_missing_load(ptr %p) {
; CHECK: remark: {{.*}} Missing grade for load; using TOP
; CHECK-LABEL: @test_missing_load
; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  %v = load i32, ptr %p
  ret i32 %v
}

define void @test_missing_free(ptr %p) {
; CHECK: remark: {{.*}} Missing grade for free; using TOP
; CHECK-LABEL: @test_missing_free
; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 })
  call void @free(ptr %p)
  ret void
}

declare void @free(ptr)
