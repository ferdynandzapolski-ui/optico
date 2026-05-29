; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-init,go-propagate,go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_free(ptr %p) {
entry:
  %g_p = call %go.grade @llvm.go.grade_from_malloc(ptr %p, i64 4)
  ; CHECK: call void @llvm.go.check_free(ptr [[PTR:%p]], %go.grade %g_p)
  ; CHECK-NEXT: call void @free(ptr [[PTR]])
  call void @free(ptr %p)
  ret void
}

declare %go.grade @llvm.go.grade_from_malloc(ptr, i64)
declare void @free(ptr)
declare void @llvm.go.check_free(ptr, %go.grade)
