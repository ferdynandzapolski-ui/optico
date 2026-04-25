; RUN: opt -load-pass-plugin=build/passes/libGOIRPasses.so -passes=go-init,go-propagate,go-check-insert -go-tier=1 -S %s | FileCheck %s

define ptr @test_shadow(ptr %p, ptr %q) {
; CHECK-LABEL: @test_shadow
  %val = load ptr, ptr %p
; CHECK: %shadow_g = call %go.grade @llvm.go.shadow_load(ptr %p)
; CHECK: load ptr, ptr %p, !go.grade ![[MD:[0-9]+]]
  store ptr %q, ptr %p
; CHECK: call void @llvm.go.shadow_store(ptr %p, %go.grade {{.*}})
  ret ptr %val
}

; CHECK: ![[MD]] = !{ptr %shadow_g}
