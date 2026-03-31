#[derive(Debug, Clone, PartialEq)]
pub enum Token {
    IntType, FloatType, BoolType, CharType, VoidType,
    Struct, Resource, Co, Optic, Rec, Atomic, Pointer,
    Next, Prev, Alloc, Free, Unsafe, Checked, Spawn,
    ResourceKw,
    Ident(String),
    IntLit(i64),
    FloatLit(f64),
    BoolLit(bool),
    CharLit(char),
    LBrace, RBrace, LParen, RParen, LBracket, RBracket, LAngle, RAngle,
    Dot, Comma, Semi, Colon, Star, Assign, Put, Arrow, Pipe,
    Bang,
    EOF,
}

pub struct Lexer<'a> {
    input: &'a str,
    pos: usize,
}

impl<'a> Lexer<'a> {
    pub fn new(input: &'a str) -> Self {
        Self { input, pos: 0 }
    }

    fn peek(&self) -> Option<char> {
        self.input[self.pos..].chars().next()
    }

    fn advance(&mut self) -> Option<char> {
        let c = self.peek();
        if let Some(c) = c {
            self.pos += c.len_utf8();
        }
        c
    }

    fn skip_whitespace(&mut self) {
        while let Some(c) = self.peek() {
            if c.is_whitespace() {
                self.advance();
            } else if c == '/' && self.input[self.pos..].starts_with("//") {
                while let Some(c) = self.advance() {
                    if c == '\n' { break; }
                }
            } else {
                break;
            }
        }
    }

    pub fn next_token(&mut self) -> Token {
        self.skip_whitespace();
        let c = match self.advance() {
            Some(c) => c,
            None => return Token::EOF,
        };

        match c {
            '{' => Token::LBrace,
            '}' => Token::RBrace,
            '(' => Token::LParen,
            ')' => Token::RParen,
            '[' => Token::LBracket,
            ']' => Token::RBracket,
            '<' => Token::LAngle,
            '>' => Token::RAngle,
            '.' => Token::Dot,
            ',' => Token::Comma,
            ';' => Token::Semi,
            ':' => {
                if self.peek() == Some('=') {
                    self.advance();
                    Token::Put
                } else {
                    Token::Colon
                }
            }
            '*' => Token::Star,
            '=' => Token::Assign,
            '|' => Token::Pipe,
            '!' => Token::Bang,
            '-' if self.peek() == Some('>') => {
                self.advance();
                Token::Arrow
            }
            '\'' => {
                let val = self.advance().unwrap();
                self.advance(); // skip '
                Token::CharLit(val)
            }
            _ if c.is_ascii_digit() => {
                let mut s = c.to_string();
                while let Some(c) = self.peek() {
                    if c.is_ascii_digit() || c == '.' {
                        s.push(self.advance().unwrap());
                    } else {
                        break;
                    }
                }
                if s.contains('.') {
                    Token::FloatLit(s.parse().unwrap())
                } else {
                    Token::IntLit(s.parse().unwrap())
                }
            }
            _ if c.is_alphabetic() || c == '_' => {
                let mut s = c.to_string();
                while let Some(c) = self.peek() {
                    if c.is_alphanumeric() || c == '_' {
                        s.push(self.advance().unwrap());
                    } else {
                        break;
                    }
                }
                match s.as_str() {
                    "Int" | "int" => Token::IntType,
                    "Float" | "float" => Token::FloatType,
                    "Bool" | "bool" => Token::BoolType,
                    "Char" | "char" => Token::CharType,
                    "Void" | "void" => Token::VoidType,
                    "Struct" | "struct" => Token::Struct,
                    "Resource" | "resource" => Token::ResourceKw,
                    "co" => Token::Co,
                    "optic" => Token::Optic,
                    "rec" => Token::Rec,
                    "atomic" => Token::Atomic,
                    "pointer" => Token::Pointer,
                    "next" => Token::Next,
                    "prev" => Token::Prev,
                    "alloc" => Token::Alloc,
                    "free" => Token::Free,
                    "unsafe" => Token::Unsafe,
                    "checked" => Token::Checked,
                    "spawn" => Token::Spawn,
                    "true" => Token::BoolLit(true),
                    "false" => Token::BoolLit(false),
                    _ => Token::Ident(s),
                }
            }
            _ => panic!("Unexpected character: {}", c),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_lexer() {
        let mut lexer = Lexer::new("co int* counter = alloc<int>(0);");
        assert_eq!(lexer.next_token(), Token::Co);
        assert_eq!(lexer.next_token(), Token::IntType);
        assert_eq!(lexer.next_token(), Token::Star);
        assert_eq!(lexer.next_token(), Token::Ident("counter".to_string()));
        assert_eq!(lexer.next_token(), Token::Assign);
        assert_eq!(lexer.next_token(), Token::Alloc);
        assert_eq!(lexer.next_token(), Token::LAngle);
        assert_eq!(lexer.next_token(), Token::IntType);
        assert_eq!(lexer.next_token(), Token::RAngle);
        assert_eq!(lexer.next_token(), Token::LParen);
        assert_eq!(lexer.next_token(), Token::IntLit(0));
        assert_eq!(lexer.next_token(), Token::RParen);
        assert_eq!(lexer.next_token(), Token::Semi);
    }
}
