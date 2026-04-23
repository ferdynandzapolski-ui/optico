; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @print_int(i32)

define i32 @test_arithmetic() {
    %t1 = alloca i32
  store i32 10, i32* %t1
    %t2 = alloca i32
  store i32 3, i32* %t2
    %t3 = alloca i32
  %t4 = load i32, i32* %t1
  %t5 = load i32, i32* %t2
  %t6 = mul i32 %t5, 2
  %t7 = add i32 %t4, %t6
  store i32 %t7, i32* %t3
  ret i32 0
}

define i32 @test_comparisons() {
    %t8 = alloca i32
  store i32 5, i32* %t8
    %t9 = alloca i32
  store i32 10, i32* %t9
    %t10 = alloca i32
  store i32 0, i32* %t10
  %t11 = load i32, i32* %t8
  %t12 = load i32, i32* %t9
  %t13 = icmp slt i1 %t11, %t12
    br i1 %t13, label %then_13, label %else_13

then_13:
    %t15 = load i32, i32* %t10
  %t16 = add i32 %t15, 1
  store i32 %t16, i32* %t10
    br label %end_13

else_13:
    br label %end_13

end_13:
  %t17 = load i32, i32* %t8
  %t18 = icmp eq i1 %t17, 5
    br i1 %t18, label %then_18, label %else_18

then_18:
    %t20 = load i32, i32* %t10
  %t21 = add i32 %t20, 2
  store i32 %t21, i32* %t10
    br label %end_18

else_18:
    br label %end_18

end_18:
  %t22 = load i32, i32* %t9
  %t23 = load i32, i32* %t8
  %t24 = icmp sgt i1 %t22, %t23
    br i1 %t24, label %then_24, label %else_24

then_24:
    %t26 = load i32, i32* %t10
  %t27 = add i32 %t26, 4
  store i32 %t27, i32* %t10
    br label %end_24

else_24:
    br label %end_24

end_24:
  ret i32 0
}

define void @test_expressions() {
  %t28 = call i32 @test_arithmetic()
  call void @print_int(i32 %t28)
  %t29 = call i32 @test_comparisons()
  call void @print_int(i32 %t29)
  ret void
}

