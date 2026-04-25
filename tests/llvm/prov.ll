; RUN: opt -load-pass-plugin=build/passes/libGOIRPasses.so -passes=go-init,go-propagate,go-check-insert -go-tier=0 -go-prov-policy=1 -S %s | FileCheck %s

define void @test_prov(ptr %p) {
; CHECK-LABEL: @test_prov
  %i = ptrtoint ptr %p to i64
; CHECK: call void @llvm.go.prov_expose(ptr %p, %go.grade {{.*}})
  %p2 = inttoptr i64 %i to ptr
; CHECK: %res = call { ptr, %go.grade } @llvm.go.inttoptr_resolve(i64 %i, i32 1)
; CHECK: %new_ptr = extractvalue { ptr, %go.grade } %res, 0
; CHECK: %new_g = extractvalue { ptr, %go.grade } %res, 1
  ret void
}
