; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @print_int(i32)

define i32 @add(i32 %x, i32 %y) {
  %t1 = alloca i32
  store i32 %x, i32* %t1
  %t2 = alloca i32
  store i32 %y, i32* %t2
  ret i32 0
}

define void @main() {
    call void @print_int(i32 0)
  ret void
}

