; RUN: %opt -passes=go-mem-intrinsic -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

declare void @llvm.memcpy.p0.p0.i64(ptr nocapture writeonly, ptr nocapture readonly, i64, i1 immarg)

define void @test_memcpy(ptr %dst, ptr %src, i64 %len) {
; CHECK-LABEL: @test_memcpy
; CHECK: call void @__go_memcpy(ptr %dst, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, ptr %src, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 %len, i32 0)
; CHECK-NOT: call void @llvm.memcpy
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false)
  ret void
}

define void @test_memcpy_with_metadata(ptr %dst, ptr %src, i64 %len, %go.grade %gd, %go.grade %gs) {
; CHECK-LABEL: @test_memcpy_with_metadata
; CHECK: call void @__go_memcpy(ptr %dst, %go.grade undef, ptr %src, %go.grade undef, i64 %len, i32 0)
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false), !go.grade.dst !0, !go.grade.src !1
  ret void
}

!0 = !{ptr undef}
!1 = !{ptr undef}
