; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; External declarations
declare void @print_int(i32)

; Test while loop
define i32 @test_while_loop() {
  %sum_ptr = alloca i32
  store i32 0, i32* %sum_ptr
  %i_ptr = alloca i32
  store i32 1, i32* %i_ptr

  br label %loop

loop:
  %i_val = load i32, i32* %i_ptr
  %cond = icmp sle i32 %i_val, 5
  br i1 %cond, label %body, label %end

body:
  %sum_val = load i32, i32* %sum_ptr
  %new_sum = add i32 %sum_val, %i_val
  store i32 %new_sum, i32* %sum_ptr

  %new_i = add i32 %i_val, 1
  store i32 %new_i, i32* %i_ptr
  br label %loop

end:
  %final_sum = load i32, i32* %sum_ptr
  ret i32 %final_sum
}

; Test nested if-else
define i32 @test_nested_if(i32 %x) {
  %x_ptr = alloca i32
  store i32 %x, i32* %x_ptr

  %x_val = load i32, i32* %x_ptr
  %cmp1 = icmp sgt i32 %x_val, 10
  br i1 %cmp1, label %branch1, label %branch2

branch1:
  %cmp2 = icmp sgt i32 %x_val, 20
  br i1 %cmp2, label %ret100, label %ret50

branch2:
  %cmp3 = icmp sgt i32 %x_val, 5
  br i1 %cmp3, label %ret25, label %ret0

ret100:
  ret i32 100

ret50:
  ret i32 50

ret25:
  ret i32 25

ret0:
  ret i32 0
}

; Test complex control flow
define void @test_control_flow() {
  %result1 = call i32 @test_while_loop()
  call void @print_int(i32 %result1)

  %result2 = call i32 @test_nested_if(i32 15)
  call void @print_int(i32 %result2)

  %result3 = call i32 @test_nested_if(i32 7)
  call void @print_int(i32 %result3)

  %result4 = call i32 @test_nested_if(i32 3)
  call void @print_int(i32 %result4)

  ret void
}