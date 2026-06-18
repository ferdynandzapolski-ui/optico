; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_load(ptr %p) {
entry:
  ; CHECK: @test_load
  ; CHECK: [[G:%[0-9]+]] = call %go.grade @llvm.go.grade_from_alloca
  %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 4)
  ; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade [[G]], i64 4)
  ; CHECK: load i32, ptr %p
  %v = load i32, ptr %p
  ret void
}

define void @test_store(ptr %p, i32 %v) {
entry:
  ; CHECK: @test_store
  ; CHECK: [[G:%[0-9]+]] = call %go.grade @llvm.go.grade_from_alloca
  %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 4)
  ; CHECK: call void @llvm.go.check_store(ptr %p, %go.grade [[G]], i64 4)
  ; CHECK: store i32 %v, ptr %p
  store i32 %v, ptr %p
  ret void
}

define void @test_free(ptr %p) {
entry:
  ; CHECK: @test_free
  ; CHECK: [[G:%[0-9]+]] = call %go.grade @llvm.go.grade_from_alloca
  %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 4)
  ; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade [[G]])
  ; CHECK: call void @free(ptr %p)
  call void @free(ptr %p)
  ret void
}

declare void @free(ptr)
declare %go.grade @llvm.go.grade_from_alloca(ptr, i64)
declare void @llvm.go.check_load(ptr, %go.grade, i64)
declare void @llvm.go.check_store(ptr, %go.grade, i64)
declare void @llvm.go.check_free(ptr, %go.grade)
