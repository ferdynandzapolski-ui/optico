; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"


define i32 @add(i32 %a, i32 %b) {
  %t1 = alloca i32
  store i32 %a, i32* %t1
  %t2 = alloca i32
  store i32 %b, i32* %t2
  %t3 = load i32, i32* %t1
  %t4 = load i32, i32* %t2
  %t5 = add i32 %t3, %t4
    ret i32 %t5
  ret i32 0
}

define i32 @mul(i32 %a, i32 %b) {
  %t6 = alloca i32
  store i32 %a, i32* %t6
  %t7 = alloca i32
  store i32 %b, i32* %t7
  %t8 = load i32, i32* %t7
    ret i32 %t8
  ret i32 0
}

define i32 @main() {
    %t9 = alloca i32
  store i32 10, i32* %t9
    %t10 = alloca i32
  store i32 20, i32* %t10
    %t11 = alloca i32
  %t12 = load i32, i32* %t9
  %t13 = load i32, i32* %t10
  %t14 = call i32 @add(i32 %t12, i32 %t13)
  store i32 %t14, i32* %t11
    %t15 = alloca i32
  %t16 = load i32, i32* %t9
  %t17 = load i32, i32* %t10
  %t18 = call i32 @mul(i32 %t16, i32 %t17)
  store i32 %t18, i32* %t15
    %t19 = alloca i32
  %t20 = load i32, i32* %t11
  %t21 = load i32, i32* %t15
  %t22 = add i32 %t20, %t21
  store i32 %t22, i32* %t19
  %t23 = load i32, i32* %t19
    ret i32 %t23
  ret i32 0
}

