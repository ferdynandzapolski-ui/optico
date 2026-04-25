; RUN: opt -load-pass-plugin=build/passes/libGOIRPasses.so -passes=go-init,go-propagate,go-mem-intrinsic,go-lower -S %s | FileCheck %s

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memmove.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

define void @test_mem(ptr %dst, ptr %src) {
; CHECK-LABEL: @test_mem
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 16, i1 false)
; CHECK: call void @__go_memcpy(ptr %dst, %go.grade {{.*}}, ptr %src, %go.grade {{.*}}, i64 16, i32 0)
  call void @llvm.memmove.p0.p0.i64(ptr %dst, ptr %src, i64 16, i1 false)
; CHECK: call void @__go_memmove(ptr %dst, %go.grade {{.*}}, ptr %src, %go.grade {{.*}}, i64 16, i32 0)
  call void @llvm.memset.p0.i64(ptr %dst, i8 0, i64 16, i1 false)
; CHECK: call void @__go_memset(ptr %dst, %go.grade {{.*}}, i8 0, i64 16)
  ret void
}
