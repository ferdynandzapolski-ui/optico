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

define i32 @string_len(i32 %s) {
  %t14 = alloca i32
  store i32 %s, i32* %t14
  ret i32 0
}

define i8 @string_at(i32 %s, i32 %index) {
  %t15 = alloca i32
  store i32 %s, i32* %t15
  %t16 = alloca i32
  store i32 %index, i32* %t16
  ret i32 0
}

define i1 @string_equal(i32 %s1, i32 %s2) {
  %t17 = alloca i32
  store i32 %s1, i32* %t17
  %t18 = alloca i32
  store i32 %s2, i32* %t18
  ret i32 0
}

define i32 @string_find(i32 %s, i8 %c, i32 %start) {
  %t19 = alloca i32
  store i32 %s, i32* %t19
  %t20 = alloca i8
  store i8 %c, i8* %t20
  %t21 = alloca i32
  store i32 %start, i32* %t21
  ret i32 0
}

define i32 @char_to_int(i8 %c) {
  %t22 = alloca i8
  store i8 %c, i8* %t22
  ret i32 0
}

define i32 @string_to_int(i32 %s) {
  %t23 = alloca i32
  store i32 %s, i32* %t23
  %t24 = call i32 @string_to_int_rec(i32 0, i32 0, i32 0, i32 0)
  ret i32 %t24
}

define i32 @string_to_int_rec(i32 %data, i32 %len, i32 %i, i32 %acc) {
  %t25 = alloca i32
  store i32 %data, i32* %t25
  %t26 = alloca i32
  store i32 %len, i32* %t26
  %t27 = alloca i32
  store i32 %i, i32* %t27
  %t28 = alloca i32
  store i32 %acc, i32* %t28
  ret i32 0
}

define i32 @string_slice(i32 %s, i32 %start, i32 %end) {
  %t29 = alloca i32
  store i32 %s, i32* %t29
  %t30 = alloca i32
  store i32 %start, i32* %t30
  %t31 = alloca i32
  store i32 %end, i32* %t31
  ret i32 0
}

define i32 @string_push_char(i32 %s, i8 %c) {
  %t32 = alloca i32
  store i32 %s, i32* %t32
  %t33 = alloca i8
  store i8 %c, i8* %t33
  ret i32 0
}

define i32 @string_concat(i32 %s1, i32 %s2) {
  %t34 = alloca i32
  store i32 %s1, i32* %t34
  %t35 = alloca i32
  store i32 %s2, i32* %t35
  ret i32 0
}

define void @string_copy_rec(i32 %dst, i32 %src, i32 %len, i32 %dst_off, i32 %i) {
  %t36 = alloca i32
  store i32 %dst, i32* %t36
  %t37 = alloca i32
  store i32 %src, i32* %t37
  %t38 = alloca i32
  store i32 %len, i32* %t38
  %t39 = alloca i32
  store i32 %dst_off, i32* %t39
  %t40 = alloca i32
  store i32 %i, i32* %t40
  ret void
}

define i1 @string_equal_rec(i32 %d1, i32 %d2, i32 %len, i32 %i) {
  %t41 = alloca i32
  store i32 %d1, i32* %t41
  %t42 = alloca i32
  store i32 %d2, i32* %t42
  %t43 = alloca i32
  store i32 %len, i32* %t43
  %t44 = alloca i32
  store i32 %i, i32* %t44
  ret i32 0
}

define i32 @string_compare(i32 %s1, i32 %s2) {
  %t45 = alloca i32
  store i32 %s1, i32* %t45
  %t46 = alloca i32
  store i32 %s2, i32* %t46
  %t47 = call i32 @string_compare_rec(i32 0, i32 0, i32 0, i32 0, i32 0)
  ret i32 %t47
}

define i32 @string_compare_rec(i32 %d1, i32 %l1, i32 %d2, i32 %l2, i32 %i) {
  %t48 = alloca i32
  store i32 %d1, i32* %t48
  %t49 = alloca i32
  store i32 %l1, i32* %t49
  %t50 = alloca i32
  store i32 %d2, i32* %t50
  %t51 = alloca i32
  store i32 %l2, i32* %t51
  %t52 = alloca i32
  store i32 %i, i32* %t52
  ret i32 0
}

define i1 @string_starts_with(i32 %s, i32 %prefix) {
  %t53 = alloca i32
  store i32 %s, i32* %t53
  %t54 = alloca i32
  store i32 %prefix, i32* %t54
  ret i32 0
}

define i1 @string_starts_with_rec(i32 %d, i32 %p, i32 %len, i32 %i) {
  %t55 = alloca i32
  store i32 %d, i32* %t55
  %t56 = alloca i32
  store i32 %p, i32* %t56
  %t57 = alloca i32
  store i32 %len, i32* %t57
  %t58 = alloca i32
  store i32 %i, i32* %t58
  ret i32 0
}

