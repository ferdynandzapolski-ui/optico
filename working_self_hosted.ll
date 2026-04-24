; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(i32)

define i32 @compile_program() {
  %t1 = call i32 @printf(i32 0)
  %t2 = call i32 @printf(i32 0)
  %t3 = call i32 @printf(i32 0)
  %t4 = call i32 @printf(i32 0)
  %t5 = call i32 @printf(i32 0)
  %t6 = call i32 @printf(i32 0)
  %t7 = call i32 @printf(i32 0)
  %t8 = call i32 @printf(i32 0)
  %t9 = call i32 @printf(i32 0)
  %t10 = call i32 @printf(i32 0)
  %t11 = call i32 @printf(i32 0)
  %t12 = call i32 @printf(i32 0)
  %t13 = call i32 @printf(i32 0)
  %t14 = call i32 @printf(i32 0)
  %t15 = call i32 @printf(i32 0)
  %t16 = call i32 @printf(i32 0)
    ret i32 30
  ret i32 0
}

define i32 @main() {
  %t17 = call i32 @compile_program()
    ret i32 %t17
  ret i32 0
}

