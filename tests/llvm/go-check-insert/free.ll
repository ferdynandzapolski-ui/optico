; RUN: opt -load-pass-plugin=libGOIRPasses.so -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

declare void @free(ptr)

define void @test_free(ptr %p, %go.grade %g) {
; CHECK-LABEL: @test_free
; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade { i64 0, i64 100, i32 1, i32 0, i32 3, i32 0, i64 0, i32 0 })
; CHECK: call void @free(ptr %p)
  call void @free(ptr %p), !go.grade !1
  ret void
}

!1 = !{!"go.grade", %go.grade { i64 0, i64 100, i32 1, i32 0, i32 3, i32 0, i64 0, i32 0 }}
