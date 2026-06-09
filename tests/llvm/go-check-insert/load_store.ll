; RUN: opt -load-pass-plugin=libGOIRPasses.so -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define i32 @test_load(ptr %p, %go.grade %g) {
; CHECK-LABEL: @test_load
; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade { i64 0, i64 100, i32 1, i32 0, i32 3, i32 0, i64 0, i32 0 }, i64 4)
; CHECK: %v = load i32, ptr %p
  %v = load i32, ptr %p, !go.grade !1
  ret i32 %v
}

define void @test_store(ptr %p, i32 %v, %go.grade %g) {
; CHECK-LABEL: @test_store
; CHECK: call void @llvm.go.check_store(ptr %p, %go.grade { i64 0, i64 100, i32 1, i32 0, i32 3, i32 0, i64 0, i32 0 }, i64 4)
; CHECK: store i32 %v, ptr %p
  store i32 %v, ptr %p, !go.grade !1
  ret void
}

!1 = !{!"go.grade", %go.grade { i64 0, i64 100, i32 1, i32 0, i32 3, i32 0, i64 0, i32 0 }}
