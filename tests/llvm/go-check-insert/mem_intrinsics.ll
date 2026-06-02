; RUN: opt -load-pass-plugin=libGOIRPasses.so -passes=go-init,go-propagate,go-check-insert -S %s | FileCheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_mem_intrinsics(ptr %dst, ptr %src, i64 %len) {
entry:
  ; CHECK: %g_p = call %go.grade @llvm.go.grade_from_malloc(ptr %dst, i64 100)
  %g_dst = call %go.grade @llvm.go.grade_from_malloc(ptr %dst, i64 100)
  ; CHECK: %g_p1 = call %go.grade @llvm.go.grade_from_malloc(ptr %src, i64 100)
  %g_src = call %go.grade @llvm.go.grade_from_malloc(ptr %src, i64 100)

  ; CHECK: call void @llvm.go.check_store(ptr %dst, %go.grade %g_p, i64 %len)
  ; CHECK: call void @llvm.go.check_load(ptr %src, %go.grade %g_p1, i64 %len)
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false)

  ; CHECK: call void @llvm.go.check_store(ptr %dst, %go.grade %g_p, i64 %len)
  ; CHECK: call void @llvm.go.check_load(ptr %src, %go.grade %g_p1, i64 %len)
  call void @llvm.memmove.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false)

  ; CHECK: call void @llvm.go.check_store(ptr %dst, %go.grade %g_p, i64 %len)
  call void @llvm.memset.p0.i64(ptr %dst, i8 0, i64 %len, i1 false)

  ret void
}

declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg)
declare void @llvm.memmove.p0.p0.i64(ptr nocapture writeonly, ptr nocapture readonly, i64, i1 immarg)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg)
declare %go.grade @llvm.go.grade_from_malloc(ptr, i64)
