; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; External declarations
declare void @print_int(i32)

; Function with parameters and return
define i32 @add(i32 %x, i32 %y) {
  %x_ptr = alloca i32
  store i32 %x, i32* %x_ptr
  %y_ptr = alloca i32
  store i32 %y, i32* %y_ptr
  %x_val = load i32, i32* %x_ptr
  %y_val = load i32, i32* %y_ptr
  %result = add i32 %x_val, %y_val
  ret i32 %result
}

; Function calling other functions
define i32 @add_three(i32 %a, i32 %b, i32 %c) {
  %a_ptr = alloca i32
  store i32 %a, i32* %a_ptr
  %b_ptr = alloca i32
  store i32 %b, i32* %b_ptr
  %c_ptr = alloca i32
  store i32 %c, i32* %c_ptr

  %a_val = load i32, i32* %a_ptr
  %b_val = load i32, i32* %b_ptr
  %partial = call i32 @add(i32 %a_val, i32 %b_val)

  %c_val = load i32, i32* %c_ptr
  %result = call i32 @add(i32 %partial, i32 %c_val)
  ret i32 %result
}

; Void function
define void @print_sum(i32 %x, i32 %y) {
  %x_ptr = alloca i32
  store i32 %x, i32* %x_ptr
  %y_ptr = alloca i32
  store i32 %y, i32* %y_ptr

  %x_val = load i32, i32* %x_ptr
  %y_val = load i32, i32* %y_ptr
  %sum = call i32 @add(i32 %x_val, i32 %y_val)
  call void @print_int(i32 %sum)
  ret void
}

; Simple multiply function
define i32 @multiply(i32 %x, i32 %y) {
  %x_ptr = alloca i32
  store i32 %x, i32* %x_ptr
  %y_ptr = alloca i32
  store i32 %y, i32* %y_ptr
  %x_val = load i32, i32* %x_ptr
  %y_val = load i32, i32* %y_ptr
  %result = mul i32 %x_val, %y_val
  ret i32 %result
}

; Main test function
define void @test_functions() {
  call void @print_sum(i32 5, i32 3)
  %result1 = call i32 @add_three(i32 1, i32 2, i32 3)
  call void @print_int(i32 %result1)
  %result2 = call i32 @multiply(i32 6, i32 7)
  call void @print_int(i32 %result2)
  ret void
}