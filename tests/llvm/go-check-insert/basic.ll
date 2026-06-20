; RUN: %opt -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_load_store(ptr %p, ptr %g_p_ptr) {
; CHECK-LABEL: @test_load_store
  %g_p = load %go.grade, ptr %g_p_ptr
  %p_inst = bitcast ptr %p to ptr
  call void @llvm.metadata(ptr %p_inst, %go.grade %g_p)

; CHECK: call void @llvm.go.check_load(ptr %p_inst, %go.grade %g_p, i64 4)
; CHECK-NEXT: %v = load i32, ptr %p_inst, align 4
  %v = load i32, ptr %p_inst, align 4

; CHECK: call void @llvm.go.check_store(ptr %p_inst, %go.grade %g_p, i64 4)
; CHECK-NEXT: store i32 %v, ptr %p_inst, align 4
  store i32 %v, ptr %p_inst, align 4
  ret void
}

define void @test_free(ptr %p, ptr %g_p_ptr) {
; CHECK-LABEL: @test_free
  %g_p = load %go.grade, ptr %g_p_ptr
  %p_inst = bitcast ptr %p to ptr
  call void @llvm.metadata(ptr %p_inst, %go.grade %g_p)

; CHECK: call void @llvm.go.check_free(ptr %p_inst, %go.grade %g_p)
; CHECK-NEXT: call void @free(ptr %p_inst)
  call void @free(ptr %p_inst)
  ret void
}

declare void @free(ptr)
declare void @llvm.metadata(ptr, %go.grade)

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"go-tier", i32 0}

; Manual metadata attachment for testing
; Since the pass looks for !go.grade metadata on instructions
define void @test_metadata_attachment(ptr %p) {
; CHECK-LABEL: @test_metadata_attachment
  %p_inst = bitcast ptr %p to ptr, !go.grade !1
; CHECK: call void @llvm.go.check_load(ptr %p_inst, %go.grade { i64 100, i64 200, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  %v = load i32, ptr %p_inst
  ret void
}

!1 = !{%go.grade { i64 100, i64 200, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }}
