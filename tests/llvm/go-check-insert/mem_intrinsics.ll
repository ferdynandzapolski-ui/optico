; RUN: opt -load-pass-plugin=libGOIRPasses.so -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg)
declare void @llvm.memmove.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg)

define void @test_memcpy(ptr %dst, ptr %src, %go.grade %gd, %go.grade %gs) {
; CHECK-LABEL: @test_memcpy
; CHECK: call void @__go_memcpy(ptr %dst, %go.grade { i64 0, i64 100, i32 1, i32 0, i32 3, i32 0, i64 0, i32 0 }, ptr %src, %go.grade { i64 200, i64 300, i32 2, i32 0, i32 3, i32 0, i64 0, i32 0 }, i64 16, i32 0)
; CHECK-NOT: call void @llvm.memcpy
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 16, i1 false), !go.grade.dst !1, !go.grade.src !2
  ret void
}

define void @test_memmove(ptr %dst, ptr %src, %go.grade %gd, %go.grade %gs) {
; CHECK-LABEL: @test_memmove
; CHECK: call void @__go_memmove(ptr %dst, %go.grade { i64 0, i64 100, i32 1, i32 0, i32 3, i32 0, i64 0, i32 0 }, ptr %src, %go.grade { i64 200, i64 300, i32 2, i32 0, i32 3, i32 0, i64 0, i32 0 }, i64 16, i32 0)
; CHECK-NOT: call void @llvm.memmove
  call void @llvm.memmove.p0.p0.i64(ptr %dst, ptr %src, i64 16, i1 false), !go.grade.dst !1, !go.grade.src !2
  ret void
}

define void @test_memset(ptr %p, %go.grade %g) {
; CHECK-LABEL: @test_memset
; CHECK: call void @__go_memset(ptr %p, %go.grade { i64 0, i64 100, i32 1, i32 0, i32 3, i32 0, i64 0, i32 0 }, i8 0, i64 16)
; CHECK-NOT: call void @llvm.memset
  call void @llvm.memset.p0.i64(ptr %p, i8 0, i64 16, i1 false), !go.grade !1
  ret void
}

!1 = !{!"go.grade", %go.grade { i64 0, i64 100, i32 1, i32 0, i32 3, i32 0, i64 0, i32 0 }}
!2 = !{!"go.grade", %go.grade { i64 200, i64 300, i32 2, i32 0, i32 3, i32 0, i64 0, i32 0 }}
