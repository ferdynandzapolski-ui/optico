; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @print_int(i32)
declare i32 @malloc(i32)
declare void @free(i32)

define void @lexer_init(i32 %src) {
  %t1 = alloca i32
  store i32 %src, i32* %t1
  ret void
}

define i32 @lexer_next() {
  ret i32 0
}

define i32 @eval_expr() {
    %t2 = call i32 @lexer_next()
  ret i32 0
}

define i32 @compile(i32 %source) {
  %t3 = alloca i32
  store i32 %source, i32* %t3
    call void @lexer_init(i32 0)
    %t4 = call i32 @lexer_next()
    call void @print_int(i32 0)
  ret i32 0
}

