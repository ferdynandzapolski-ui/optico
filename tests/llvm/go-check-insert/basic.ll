; RUN: opt -load-pass-plugin=%llvmshlibdir/libGOIRPasses.so -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_load(ptr %p) {
; CHECK-LABEL: @test_load
; CHECK: [[G:%.*]] = load %go.grade, ptr
; CHECK-NEXT: call void @llvm.go.check_load(ptr %p, %go.grade [[G]], i64 4)
; CHECK-NEXT: %v = load i32, ptr %p
  %v = load i32, ptr %p
  ret void
}

define void @test_store(ptr %p) {
; CHECK-LABEL: @test_store
; CHECK: [[G:%.*]] = load %go.grade, ptr
; CHECK-NEXT: call void @llvm.go.check_store(ptr %p, %go.grade [[G]], i64 4)
; CHECK-NEXT: store i32 0, ptr %p
  store i32 0, ptr %p
  ret void
}

define void @test_free(ptr %p) {
; CHECK-LABEL: @test_free
; CHECK: [[G:%.*]] = load %go.grade, ptr
; CHECK-NEXT: call void @llvm.go.check_free(ptr %p, %go.grade [[G]])
; CHECK-NEXT: call void @free(ptr %p)
  call void @free(ptr %p)
  ret void
}

; Metadata setup for the tests
declare void @free(ptr)
