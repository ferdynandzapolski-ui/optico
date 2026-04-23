; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @print_int(i32)

define i32 @add(i32 %x, i32 %y) {
  %t1 = alloca i32
  store i32 %x, i32* %t1
  %t2 = alloca i32
  store i32 %y, i32* %t2
  %t3 = load i32, i32* %t1
  %t4 = load i32, i32* %t2
  %t5 = add i32 %t3, %t4
  ret i32 %t5
}

define void @optico_main() {
  %t6 = call i32 @add(i32 1, i32 2)
  call void @print_int(i32 %t6)
  ret void
}

