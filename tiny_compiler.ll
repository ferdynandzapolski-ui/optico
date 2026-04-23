; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @print_int(i32)

define void @init_parser(i32 %input) {
  %t1 = alloca i32
  store i32 %input, i32* %t1
  ret void
}

define i32 @get_digit(i32 %input) {
  %t2 = alloca i32
  store i32 %input, i32* %t2
  ret i32 0
}

define i32 @parse_number(i32 %input) {
  %t3 = alloca i32
  store i32 %input, i32* %t3
  ret i32 0
}

define i32 @parse_expr(i32 %input) {
  %t4 = alloca i32
  store i32 %input, i32* %t4
  ret i32 0
}

define i32 @compile(i32 %source) {
  %t5 = alloca i32
  store i32 %source, i32* %t5
  %t6 = call i32 @init_parser(i32 %source)
  call void @print_int(i32 %result)
  ret i32 0
}

