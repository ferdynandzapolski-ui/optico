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
        if self.current_token == Token::Struct {
            self.advance();
            let name = match &self.current_token {
                Token::Ident(n) => n.clone(),
                _ => panic!("Expected struct name, found {:?}", self.current_token),
            };
            self.advance();
            self.expect(Token::LBrace);
            let mut fields = Vec::new();
            while self.current_token != Token::RBrace {
                let f_ty = self.parse_type();
                let f_name = match &self.current_token {
                    Token::Ident(n) => n.clone(),
                    Token::Next => "next".to_string(), // Allow 'next' as field name
                    _ => panic!("Expected field name, found {:?}", self.current_token),
                };
                self.advance();
                self.expect(Token::Semi);
                fields.push((f_name, f_ty));
            }
            self.expect(Token::RBrace);
            return Decl::Struct { name, fields };
        }

        if self.current_token == Token::ResourceKw {
            self.advance();
            let ty = self.parse_type();
            let name = match &self.current_token {
                Token::Ident(n) => n.clone(),
                _ => panic!("Expected resource name"),
            };
            self.advance();
            self.expect(Token::Assign);
            let val = self.parse_expr();
            return Decl::Resource { name, ty, val };
        }

        if self.current_token == Token::Protocol {
            return self.parse_protocol_decl();
        }

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

    fn parse_protocol_decl(&mut self) -> Decl {
        self.expect(Token::Protocol);
        let name = match &self.current_token {
            Token::Ident(n) => n.clone(),
            _ => panic!("Expected protocol name"),
        };
        self.advance();
        self.expect(Token::LBrace);
        let mut states = Vec::new();
        while self.current_token == Token::State {
            self.advance();
            let s_name = match &self.current_token {
                Token::Ident(n) => n.clone(),
                _ => panic!("Expected state name"),
            };
            self.advance();
            self.expect(Token::LBrace);
            let mut optics = Vec::new();
            while self.current_token != Token::RBrace {
                let o_ty = self.parse_type();
                let o_name = match &self.current_token {
                    Token::Ident(n) => n.clone(),
                    _ => panic!("Expected optic name in state"),
                };
                self.advance();
                self.expect(Token::Semi);
                optics.push((o_name, o_ty));
            }
            self.expect(Token::RBrace);
            states.push(ProtocolState { name: s_name, optics });
        }
        self.expect(Token::RBrace);
        Decl::Protocol { name, states }
    }

    fn parse_type(&mut self) -> Type {
        let mut ty = match &self.current_token {
            Token::IntType => { self.advance(); Type::Int }
            Token::FloatType => { self.advance(); Type::Float }
            Token::BoolType => { self.advance(); Type::Bool }
            Token::CharType => { self.advance(); Type::Char }
            Token::VoidType => { self.advance(); Type::Void }
            Token::Co => {
                self.advance();
                let mut dur = None;
                if let Token::Ident(n) = &self.current_token {
                    match n.as_str() {
                        "Volatile" => { dur = Some(crate::persistence::Durability::Volatile); self.advance(); }
                        "Normal" => { dur = Some(crate::persistence::Durability::Normal); self.advance(); }
                        "Durable" => { dur = Some(crate::persistence::Durability::Durable); self.advance(); }
                        _ => {}
                    }
                }
                Type::Co(Box::new(self.parse_type()), dur)
            }
            Token::Optic => {
                self.advance();
                let inner = self.parse_type();
                let mut res_assoc = None;
                if self.current_token == Token::Bang {
                    self.advance();
                    res_assoc = match &self.current_token {
                        Token::Ident(n) => Some(n.clone()),
                        _ => panic!("Expected resource name after !"),
                    };
                    self.advance();
                }
                Type::Optic(Box::new(inner), res_assoc)
            }
            Token::Rec => {
                self.advance();
                self.expect(Token::Optic);
                let inner = self.parse_type();
                let mut res_assoc = None;
                if self.current_token == Token::Bang {
                    self.advance();
                    res_assoc = match &self.current_token {
                        Token::Ident(n) => Some(n.clone()),
                        _ => panic!("Expected resource name after !"),
                    };
                    self.advance();
                }
                Type::RecOptic(Box::new(inner), res_assoc)
            }
            Token::Atomic => {
                self.advance();
                self.expect(Token::Optic);
                let inner = self.parse_type();
                let mut res_assoc = None;
                if self.current_token == Token::Bang {
                    self.advance();
                    res_assoc = match &self.current_token {
                        Token::Ident(n) => Some(n.clone()),
                        _ => panic!("Expected resource name after !"),
                    };
                    self.advance();
                }
                Type::AtomicOptic(Box::new(inner), res_assoc)
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
                let mut dur = None;
                let mut name = n.clone();
                match name.as_str() {
                    "Volatile" | "Normal" | "Durable" => {
                        let potential_dur = match name.as_str() {
                            "Volatile" => crate::persistence::Durability::Volatile,
                            "Normal" => crate::persistence::Durability::Normal,
                            "Durable" => crate::persistence::Durability::Durable,
                            _ => unreachable!(),
                        };

                        // We need to look ahead to see if it's a prefix or a type name
                        // Lexer doesn't have lookahead beyond 1 char, but Parser has current_token.
                        // If we advance, we must be sure it's a prefix.
                        // A durability prefix is always followed by another Ident (the type name)
                        // OR a Protocol name which is also an Ident.

                        let mut lex_copy = self.lexer.clone();
                        let next_token = lex_copy.next_token();
                        if let Token::Ident(_) = next_token {
                             dur = Some(potential_dur);
                             self.advance();
                             name = match &self.current_token {
                                 Token::Ident(n) => n.clone(),
                                 _ => unreachable!(),
                             };
                        }
                    }
                    _ => {}
                }

                self.advance();
                if self.current_token == Token::LBracket {
                    self.advance();
                    let state = match &self.current_token {
                        Token::Ident(s) => s.clone(),
                        _ => panic!("Expected state name in []"),
                    };
                    self.advance();
                    self.expect(Token::RBracket);
                    Type::Resource(vec![], Some(state), Some(name), dur)
                } else {
                    if dur.is_some() {
                        panic!("Durability only allowed for co types and resources");
                    }
                    Type::Named(name)
                }
            }
            _ => panic!("Unexpected token in type: {:?}", self.current_token),
        };

        while self.current_token == Token::Star {
            self.advance();
            ty = Type::Optic(Box::new(ty), None);
        }

        ty
    }

    fn parse_expr(&mut self) -> Expr {
        if self.current_token == Token::If {
            self.advance();
            self.expect(Token::LParen);
            let cond = self.parse_expr();
            self.expect(Token::RParen);
            let then = self.parse_expr();
            let mut els = None;
            if self.current_token == Token::Else {
                self.advance();
                els = Some(Box::new(self.parse_expr()));
            }
            return Expr::If(Box::new(cond), Box::new(then), els);
        }

        self.parse_put_expr()
    }

    fn parse_put_expr(&mut self) -> Expr {
        let mut e = self.parse_comparison_expr();
        loop {
            match &self.current_token {
                Token::Pipe => {
                    self.advance();
                    e = Expr::Compose(Box::new(e), Box::new(self.parse_comparison_expr()));
                }
                Token::Put => {
                    self.advance();
                    e = Expr::Put(Box::new(e), Box::new(self.parse_comparison_expr()));
                }
                _ => break,
            }
        }
        e
    }

    fn parse_comparison_expr(&mut self) -> Expr {
        let mut e = self.parse_additive_expr();
        loop {
            let kind = match &self.current_token {
                Token::Eq => BinOpKind::Eq,
                Token::Ne => BinOpKind::Ne,
                Token::Lt => BinOpKind::Lt,
                Token::Gt => BinOpKind::Gt,
                Token::Le => BinOpKind::Le,
                Token::Ge => BinOpKind::Ge,
                _ => break,
            };
            self.advance();
            e = Expr::BinOp(kind, Box::new(e), Box::new(self.parse_additive_expr()));
        }
        e
    }

    fn parse_additive_expr(&mut self) -> Expr {
        let mut e = self.parse_multiplicative_expr();
        loop {
            let kind = match &self.current_token {
                Token::Plus => BinOpKind::Add,
                Token::Minus => BinOpKind::Sub,
                _ => break,
            };
            self.advance();
            e = Expr::BinOp(kind, Box::new(e), Box::new(self.parse_multiplicative_expr()));
        }
        e
    }

    fn parse_multiplicative_expr(&mut self) -> Expr {
        let mut e = self.parse_postfix_expr();
        loop {
            let kind = match &self.current_token {
                Token::Star => BinOpKind::Mul,
                Token::Slash => BinOpKind::Div,
                _ => break,
            };
            self.advance();
            e = Expr::BinOp(kind, Box::new(e), Box::new(self.parse_postfix_expr()));
        }
        e
    }

    fn parse_postfix_expr(&mut self) -> Expr {
        let mut e = self.parse_primary_expr();
        loop {
            match &self.current_token {
                Token::Arrow | Token::Dot => {
                    self.advance();
                    let field = match &self.current_token {
                        Token::Ident(n) => n.clone(),
                        Token::Next => "next".to_string(), // Allow 'next' as field name
                        _ => panic!("Expected field name after postfix operator, found {:?}", self.current_token),
                    };
                    self.advance();
                    e = Expr::Access(Box::new(e), field);
                }
                Token::LBracket => {
                    self.advance();
                    let index = self.parse_expr();
                    self.expect(Token::RBracket);
                    e = Expr::Index(Box::new(e), Box::new(index));
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
                let mut _ty = self.parse_type();
                while self.current_token == Token::Star {
                    self.advance();
                    _ty = Type::Optic(Box::new(_ty), None);
                }
                let mut dur = None;
                if self.current_token == Token::Comma {
                    self.advance();
                    match &self.current_token {
                        Token::Ident(n) => {
                            match n.as_str() {
                                "Volatile" => dur = Some(crate::persistence::Durability::Volatile),
                                "Normal" => dur = Some(crate::persistence::Durability::Normal),
                                "Durable" => dur = Some(crate::persistence::Durability::Durable),
                                _ => panic!("Unknown durability level: {}", n),
                            }
                        }
                        _ => panic!("Expected durability level after comma in alloc"),
                    }
                    self.advance();
                }
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
                Expr::Alloc(_ty, args, dur)
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
                let _ty = self.parse_type();
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
            Token::BoolLit(b) => {
                let val = *b;
                self.advance();
                Expr::ConstBool(val)
            }
            Token::IntLit(n) => {
                let val = *n;
                self.advance();
                Expr::ConstInt(val)
            }
            Token::CharLit(c) => {
                let val = *c;
                self.advance();
                Expr::ConstChar(val)
            }
            Token::Co | Token::IntType | Token::FloatType | Token::BoolType | Token::CharType | Token::VoidType | Token::Optic | Token::Rec | Token::Atomic | Token::Struct => {
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

    #[test]
    fn test_struct_parser() {
        let input = "struct Point { int x; int y; }";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        assert_eq!(prog.decls.len(), 1);
        match &prog.decls[0] {
            Decl::Struct { name, fields } => {
                assert_eq!(name, "Point");
                assert_eq!(fields.len(), 2);
                assert_eq!(fields[0].0, "x");
                assert_eq!(fields[0].1, Type::Int);
                assert_eq!(fields[1].0, "y");
                assert_eq!(fields[1].1, Type::Int);
            }
            _ => panic!("Expected Struct declaration"),
        }
    }

    #[test]
    fn test_top_level_resource_parser() {
        let input = "resource Console[Open] console = alloc<Console>(1);";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        assert_eq!(prog.decls.len(), 1);
        match &prog.decls[0] {
            Decl::Resource { name, ty, .. } => {
                assert_eq!(name, "console");
                assert_eq!(*ty, Type::Resource(vec![], Some("Open".to_string()), Some("Console".to_string()), None));
            }
            _ => panic!("Expected Resource declaration"),
        }
    }

    #[test]
    fn test_optic_association_parser() {
        let input = "optic char*!f buffer;";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        assert_eq!(prog.decls.len(), 1);
        match &prog.decls[0] {
            Decl::Global(name, ty, _) => {
                assert_eq!(name, "buffer");
                match ty {
                    Type::Optic(inner, assoc) => {
                        assert_eq!(assoc, &Some("f".to_string()));
                    }
                    _ => panic!("Expected Optic type"),
                }
            }
            _ => panic!("Expected Global declaration"),
        }
    }

    #[test]
    fn test_parser_new_exprs() {
        let input = "void main() { if (x == y) { x = 1; } else { x = 2; } a = b + c * d; e = f[g]; }";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        assert_eq!(prog.decls.len(), 1);
    }
}
