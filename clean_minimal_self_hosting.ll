; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"


define i32 @compile_source() {
    %t1 = alloca i32
  store i32 10, i32* %t1
    %t2 = alloca i32
  store i32 20, i32* %t2
    %t3 = alloca i32
  %t4 = load i32, i32* %t1
  %t5 = load i32, i32* %t2
  %t6 = add i32 %t4, %t5
  store i32 %t6, i32* %t3
  %t7 = load i32, i32* %t3
    ret i32 %t7
}

define i32 @main() {
  %t8 = call i32 @compile_source()
    ret i32 %t8
}



