; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-check-insert -S %s 2>&1 | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

; CHECK: remark: {{.*}} Missing grade; synthesizing TOP
; CHECK: remark: {{.*}} Missing grade; synthesizing TOP

define void @test_missing_grade(ptr %p) {
entry:
  ; CHECK: call void @llvm.go.check_store(ptr [[PTR:%p]], %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  ; CHECK-NEXT: store i32 42, ptr [[PTR]]
  store i32 42, ptr %p
  ; CHECK: call void @llvm.go.check_load(ptr [[PTR]], %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  ; CHECK-NEXT: %v = load i32, ptr [[PTR]]
  %v = load i32, ptr %p
  ret void
}

declare void @llvm.go.check_load(ptr, %go.grade, i64)
declare void @llvm.go.check_store(ptr, %go.grade, i64)
