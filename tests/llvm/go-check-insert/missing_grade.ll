; RUN: opt -load-pass-plugin=%llvmshlibdir/libGOIRPasses%shlibext -passes=go-check-insert -pass-remarks=go-check-insert -S %s 2>&1 | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_missing(ptr %p) {
; CHECK: remark: <unknown>:0:0: Missing grade for load; using TOP
; CHECK: remark: <unknown>:0:0: Missing grade for store; using TOP
; CHECK: remark: <unknown>:0:0: Missing grade for free; using TOP

; CHECK-LABEL: @test_missing
  %v = load i32, ptr %p
; CHECK: call void @llvm.go.check_load(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)

  store i32 42, ptr %p
; CHECK: call void @llvm.go.check_store(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 4)

  call void @free(ptr %p)
; CHECK: call void @llvm.go.check_free(ptr %p, %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 })

  ret void
}

declare void @free(ptr)
