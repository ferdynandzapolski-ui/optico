; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@KIND_INT = global i32 0
@KIND_BINOP = global i32 0
@KIND_RETURN = global i32 0
@TYPE_INT = global i32 0
@TYPE_VOID = global i32 0
@TYPE_OPTIC = global i32 0
@TYPE_CO = global i32 0
@TYPE_POINTER = global i32 0
@TYPE_STRUCT_T = global i32 0
@TYPE_RESOURCE = global i32 0

define i32 @compile_program() {
    %t1 = alloca i32
  store i32 1024, i32* %t1
    %t2 = alloca i32
  store i32 0, i32* %t2
    %t3 = alloca i32
  store i32 0, i32* %t3
    %t4 = alloca i32
  store i32 1, i32* %t4
    %t5 = alloca i32
  store i32 0, i32* %t5
  %t6 = load i32, i32* %t1
  %t7 = load i32, i32* %t3
  %t8 = add i32 %t6, %t7
    ret i32 %t8
  ret i32 0
}

define i32 @main() {
  %t9 = call i32 @compile_program()
    ret i32 %t9
  ret i32 0
}

