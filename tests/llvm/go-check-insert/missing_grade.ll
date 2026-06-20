; RUN: %opt -passes=go-check-insert -pass-remarks=go-check-insert -S %s 2>&1 | FileCheck %s

; CHECK: remark: {{.*}} Missing grade for load; using TOP
; CHECK: remark: {{.*}} Missing grade for store; using TOP
; CHECK: remark: {{.*}} Missing grade for free; using TOP

define void @test_missing(ptr %p) {
; CHECK-LABEL: @test_missing
; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  %v = load i32, ptr %p
; CHECK: call void @llvm.go.check_store(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  store i32 %v, ptr %p
; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 })
  call void @free(ptr %p)
  ret void
}

declare void @free(ptr)
