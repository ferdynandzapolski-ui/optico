; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-init,go-propagate -S %s | FileCheck %s

define void @test_gep_prop(ptr %p) {
entry:
  %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 4)
  %q = getelementptr i32, ptr %p, i64 1
  ; CHECK: %g_q = call %go.grade @llvm.go.gep_grade(%go.grade %g_p, i64 4, i64 1)
  ret void
}

declare %go.grade @llvm.go.grade_from_alloca(ptr, i64)
declare %go.grade @llvm.go.gep_grade(%go.grade, i64, i64)

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }
