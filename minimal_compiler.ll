; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @print_int(i32)

define i32 @lexer_new(i32 %src) {
  %t1 = alloca i32
  store i32 %src, i32* %t1
  ret i32 0
}

define i32 @lexer_next(i32 %l) {
  %t2 = alloca i32
  store i32 %l, i32* %t2
  ret i32 0
}

define i32 @eval_expr(i32 %l) {
  %t3 = alloca i32
  store i32 %l, i32* %t3
  %t4 = call i32 @lexer_next(i32 %l)
  ret i32 0
}

define i32 @compile(i32 %source) {
  %t5 = alloca i32
  store i32 %source, i32* %t5
  %t6 = call i32 @lexer_next(i32 %lexer)
  call void @print_int(i32 %result)
  ret i32 0
}

