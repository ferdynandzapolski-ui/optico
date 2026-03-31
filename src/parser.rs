use crate::ast::*;
use crate::lexer::*;

pub struct Parser<'a> {
    lexer: Lexer<'a>,
    current_token: Token,
}

impl<'a> Parser<'a> {
    pub fn new(input: &'a str) -> Self {
        let mut lexer = Lexer::new(input);
        let current_token = lexer.next_token();
        Self { lexer, current_token }
    }

    fn advance(&mut self) {
        self.current_token = self.lexer.next_token();
    }

    fn expect(&mut self, expected: Token) {
        if std::mem::discriminant(&self.current_token) == std::mem::discriminant(&expected) {
            self.advance();
        } else {
            panic!("Expected {:?}, found {:?}", expected, self.current_token);
        }
    }

    pub fn parse_program(&mut self) -> Program {
        let mut decls = Vec::new();
        while self.current_token != Token::EOF {
            decls.push(self.parse_decl());
            if self.current_token == Token::Semi {
                self.advance();
            }
        }
        Program { decls }
    }

    fn parse_decl(&mut self) -> Decl {
        let ty = self.parse_type();
        let name = match &self.current_token {
            Token::Ident(n) => n.clone(),
            _ => panic!("Expected identifier after type, found {:?}", self.current_token),
        };
        self.advance();

        if self.current_token == Token::LParen {
            // Function declaration
            self.advance();
            let mut params = Vec::new();
            while self.current_token != Token::RParen {
                let p_ty = self.parse_type();
                let p_name = match &self.current_token {
                    Token::Ident(n) => n.clone(),
                    _ => panic!("Expected parameter name"),
                };
                self.advance();
                params.push((p_name, p_ty));
                if self.current_token == Token::Comma {
                    self.advance();
                }
            }
            self.expect(Token::RParen);
            let body = self.parse_expr();
            Decl::Func { name, params, ret_type: ty, body }
        } else {
            // Global variable
            let val = if self.current_token == Token::Assign {
                self.advance();
                Some(self.parse_expr())
            } else {
                None
            };
            Decl::Global(name, ty, val)
        }
    }

    fn parse_type(&mut self) -> Type {
        let mut ty = match &self.current_token {
            Token::IntType => { self.advance(); Type::Int }
            Token::FloatType => { self.advance(); Type::Float }
            Token::BoolType => { self.advance(); Type::Bool }
            Token::CharType => { self.advance(); Type::Char }
            Token::VoidType => { self.advance(); Type::Void }
            Token::Co => { self.advance(); Type::Co(Box::new(self.parse_type())) }
            Token::Optic => { self.advance(); Type::Optic(Box::new(self.parse_type())) }
            Token::Rec => {
                self.advance();
                self.expect(Token::Optic);
                Type::RecOptic(Box::new(self.parse_type()))
            }
            Token::Atomic => {
                self.advance();
                self.expect(Token::Optic);
                Type::AtomicOptic(Box::new(self.parse_type()))
            }
            Token::Pointer => {
                self.advance();
                self.expect(Token::LAngle);
                let inner = self.parse_type();
                self.expect(Token::Comma);
                let ctx = match &self.current_token {
                    Token::Ident(n) => n.clone(),
                    _ => panic!("Expected context ID"),
                };
                self.advance();
                self.expect(Token::RAngle);
                Type::Pointer(Box::new(inner), ctx)
            }
            Token::Ident(n) => {
                let name = n.clone();
                self.advance();
                Type::Struct(vec![(name, Type::Void)]) // Placeholder
            }
            _ => panic!("Unexpected token in type: {:?}", self.current_token),
        };

        while self.current_token == Token::Star {
            self.advance();
            ty = Type::Optic(Box::new(ty));
        }

        ty
    }

    fn parse_expr(&mut self) -> Expr {
        let mut e = self.parse_postfix_expr();
        loop {
            match &self.current_token {
                Token::Pipe => {
                    self.advance();
                    e = Expr::Compose(Box::new(e), Box::new(self.parse_expr()));
                }
                Token::Put => {
                    self.advance();
                    e = Expr::Put(Box::new(e), Box::new(self.parse_expr()));
                }
                _ => break,
            }
        }
        e
    }

    fn parse_postfix_expr(&mut self) -> Expr {
        let mut e = self.parse_primary_expr();
        loop {
            match &self.current_token {
                Token::Arrow => {
                    self.advance();
                    let field = match &self.current_token {
                        Token::Ident(n) => n.clone(),
                        _ => panic!("Expected field name after ->, found {:?}", self.current_token),
                    };
                    self.advance();
                    e = Expr::Access(Box::new(e), field);
                }
                _ => break,
            }
        }
        e
    }

    fn parse_primary_expr(&mut self) -> Expr {
        match &self.current_token {
            Token::LBrace => {
                self.advance();
                let mut exprs = Vec::new();
                while self.current_token != Token::RBrace {
                    exprs.push(self.parse_expr());
                    if self.current_token == Token::Semi {
                        self.advance();
                    }
                }
                self.expect(Token::RBrace);
                Expr::Block(exprs)
            }
            Token::Next => {
                self.advance();
                Expr::Next(Box::new(self.parse_expr()))
            }
            Token::Prev => {
                self.advance();
                Expr::Prev(Box::new(self.parse_expr()))
            }
            Token::Alloc => {
                self.advance();
                self.expect(Token::LAngle);
                let ty = self.parse_type();
                self.expect(Token::RAngle);
                self.expect(Token::LParen);
                let mut args = Vec::new();
                while self.current_token != Token::RParen {
                    args.push(self.parse_expr());
                    if self.current_token == Token::Comma {
                        self.advance();
                    }
                }
                self.expect(Token::RParen);
                Expr::Alloc(ty, args)
            }
            Token::Free => {
                self.advance();
                self.expect(Token::LParen);
                let e = self.parse_expr();
                self.expect(Token::RParen);
                Expr::Free(Box::new(e))
            }
            Token::Star => {
                self.advance();
                Expr::Get(Box::new(self.parse_expr()))
            }
            Token::ResourceKw => {
                self.advance();
                let ty = self.parse_type();
                let name = match &self.current_token {
                    Token::Ident(n) => n.clone(),
                    _ => panic!("Expected resource name"),
                };
                self.advance();
                self.expect(Token::Assign);
                let e = self.parse_expr();
                Expr::ResourceDecl(name, Box::new(e))
            }
            Token::Spawn => {
                self.advance();
                Expr::Spawn(Box::new(self.parse_expr()))
            }
            Token::IntLit(n) => {
                let val = *n;
                self.advance();
                Expr::ConstInt(val)
            }
            Token::Co | Token::IntType | Token::FloatType | Token::BoolType | Token::CharType | Token::VoidType | Token::Optic | Token::Rec | Token::Atomic => {
                let ty = self.parse_type();
                let name = match &self.current_token {
                    Token::Ident(n) => n.clone(),
                    _ => panic!("Expected identifier after type in expr, found {:?}", self.current_token),
                };
                self.advance();
                if self.current_token == Token::Assign {
                    self.advance();
                    Expr::Assign(name, Box::new(self.parse_expr()))
                } else {
                    Expr::Var(name)
                }
            }
            Token::Ident(n) => {
                let name = n.clone();
                self.advance();
                if self.current_token == Token::Assign {
                    self.advance();
                    Expr::Assign(name, Box::new(self.parse_expr()))
                } else if self.current_token == Token::LParen {
                    self.advance();
                    let mut args = Vec::new();
                    while self.current_token != Token::RParen {
                        args.push(self.parse_expr());
                        if self.current_token == Token::Comma {
                            self.advance();
                        }
                    }
                    self.expect(Token::RParen);
                    Expr::Call(Box::new(Expr::Var(name)), args)
                } else {
                    Expr::Var(name)
                }
            }
            _ => panic!("Unexpected token in expression: {:?}", self.current_token),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parser() {
        let input = "co int* counter = alloc<int>(0);";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        assert_eq!(prog.decls.len(), 1);
    }
}
