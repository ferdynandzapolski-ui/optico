; RUN: opt -load-pass-plugin=libGOIRPasses.so -passes=go-check-insert -S %s 2>&1 | FileCheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_missing_grade(ptr %p) {
entry:
  ; CHECK: Remark: Missing grade for pointer; using TOP grade
  ; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  %v = load i32, ptr %p, align 4
  ret void
}
