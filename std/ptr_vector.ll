; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @malloc_ptr(i32)
declare void @free_ptr(i32)

define i32 @ptr_vector_new(i32 %cap) {
  %t1 = alloca i32
  store i32 %cap, i32* %t1
  ret i32 0
}

define void @ptr_vector_copy_rec(i32 %dst, i32 %src, i32 %count, i32 %i) {
  %t2 = alloca i32
  store i32 %dst, i32* %t2
  %t3 = alloca i32
  store i32 %src, i32* %t3
  %t4 = alloca i32
  store i32 %count, i32* %t4
  %t5 = alloca i32
  store i32 %i, i32* %t5
  ret void
}

define void @ptr_vector_push(i32 %v, i32 %val) {
  %t6 = alloca i32
  store i32 %v, i32* %t6
  %t7 = alloca i32
  store i32 %val, i32* %t7
  ret void
}

define void @ptr_vector_set(i32 %v, i32 %i, i32 %val) {
  %t8 = alloca i32
  store i32 %v, i32* %t8
  %t9 = alloca i32
  store i32 %i, i32* %t9
  %t10 = alloca i32
  store i32 %val, i32* %t10
  ret void
}

define i32 @ptr_vector_get(i32 %v, i32 %index) {
  %t12 = alloca i32
  store i32 %v, i32* %t12
  %t13 = alloca i32
  store i32 %index, i32* %t13
  ret i32 0
}

define i32 @ptr_vector_pop(i32 %v) {
  %t14 = alloca i32
  store i32 %v, i32* %t14
  ret i32 0
}

define void @ptr_vector_clear(i32 %v) {
  %t15 = alloca i32
  store i32 %v, i32* %t15
  ret void
}

define i32 @ptr_vector_size(i32 %v) {
  %t16 = alloca i32
  store i32 %v, i32* %t16
  ret i32 0
}

define i1 @ptr_vector_empty(i32 %v) {
  %t17 = alloca i32
  store i32 %v, i32* %t17
  %t18 = icmp eq i1 0, 0
  ret i32 %t18
}

