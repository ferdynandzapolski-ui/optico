; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-check-insert -S %s 2>&1 | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

; CHECK: remark: {{.*}} Missing grade for load; using TOP
; CHECK: remark: {{.*}} Missing grade for store; using TOP
; CHECK: remark: {{.*}} Missing grade for free; using TOP

define void @test_missing_grade(ptr %p) {
entry:
  ; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  %v = load i32, ptr %p, align 4

  ; CHECK: call void @llvm.go.check_store(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)
  store i32 42, ptr %p, align 4

  ; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 })
  call void @free(ptr %p)

  ret void
}

declare void @free(ptr)
