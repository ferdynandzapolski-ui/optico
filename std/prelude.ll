; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"


define i32 @id(i32 %o) {
  %t1 = alloca i32
  store i32 %o, i32* %t1
  %t2 = load i32, i32* %t1
  ret i32 %t2
}

define i32 @left(i32 %p) {
  %t3 = alloca i32
  store i32 %p, i32* %t3
  ret i32 0
}

define i32 @right(i32 %p) {
  %t4 = alloca i32
  store i32 %p, i32* %t4
  ret i32 0
}

define i32 @is_some(i32 %o) {
  %t5 = alloca i32
  store i32 %o, i32* %t5
  ret i32 0
}

define i32 @unwrap(i32 %o) {
  %t6 = alloca i32
  store i32 %o, i32* %t6
  ret i32 0
}

define i32 @is_ok(i32 %r) {
  %t7 = alloca i32
  store i32 %r, i32* %t7
  ret i32 0
}

define i1 @is_digit(i8 %c) {
  %t8 = alloca i8
  store i8 %c, i8* %t8
  ret i32 0
}

define i1 @is_alpha(i8 %c) {
  %t9 = alloca i8
  store i8 %c, i8* %t9
  ret i32 0
}

define i1 @is_alnum(i8 %c) {
  %t10 = alloca i8
  store i8 %c, i8* %t10
  ret i32 0
}

define i1 @is_space(i8 %c) {
  %t11 = alloca i8
  store i8 %c, i8* %t11
  ret i32 0
}

define i1 @is_newline(i8 %c) {
  %t12 = alloca i8
  store i8 %c, i8* %t12
  ret i32 0
}

define i1 @is_hex_digit(i8 %c) {
  %t13 = alloca i8
  store i8 %c, i8* %t13
  ret i32 0
}

