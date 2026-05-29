; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-init,go-propagate,go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_load_store(ptr %p) {
entry:
  %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 4)
  ; CHECK: call void @llvm.go.check_store(ptr [[PTR:%p]], %go.grade %g_p, i64 4)
  ; CHECK-NEXT: store i32 42, ptr [[PTR]]
  store i32 42, ptr %p
  ; CHECK: call void @llvm.go.check_load(ptr [[PTR]], %go.grade %g_p, i64 4)
  ; CHECK-NEXT: %v = load i32, ptr [[PTR]]
  %v = load i32, ptr %p
  ret void
}

declare %go.grade @llvm.go.grade_from_alloca(ptr, i64)
declare void @llvm.go.check_load(ptr, %go.grade, i64)
declare void @llvm.go.check_store(ptr, %go.grade, i64)
