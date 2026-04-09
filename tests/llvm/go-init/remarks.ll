; RUN: %opt -passes=go-init -pass-remarks=go-init -S %s 2>&1 | FileCheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"

define void @test_dynamic_alloca(i32 %n) {
; CHECK: remark: <unknown>:0:0: Dynamic alloca size; using dynamic grade init
  %p = alloca i32, i32 %n, align 4
  ret void
}
