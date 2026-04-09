; RUN: %opt -passes=go-init -S %s | FileCheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"

; CHECK: %go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_alloca() {
; CHECK-LABEL: @test_alloca(
; CHECK: %p = alloca i32, align 4
; CHECK: %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 4)
  %p = alloca i32, align 4
  ret void
}

define void @test_alloca_array() {
; CHECK-LABEL: @test_alloca_array(
; CHECK: %p = alloca i32, i32 10, align 4
; CHECK: %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 40)
  %p = alloca i32, i32 10, align 4
  ret void
}

define void @test_alloca_vla(i32 %n) {
; CHECK-LABEL: @test_alloca_vla(
; CHECK: %p = alloca i32, i32 %n, align 4
; CHECK: %1 = zext i32 %n to i64
; CHECK: %2 = mul i64 %1, 4
; CHECK: %g_p = call %go.grade @llvm.go.grade_from_alloca(ptr %p, i64 %2)
  %p = alloca i32, i32 %n, align 4
  ret void
}

declare ptr @malloc(i64)
declare ptr @calloc(i64, i64)
declare ptr @realloc(ptr, i64)

define void @test_malloc(i64 %n) {
; CHECK-LABEL: @test_malloc(
; CHECK: %p = call ptr @malloc(i64 %n)
; CHECK: %g_p = call %go.grade @llvm.go.grade_from_malloc(ptr %p, i64 %n)
  %p = call ptr @malloc(i64 %n)
  ret void
}

define void @test_calloc(i64 %n, i64 %s) {
; CHECK-LABEL: @test_calloc(
; CHECK: %p = call ptr @calloc(i64 %n, i64 %s)
; CHECK: %1 = mul i64 %n, %s
; CHECK: %g_p = call %go.grade @llvm.go.grade_from_malloc(ptr %p, i64 %1)
  %p = call ptr @calloc(i64 %n, i64 %s)
  ret void
}

define void @test_realloc(ptr %p, i64 %n) {
; CHECK-LABEL: @test_realloc(
; CHECK: %q = call ptr @realloc(ptr %p, i64 %n)
; CHECK: %g_p = call %go.grade @llvm.go.grade_from_malloc(ptr %q, i64 %n)
  %q = call ptr @realloc(ptr %p, i64 %n)
  ret void
}
