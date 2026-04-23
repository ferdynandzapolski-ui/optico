; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @print_int(i32)

define void @test_variables() {
    %t1 = alloca i32
  store i32 10, i32* %t1
    %t2 = alloca i32
  store i32 20, i32* %t2
    %t3 = alloca i32
  %t4 = add i32 %x, %y
  store i32 %t4, i32* %t3
  call void @print_int(i32 %z)
  ret void
}

define void @main() {
  call void @test_variables()
  ret void
}

