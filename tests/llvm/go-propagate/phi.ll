; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-init,go-propagate -S %s | FileCheck %s

define void @test_phi_prop(ptr %p, ptr %q, i1 %cond) {
entry:
  %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 4)
  %g_q = call %go.grade @llvm.go.grade_from_alloca(ptr %q, i64 4)
  br i1 %cond, label %then, label %else

then:
  br label %merge

else:
  br label %merge

merge:
  %phi = phi ptr [ %p, %then ], [ %q, %else ]
  ; CHECK: %g_phi = call %go.grade @llvm.go.join_grade(%go.grade %g_p, %go.grade %g_q)
  ret void
}

declare %go.grade @llvm.go.grade_from_alloca(ptr, i64)
declare %go.grade @llvm.go.join_grade(%go.grade, %go.grade)

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }
