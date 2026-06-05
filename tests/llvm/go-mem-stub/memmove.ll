; RUN: %opt -passes=go-mem-intrinsic -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

declare void @llvm.memmove.p0.p0.i64(ptr nocapture writeonly, ptr nocapture readonly, i64, i1 immarg)

define void @test_memmove(ptr %dst, ptr %src, i64 %len) {
; CHECK-LABEL: @test_memmove
; CHECK: call void @__go_memmove(ptr %dst, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, ptr %src, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 %len, i32 0)
; CHECK-NOT: call void @llvm.memmove
  call void @llvm.memmove.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false)
  ret void
}
