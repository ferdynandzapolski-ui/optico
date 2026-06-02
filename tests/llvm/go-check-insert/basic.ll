; RUN: opt -load-pass-plugin=libGOIRPasses.so -passes=go-init,go-propagate,go-check-insert -S %s | FileCheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_basic(i32 %n) {
entry:
  %p = alloca i32, align 4
  ; CHECK: %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 4)
  ; CHECK: call void @llvm.go.check_store(ptr %p, %go.grade %g_p, i64 4)
  store i32 42, ptr %p, align 4
  ; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade %g_p, i64 4)
  %v = load i32, ptr %p, align 4
  ; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade %g_p)
  call void @free(ptr %p)
  ret void
}

declare void @free(ptr)
