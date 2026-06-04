; RUN: opt -load-pass-plugin=%llvmshlibdir/libGOIRPasses%shlibext -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_load_store_free(ptr %p, %go.grade %g) {
; CHECK-LABEL: @test_load_store_free
  %p_val = load i32, ptr %p, !go.grade !0
; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade %g, i64 4)
; CHECK-NEXT: %p_val = load i32, ptr %p

  store i32 42, ptr %p, !go.grade !0
; CHECK: call void @llvm.go.check_store(ptr %p, %go.grade %g, i64 4)
; CHECK-NEXT: store i32 42, ptr %p

  call void @free(ptr %p), !go.grade !0
; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade %g)
; CHECK-NEXT: call void @free(ptr %p)

  ret void
}

declare void @free(ptr)

!0 = !{metadata %go.grade %g}
