; RUN: %opt -passes=go-mem-intrinsic -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg)

define void @test_memset(ptr %dst, i8 %val, i64 %len) {
; CHECK-LABEL: @test_memset
; CHECK: [[ZEXT:%[0-9]+]] = zext i8 %val to i32
; CHECK: call void @__go_memset(ptr %dst, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i32 [[ZEXT]], i64 %len)
; CHECK-NOT: call void @llvm.memset
  call void @llvm.memset.p0.i64(ptr %dst, i8 %val, i64 %len, i1 false)
  ret void
}
