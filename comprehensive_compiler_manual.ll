; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; External declarations
declare i32 @malloc(i32)
declare void @free(i32)

; Global state
@input_pos = global i32 0
@current_token = global i32 0
@token_value = global i32 0

; Initialize lexer
define void @lexer_init(i8* %input) {
  store i32 0, i32* @input_pos
  store i32 0, i32* @current_token
  store i32 0, i32* @token_value
  ret void
}

; Get next character
define i8 @get_char(i8* %input) {
  %pos = load i32, i32* @input_pos
  %char_ptr = getelementptr i8, i8* %input, i32 %pos
  %char = load i8, i8* %char_ptr
  ret i8 %char
}

; Advance position
define void @advance() {
  %pos = load i32, i32* @input_pos
  %new_pos = add i32 %pos, 1
  store i32 %new_pos, i32* @input_pos
  ret void
}

; Skip whitespace
define void @skip_whitespace(i8* %input) {
  br label %loop

loop:
  %char = call i8 @get_char(i8* %input)
  %is_space = icmp eq i8 %char, 32  ; space
  %is_tab = icmp eq i8 %char, 9     ; tab
  %is_newline = icmp eq i8 %char, 10 ; newline
  %is_whitespace = or i1 %is_space, %is_tab
  %is_whitespace2 = or i1 %is_whitespace, %is_newline
  br i1 %is_whitespace2, label %skip, label %end

skip:
  call void @advance()
  br label %loop

end:
  ret void
}

; Read number
define void @read_number(i8* %input) {
  store i32 0, i32* @token_value
  br label %loop

loop:
  %char = call i8 @get_char(i8* %input)
  %is_digit = icmp uge i8 %char, 48  ; >= '0'
  %is_digit2 = icmp ule i8 %char, 57 ; <= '9'
  %valid = and i1 %is_digit, %is_digit2
  br i1 %valid, label %digit, label %end

digit:
  %val = load i32, i32* @token_value
  %digit_val = sub i8 %char, 48
  %digit_ext = sext i8 %digit_val to i32
  %new_val = mul i32 %val, 10
  %final_val = add i32 %new_val, %digit_ext
  store i32 %final_val, i32* @token_value
  call void @advance()
  br label %loop

end:
  ret void
}

; Lex next token (simplified)
define void @lex_next(i8* %input) {
  call void @skip_whitespace(i8* %input)

  %char = call i8 @get_char(i8* %input)
  %is_eof = icmp eq i8 %char, 0
  br i1 %is_eof, label %eof, label %not_eof

eof:
  store i32 0, i32* @current_token  ; TOK_EOF
  br label %end

not_eof:
  %is_digit = icmp uge i8 %char, 48
  %is_digit2 = icmp ule i8 %char, 57
  %is_number = and i1 %is_digit, %is_digit2
  br i1 %is_number, label %number, label %symbol

number:
  call void @read_number(i8* %input)
  store i32 1, i32* @current_token  ; TOK_INT
  br label %end

symbol:
  ; Simple symbol recognition
  %is_plus = icmp eq i8 %char, 43
  br i1 %is_plus, label %plus, label %other

plus:
  store i32 3, i32* @current_token  ; TOK_PLUS
  call void @advance()
  br label %end

other:
  store i32 0, i32* @current_token  ; Default
  call void @advance()
  br label %end

end:
  ret void
}

; Parse primary expression
define i32 @parse_primary(i8* %input) {
  %tok = load i32, i32* @current_token
  %is_int = icmp eq i32 %tok, 1  ; TOK_INT
  br i1 %is_int, label %int_lit, label %paren

int_lit:
  %val = load i32, i32* @token_value
  call void @lex_next(i8* %input)
  ret i32 %val

paren:
  %is_lparen = icmp eq i32 %tok, 7  ; TOK_LPAREN
  br i1 %is_lparen, label %parse_expr, label %error

parse_expr:
  call void @lex_next(i8* %input)
  %expr_val = call i32 @parse_expr(i8* %input)
  %tok2 = load i32, i32* @current_token
  %is_rparen = icmp eq i32 %tok2, 8  ; TOK_RPAREN
  br i1 %is_rparen, label %close_paren, label %error

close_paren:
  call void @lex_next(i8* %input)
  ret i32 %expr_val

error:
  ret i32 0
}

; Parse multiplication/division
define i32 @parse_mul_div(i8* %input) {
entry:
  %left_init = call i32 @parse_primary(i8* %input)

  br label %loop

loop:
  %left = phi i32 [ %left_init, %entry ], [ %new_left, %mul ], [ %new_left2, %div ]
  %tok = load i32, i32* @current_token
  %is_star = icmp eq i32 %tok, 5   ; TOK_STAR
  br i1 %is_star, label %star, label %check_slash

check_slash:
  %is_slash = icmp eq i32 %tok, 6  ; TOK_SLASH
  br i1 %is_slash, label %slash, label %end

star:
  br label %op

slash:
  br label %op

op:
  %op_type = phi i32 [5, %star], [6, %slash]
  call void @lex_next(i8* %input)
  %right = call i32 @parse_primary(i8* %input)

  %is_mul = icmp eq i32 %op_type, 5
  br i1 %is_mul, label %mul, label %div

mul:
  %new_left = mul i32 %left, %right
  br label %loop

div:
  %new_left2 = sdiv i32 %left, %right
  br label %loop

end:
  ret i32 %left
}

; Parse addition/subtraction
define i32 @parse_add_sub(i8* %input) {
entry:
  %left_init = call i32 @parse_mul_div(i8* %input)

  br label %loop

loop:
  %left = phi i32 [ %left_init, %entry ], [ %new_left, %add ], [ %new_left2, %sub ]
  %tok = load i32, i32* @current_token
  %is_plus = icmp eq i32 %tok, 3   ; TOK_PLUS
  br i1 %is_plus, label %plus, label %check_minus

check_minus:
  %is_minus = icmp eq i32 %tok, 4  ; TOK_MINUS
  br i1 %is_minus, label %minus, label %end

plus:
  br label %op

minus:
  br label %op

op:
  %op_type = phi i32 [3, %plus], [4, %minus]
  call void @lex_next(i8* %input)
  %right = call i32 @parse_mul_div(i8* %input)

  %is_add = icmp eq i32 %op_type, 3
  br i1 %is_add, label %add, label %sub

add:
  %new_left = add i32 %left, %right
  br label %loop

sub:
  %new_left2 = sub i32 %left, %right
  br label %loop

end:
  ret i32 %left
}

; Parse expression
define i32 @parse_expr(i8* %input) {
  %result = call i32 @parse_add_sub(i8* %input)
  ret i32 %result
}

; Main compiler function
define i32 @compile(i8* %source) {
  call void @lexer_init(i8* %source)
  call void @lex_next(i8* %source)

  %result = call i32 @parse_expr(i8* %source)
  ret i32 %result
}