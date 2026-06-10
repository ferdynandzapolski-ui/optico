; RUN: opt -load-pass-plugin=%llvmshlibdir/libGOIRPasses.so -passes=go-mem-intrinsic -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_memcpy(ptr %dst, ptr %src, i64 %n) {
; CHECK-LABEL: @test_memcpy
; CHECK: call void @__go_memcpy(ptr %dst, %go.grade {{.*}}, ptr %src, %go.grade {{.*}}, i64 %n, i32 0)
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 %n, i1 false)
  ret void
}

define void @test_memmove(ptr %dst, ptr %src, i64 %n) {
; CHECK-LABEL: @test_memmove
; CHECK: call void @__go_memmove(ptr %dst, %go.grade {{.*}}, ptr %src, %go.grade {{.*}}, i64 %n, i32 0)
  call void @llvm.memmove.p0.p0.i64(ptr %dst, ptr %src, i64 %n, i1 false)
  ret void
}

define void @test_memset(ptr %dst, i8 %val, i64 %n) {
; CHECK-LABEL: @test_memset
; CHECK: call void @__go_memset(ptr %dst, %go.grade {{.*}}, i8 %val, i64 %n, i32 0)
  call void @llvm.memset.p0.i64(ptr %dst, i8 %val, i64 %n, i1 false)
  ret void
}

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memmove.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)
