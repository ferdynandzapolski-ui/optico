; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare void @print_int(i32)
@input_pos = global i32 0
@current_token = global i32 0
@token_value = global i32 0

define void @lexer_init(i32 %input) {
  %t1 = alloca i32
  store i32 %input, i32* %t1
    store i32 0, i32* @input_pos    store i32 0, i32* @current_token    store i32 0, i32* @token_value  ret void
}

define i32 @get_char(i32 %input) {
  %t2 = alloca i32
  store i32 %input, i32* %t2
  ret i32 0
}

define void @advance() {
  %t3 = add i32 %input_pos, 1
  store i32 %t3, i32* @input_pos  ret void
}

define void @skip_whitespace(i32 %input) {
  %t4 = alloca i32
  store i32 %input, i32* %t4
    %t5 = alloca i32
  %t6 = load i32, i32* %t4
  %t7 = call i32 @get_char(i32 %t6)
  store i32 %t7, i32* %t5
  %t8 = load i32, i32* %t5
  %t9 = icmp eq i1 %t8, 32
    br i1 %t9, label %then_9, label %else_9

then_9:
  call void @advance()
    br label %end_9

else_9:
  %t11 = load i32, i32* %t5
  %t12 = icmp eq i1 %t11, 9
    br i1 %t12, label %then_12, label %else_12

then_12:
  call void @advance()
    br label %end_12

else_12:
  %t14 = load i32, i32* %t5
  %t15 = icmp eq i1 %t14, 10
    br i1 %t15, label %then_15, label %else_15

then_15:
  call void @advance()
    br label %end_15

else_15:
    br label %end_15

end_15:
    br label %end_12

end_12:
    br label %end_9

end_9:
    %t17 = load i32, i32* %t4
  %t18 = call i32 @get_char(i32 %t17)
  store i32 %t18, i32* %t5
  %t19 = load i32, i32* %t5
  %t20 = icmp eq i1 %t19, 32
    br i1 %t20, label %then_20, label %else_20

then_20:
  call void @advance()
    br label %end_20

else_20:
  %t22 = load i32, i32* %t5
  %t23 = icmp eq i1 %t22, 9
    br i1 %t23, label %then_23, label %else_23

then_23:
  call void @advance()
    br label %end_23

else_23:
  %t25 = load i32, i32* %t5
  %t26 = icmp eq i1 %t25, 10
    br i1 %t26, label %then_26, label %else_26

then_26:
  call void @advance()
    br label %end_26

else_26:
    br label %end_26

end_26:
    br label %end_23

end_23:
    br label %end_20

end_20:
  ret void
}

define void @read_number(i32 %input) {
  %t28 = alloca i32
  store i32 %input, i32* %t28
    store i32 0, i32* @token_value    %t29 = alloca i32
  %t30 = load i32, i32* %t28
  %t31 = call i32 @get_char(i32 %t30)
  store i32 %t31, i32* %t29
  %t32 = load i32, i32* %t29
  %t33 = icmp sge i1 %t32, 48
    br i1 %t33, label %then_33, label %else_33

then_33:
  %t35 = load i32, i32* %t29
  %t36 = icmp sle i1 %t35, 57
    br i1 %t36, label %then_36, label %else_36

then_36:
    store i32 %t39, i32* @token_value  call void @advance()
    br label %end_36

else_36:
    br label %end_36

end_36:
    br label %end_33

else_33:
    br label %end_33

end_33:
    %t40 = load i32, i32* %t28
  %t41 = call i32 @get_char(i32 %t40)
  store i32 %t41, i32* %t29
  %t42 = load i32, i32* %t29
  %t43 = icmp sge i1 %t42, 48
    br i1 %t43, label %then_43, label %else_43

then_43:
  %t45 = load i32, i32* %t29
  %t46 = icmp sle i1 %t45, 57
    br i1 %t46, label %then_46, label %else_46

then_46:
    store i32 %t49, i32* @token_value  call void @advance()
    br label %end_46

else_46:
    br label %end_46

end_46:
    br label %end_43

else_43:
    br label %end_43

end_43:
  ret void
}

define void @lex(i32 %input) {
  %t50 = alloca i32
  store i32 %input, i32* %t50
  %t51 = load i32, i32* %t50
  call void @skip_whitespace(i32 %t51)
    %t52 = alloca i32
  %t53 = load i32, i32* %t50
  %t54 = call i32 @get_char(i32 %t53)
  store i32 %t54, i32* %t52
  %t55 = load i32, i32* %t52
  %t56 = icmp eq i1 %t55, 0
    br i1 %t56, label %then_56, label %else_56

then_56:
    store i32 0, i32* @current_token    br label %end_56

else_56:
  %t58 = load i32, i32* %t52
  %t59 = icmp sge i1 %t58, 48
  %t60 = load i32, i32* %t52
  %t61 = icmp sle i1 %t60, 57
  %t62 = and i32 %t59, %t61
    br i1 %t62, label %then_62, label %else_62

then_62:
  %t64 = load i32, i32* %t50
  call void @read_number(i32 %t64)
    store i32 1, i32* @current_token    br label %end_62

else_62:
  %t65 = load i32, i32* %t52
  %t66 = icmp eq i1 %t65, 43
    br i1 %t66, label %then_66, label %else_66

then_66:
  call void @advance()
    store i32 2, i32* @current_token    br label %end_66

else_66:
  call void @advance()
    store i32 99, i32* @current_token    br label %end_66

end_66:
    br label %end_62

end_62:
    br label %end_56

end_56:
  ret void
}

define i32 @parse_primary(i32 %input) {
  %t68 = alloca i32
  store i32 %input, i32* %t68
  ret i32 0
}

define i32 @parse_expr(i32 %input) {
  %t69 = alloca i32
  store i32 %input, i32* %t69
    %t70 = alloca i32
  %t71 = load i32, i32* %t69
  %t72 = call i32 @parse_primary(i32 %t71)
  store i32 %t72, i32* %t70
  %t73 = icmp eq i1 %current_token, 2
    br i1 %t73, label %then_73, label %else_73

then_73:
  %t75 = load i32, i32* %t69
  call void @lex(i32 %t75)
    %t76 = alloca i32
  %t77 = load i32, i32* %t69
  %t78 = call i32 @parse_primary(i32 %t77)
  store i32 %t78, i32* %t76
  %t79 = load i32, i32* %t70
  %t80 = load i32, i32* %t76
  %t81 = add i32 %t79, %t80
    br label %end_73

else_73:
    br label %end_73

end_73:
  ret i32 0
}

define i32 @compile(i32 %source) {
  %t82 = alloca i32
  store i32 %source, i32* %t82
  %t83 = load i32, i32* %t82
  call void @lexer_init(i32 %t83)
  %t84 = load i32, i32* %t82
  call void @lex(i32 %t84)
    %t85 = alloca i32
  %t86 = load i32, i32* %t82
  %t87 = call i32 @parse_expr(i32 %t86)
  store i32 %t87, i32* %t85
  %t88 = load i32, i32* %t85
  call void @print_int(i32 %t88)
  ret i32 0
}

