; RUN: %opt -passes=go-init,go-propagate,go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_load_store(ptr %p) {
; CHECK-LABEL: @test_load_store
; CHECK: %g_p = call %go.grade @llvm.go.grade_from_alloca
; CHECK: call void @llvm.go.check_store(ptr %p1, %go.grade %g_p, i64 4)
; CHECK: store i32 42, ptr %p1
; CHECK: call void @llvm.go.check_load(ptr %p1, %go.grade %g_p, i64 4)
; CHECK: %v = load i32, ptr %p1
  %p1 = alloca i32, align 4
  store i32 42, ptr %p1, align 4
  %v = load i32, ptr %p1, align 4
  ret void
}

define void @test_free(ptr %p) {
; CHECK-LABEL: @test_free
; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade {{.*}})
; CHECK: call void @free(ptr %p)
  call void @free(ptr %p)
  ret void
}

declare void @free(ptr)
