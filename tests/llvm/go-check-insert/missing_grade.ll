; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_missing_grade(ptr %p) {
entry:
  ; CHECK: @test_missing_grade
  ; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  %v = load i32, ptr %p
  ret void
}
