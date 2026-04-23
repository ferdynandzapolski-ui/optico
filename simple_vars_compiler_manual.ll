; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; External declarations
declare void @print_int(i32)

; Global variables
@input_pos = global i32 0
@current_val = global i32 0

; Initialize parser
define void @init_parser(i8* %input) {
  store i32 0, i32* @input_pos
  store i32 0, i32* @current_val
  ret void
}

; Get digit value
define i32 @get_digit(i8* %input) {
  %pos = load i32, i32* @input_pos
  %char_ptr = getelementptr i8, i8* %input, i32 %pos
  %char = load i8, i8* %char_ptr
  %digit = sub i8 %char, 48  ; '0' = 48
  %sext_digit = sext i8 %digit to i32
  %new_pos = add i32 %pos, 1
  store i32 %new_pos, i32* @input_pos
  ret i32 %sext_digit
}

; Parse number
define i32 @parse_number(i8* %input) {
  %val_ptr = alloca i32
  store i32 0, i32* %val_ptr
  br label %loop

loop:
  %pos = load i32, i32* @input_pos
  %char_ptr = getelementptr i8, i8* %input, i32 %pos
  %char = load i8, i8* %char_ptr
  %is_digit = icmp uge i8 %char, 48  ; >= '0'
  %is_digit2 = icmp ule i8 %char, 57 ; <= '9'
  %is_valid = and i1 %is_digit, %is_digit2
  br i1 %is_valid, label %digit, label %end

digit:
  %val = load i32, i32* %val_ptr
  %new_val = mul i32 %val, 10
  %digit_val = call i32 @get_digit(i8* %input)
  %final_val = add i32 %new_val, %digit_val
  store i32 %final_val, i32* %val_ptr
  br label %loop

end:
  %result = load i32, i32* %val_ptr
  ret i32 %result
}

; Parse expression
define i32 @parse_expr(i8* %input) {
  %left = call i32 @parse_number(i8* %input)

  %pos = load i32, i32* @input_pos
  %char_ptr = getelementptr i8, i8* %input, i32 %pos
  %char = load i8, i8* %char_ptr
  %is_plus = icmp eq i8 %char, 43  ; '+'
  br i1 %is_plus, label %plus, label %end

plus:
  %new_pos = add i32 %pos, 1
  store i32 %new_pos, i32* @input_pos
  %right = call i32 @parse_expr(i8* %input)
  %result = add i32 %left, %right
  ret i32 %result

end:
  ret i32 %left
}

; Test function with variables
define void @test_variables() {
  ; int x = 10;
  %x_ptr = alloca i32
  store i32 10, i32* %x_ptr

  ; int y = 20;
  %y_ptr = alloca i32
  store i32 20, i32* %y_ptr

  ; int sum = x + y;
  %x_val = load i32, i32* %x_ptr
  %y_val = load i32, i32* %y_ptr
  %sum_val = add i32 %x_val, %y_val
  %sum_ptr = alloca i32
  store i32 %sum_val, i32* %sum_ptr

  ; print_int(sum);
  call void @print_int(i32 %sum_val)
  ret void
}

; Main compiler function
define i32 @compile(i8* %source) {
  call void @init_parser(i8* %source)
  %result = call i32 @parse_expr(i8* %source)
  call void @print_int(i32 %result)
  ret i32 %result
}