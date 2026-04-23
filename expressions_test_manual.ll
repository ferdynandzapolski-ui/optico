; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; External declarations
declare void @print_int(i32)

; Test arithmetic expressions
define i32 @test_arithmetic() {
  %a_ptr = alloca i32
  store i32 10, i32* %a_ptr
  %b_ptr = alloca i32
  store i32 3, i32* %b_ptr

  ; c = a + b * 2
  %a_val = load i32, i32* %a_ptr
  %b_val = load i32, i32* %b_ptr
  %b_times_2 = mul i32 %b_val, 2
  %c_val = add i32 %a_val, %b_times_2
  %c_ptr = alloca i32
  store i32 %c_val, i32* %c_ptr

  ret i32 %c_val
}

; Test comparison expressions
define i32 @test_comparisons() {
  %x_ptr = alloca i32
  store i32 5, i32* %x_ptr
  %y_ptr = alloca i32
  store i32 10, i32* %y_ptr
  %result_ptr = alloca i32
  store i32 0, i32* %result_ptr

  %x_val = load i32, i32* %x_ptr
  %y_val = load i32, i32* %y_ptr

  ; if (x < y) result += 1
  %cmp1 = icmp slt i32 %x_val, %y_val
  br i1 %cmp1, label %add1, label %next1

add1:
  %r1 = load i32, i32* %result_ptr
  %r1_new = add i32 %r1, 1
  store i32 %r1_new, i32* %result_ptr
  br label %next1

next1:
  ; if (x == 5) result += 2
  %cmp2 = icmp eq i32 %x_val, 5
  br i1 %cmp2, label %add2, label %next2

add2:
  %r2 = load i32, i32* %result_ptr
  %r2_new = add i32 %r2, 2
  store i32 %r2_new, i32* %result_ptr
  br label %next2

next2:
  ; if (y > x) result += 4
  %cmp3 = icmp sgt i32 %y_val, %x_val
  br i1 %cmp3, label %add4, label %end

add4:
  %r3 = load i32, i32* %result_ptr
  %r3_new = add i32 %r3, 4
  store i32 %r3_new, i32* %result_ptr
  br label %end

end:
  %final_result = load i32, i32* %result_ptr
  ret i32 %final_result
}

; Main test
define void @test_expressions() {
  %arith_result = call i32 @test_arithmetic()
  call void @print_int(i32 %arith_result)

  %comp_result = call i32 @test_comparisons()
  call void @print_int(i32 %comp_result)

  ret void
}