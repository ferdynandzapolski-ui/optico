; RUN: %opt -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_mem_intrinsics(ptr %dst, ptr %src, i64 %len) {
; CHECK-LABEL: @test_mem_intrinsics
  %dst_inst = bitcast ptr %dst to ptr, !go.grade !1
  %src_inst = bitcast ptr %src to ptr, !go.grade !2

; CHECK: call void @llvm.go.check_store(ptr %dst_inst, %go.grade { i64 100, i64 200, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 %len)
; CHECK: call void @llvm.go.check_load(ptr %src_inst, %go.grade { i64 300, i64 400, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 %len)
; CHECK: call void @llvm.memcpy.p0.p0.i64(ptr %dst_inst, ptr %src_inst, i64 %len, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr %dst_inst, ptr %src_inst, i64 %len, i1 false)

; CHECK: call void @llvm.go.check_store(ptr %dst_inst, %go.grade { i64 100, i64 200, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 %len)
; CHECK: call void @llvm.go.check_load(ptr %src_inst, %go.grade { i64 300, i64 400, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 %len)
; CHECK: call void @llvm.memmove.p0.p0.i64(ptr %dst_inst, ptr %src_inst, i64 %len, i1 false)
  call void @llvm.memmove.p0.p0.i64(ptr %dst_inst, ptr %src_inst, i64 %len, i1 false)

; CHECK: call void @llvm.go.check_store(ptr %dst_inst, %go.grade { i64 100, i64 200, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 %len)
; CHECK: call void @llvm.memset.p0.i64(ptr %dst_inst, i8 0, i64 %len, i1 false)
  call void @llvm.memset.p0.i64(ptr %dst_inst, i8 0, i64 %len, i1 false)

  ret void
}

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memmove.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

!1 = !{%go.grade { i64 100, i64 200, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }}
!2 = !{%go.grade { i64 300, i64 400, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }}
