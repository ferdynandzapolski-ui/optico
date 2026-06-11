; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_basic_instrumentation(ptr %p) {
entry:
  %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 4)
  ; CHECK: %v = load i32, ptr %p, align 4, !go.grade ![[MD:[0-9]+]]
  %v = load i32, ptr %p, align 4, !go.grade !0
  ; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade %g_p, i64 4)
  ; CHECK-NEXT: %v = load i32, ptr %p

  ; CHECK: store i32 42, ptr %p, align 4, !go.grade ![[MD]]
  store i32 42, ptr %p, align 4, !go.grade !0
  ; CHECK: call void @llvm.go.check_store(ptr %p, %go.grade %g_p, i64 4)
  ; CHECK-NEXT: store i32 42, ptr %p

  ; CHECK: call void @free(ptr %p), !go.grade ![[MD]]
  call void @free(ptr %p), !go.grade !0
  ; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade %g_p)
  ; CHECK-NEXT: call void @free(ptr %p)

  ret void
}

declare void @free(ptr)
declare %go.grade @llvm.go.grade_from_alloca(ptr, i64)

!0 = !{ptr %g_p}
