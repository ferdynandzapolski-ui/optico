; RUN: opt -load-pass-plugin=%llvmshlibdir/libGOIRPasses%shlibext -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_mem_intrinsics(ptr %dst, ptr %src, i64 %len, %go.grade %g_dst, %go.grade %g_src) {
; CHECK-LABEL: @test_mem_intrinsics
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false), !go.grade.dst !0, !go.grade.src !1
; CHECK: call void @llvm.go.check_store(ptr %dst, %go.grade %g_dst, i64 %len)
; CHECK: call void @llvm.go.check_load(ptr %src, %go.grade %g_src, i64 %len)
; CHECK: call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false)

  call void @llvm.memmove.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false), !go.grade.dst !0, !go.grade.src !1
; CHECK: call void @llvm.go.check_store(ptr %dst, %go.grade %g_dst, i64 %len)
; CHECK: call void @llvm.go.check_load(ptr %src, %go.grade %g_src, i64 %len)
; CHECK: call void @llvm.memmove.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false)

  call void @llvm.memset.p0.i64(ptr %dst, i8 0, i64 %len, i1 false), !go.grade.dst !0
; CHECK: call void @llvm.go.check_store(ptr %dst, %go.grade %g_dst, i64 %len)
; CHECK: call void @llvm.memset.p0.i64(ptr %dst, i8 0, i64 %len, i1 false)

  ret void
}

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memmove.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

!0 = !{metadata %go.grade %g_dst}
!1 = !{metadata %go.grade %g_src}
