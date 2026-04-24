; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @print_int(i32)

define i32 @lexer_new(i32 %src) {
  %t1 = alloca i32
  store i32 %src, i32* %t1
    %t2 = alloca i32
  store i32 0, i32* %t2
  ret i32 0
}

define i32 @lexer_next(i32 %l) {
  %t4 = alloca i32
  store i32 %l, i32* %t4
  ret i32 0
}

define i32 @eval_expr(i32 %l) {
  %t5 = alloca i32
  store i32 %l, i32* %t5
    %t6 = alloca i32
  store i32 0, i32* %t6
  %t7 = load i32, i32* %t5
  %t8 = call i32 @lexer_next(i32 %t7)
  %t9 = icmp eq i1 0, 2
    br i1 %t9, label %then_9, label %else_9

then_9:
  %t11 = load i32, i32* %t5
  %t12 = call i32 @lexer_next(i32 %t11)
    %t13 = alloca i32
  %t14 = load i32, i32* %t5
  %t15 = call i32 @eval_expr(i32 %t14)
  store i32 %t15, i32* %t13
  %t16 = load i32, i32* %t6
  %t17 = load i32, i32* %t13
  %t18 = add i32 %t16, %t17
    br label %end_9

else_9:
    br label %end_9

end_9:
  ret i32 0
}

define i32 @compile(i32 %source) {
  %t19 = alloca i32
  store i32 %source, i32* %t19
    %t20 = alloca i32
  %t21 = load i32, i32* %t19
  %t22 = call i32 @lexer_new(i32 %t21)
  store i32 %t22, i32* %t20
  %t23 = load i32, i32* %t20
  %t24 = call i32 @lexer_next(i32 %t23)
    %t25 = alloca i32
  %t26 = load i32, i32* %t20
  %t27 = call i32 @eval_expr(i32 %t26)
  store i32 %t27, i32* %t25
  %t28 = load i32, i32* %t25
  call void @print_int(i32 %t28)
  ret i32 0
}

