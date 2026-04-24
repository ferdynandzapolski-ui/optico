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

        if self.current_token == Token::Extern {
            self.advance();
            match &self.current_token {
                Token::StringLit(n) => {
                    if n != "C" {
                        panic!("Expected \"C\" after extern, found {}", n);
                    }
                }
                Token::Ident(n) => {
                    if n != "C" {
                        panic!("Expected \"C\" after extern, found {}", n);
                    }
                }
                _ => panic!("Expected \"C\" after extern, found {:?}", self.current_token),
            }
            self.advance();
            self.expect(Token::LBrace);
            let mut decls = Vec::new();
            while self.current_token != Token::RBrace {
                decls.push(self.parse_decl());
                if self.current_token == Token::Semi {
                    self.advance();
                }
            }
            self.expect(Token::RBrace);
            return Decl::ExternC(decls);
        }

        let ty = self.parse_type();
        let name = match &self.current_token {
            Token::Ident(n) => n.clone(),
            Token::Free => "free".to_string(),
            Token::Alloc => "alloc".to_string(),
            _ => panic!("Expected identifier after type, found {:?}", self.current_token),
        };
        self.advance();

        if matches!(&self.current_token, Token::LParen | Token::LBrace) {
            // Function declaration
            self.advance();
            let mut params = Vec::new();
            while !matches!(&self.current_token, Token::RParen | Token::RBrace) {
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
            if matches!(&self.current_token, Token::RParen | Token::RBrace) {
                self.advance();
            } else {
                panic!("Expected RParen or RBrace, found {:?}", self.current_token);
            }
            let body = if self.current_token == Token::Semi {
                Expr::Block(vec![])
            } else {
                self.parse_expr()
            };
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
        let mut ty = self.parse_base_type();

        while self.current_token == Token::Star {
            self.advance();
            ty = Type::Optic(Box::new(ty), None, None, None);
        }

        if self.current_token == Token::At {
            self.advance();
            match &self.current_token {
                Token::IntLit(_) | Token::FloatLit(_) => {
                    let conf = match &self.current_token {
                        Token::IntLit(n) => *n as f64,
                        Token::FloatLit(f) => *f,
                        _ => unreachable!(),
                    };
                    self.advance();
                    ty = match ty {
                        Type::Optic(inner, res, _, perf) => Type::Optic(inner, res, Some(conf), perf),
                        Type::Traversal(inner, res, _, perf) => Type::Traversal(inner, res, Some(conf), perf),
                        Type::RecOptic(inner, res, _, perf) => Type::RecOptic(inner, res, Some(conf), perf),
                        Type::AtomicOptic(inner, res, _, perf) => Type::AtomicOptic(inner, res, Some(conf), perf),
                        Type::Pointer(inner, ctx, _) => Type::Pointer(inner, ctx, Some(conf)),
                        _ => panic!("Confidence annotation @ only allowed on optic or pointer types"),
                    };
                }
                _ => panic!("Expected confidence value after @"),
            }
        }

        if self.current_token == Token::PerfGrade {
            self.advance();
            self.expect(Token::LBrace);
            let mut latency = 0;
            let mut cache = 0;
            let mut bandwidth = 0;
            while self.current_token != Token::RBrace {
                let key = match &self.current_token {
                    Token::Ident(n) => n.clone(),
                    _ => panic!("Expected key in PerfGrade"),
                };
                self.advance();
                self.expect(Token::Colon);
                let val = match &self.current_token {
                    Token::IntLit(n) => *n as u32,
                    _ => panic!("Expected integer value in PerfGrade"),
                };
                self.advance();
                match key.as_str() {
                    "latency" => latency = val,
                    "cache" => cache = val,
                    "bandwidth" => bandwidth = val,
                    _ => panic!("Unknown key in PerfGrade: {}", key),
                }
                if self.current_token == Token::Comma {
                    self.advance();
                }
            }
            self.expect(Token::RBrace);
            let perf = Some(PerfGrade { latency_us: latency, cache_lines: cache, bandwidth_gbps: bandwidth });
            ty = match ty {
                Type::Optic(inner, res, conf, _) => Type::Optic(inner, res, conf, perf),
                Type::Traversal(inner, res, conf, _) => Type::Traversal(inner, res, conf, perf),
                Type::RecOptic(inner, res, conf, _) => Type::RecOptic(inner, res, conf, perf),
                Type::AtomicOptic(inner, res, conf, _) => Type::AtomicOptic(inner, res, conf, perf),
                Type::Named(n) => {
                    match n.as_str() {
                        "optic" => Type::Optic(Box::new(Type::Int), None, None, perf),
                        "traversal" => Type::Traversal(Box::new(Type::Int), None, None, perf),
                        "rec" => Type::RecOptic(Box::new(Type::Int), None, None, perf),
                        "atomic" => Type::AtomicOptic(Box::new(Type::Int), None, None, perf),
                        _ => panic!("PerfGrade annotation only allowed on optic types"),
                    }
                }
                _ => panic!("PerfGrade annotation only allowed on optic types, found {:?}", ty),
            };
        }

        ty
    }

    fn parse_base_type(&mut self) -> Type {
        match &self.current_token {
            Token::IntType => { self.advance(); Type::Int }
            Token::FloatType => { self.advance(); Type::Float }
            Token::BoolType => { self.advance(); Type::Bool }
            Token::CharType => { self.advance(); Type::Char }
            Token::VoidType => { self.advance(); Type::Void }
            Token::Co => {
                self.advance();
                let mut dur = None;
                let mut ctx = None;
                if let Token::Ident(n) = &self.current_token {
                    match n.as_str() {
                        "Volatile" => { dur = Some(crate::persistence::Durability::Volatile); self.advance(); }
                        "Normal" => { dur = Some(crate::persistence::Durability::Normal); self.advance(); }
                        "Durable" => { dur = Some(crate::persistence::Durability::Durable); self.advance(); }
                        _ => {}
                    }
                }
                if self.current_token == Token::Lt {
                    self.advance();
                    if let Token::Ident(n) = &self.current_token {
                        ctx = Some(n.clone());
                        self.advance();
                    }
                    self.expect(Token::Gt);
                }
                Type::Co(Box::new(self.parse_type()), dur, ctx)
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
                Type::Optic(Box::new(inner), res_assoc, None, None)
            }
            Token::Traversal => {
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
                Type::Traversal(Box::new(inner), res_assoc, None, None)
            }
            Token::Later => {
                self.advance();
                let mut clock = None;
                if self.current_token == Token::Lt {
                    self.advance();
                    if let Token::Ident(n) = &self.current_token {
                        clock = Some(n.clone());
                        self.advance();
                    }
                    self.expect(Token::Gt);
                }
                Type::Later(Box::new(self.parse_type()), clock)
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
                Type::RecOptic(Box::new(inner), res_assoc, None, None)
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
                Type::AtomicOptic(Box::new(inner), res_assoc, None, None)
            }
            Token::Pointer => {
                self.advance();
                self.expect(Token::Lt);
                let inner = self.parse_type();
                self.expect(Token::Comma);
                let ctx = match &self.current_token {
                    Token::Ident(n) => n.clone(),
                    _ => panic!("Expected context ID"),
                };
                self.advance();
                self.expect(Token::Gt);
                Type::Pointer(Box::new(inner), ctx, None)
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
            Token::Free => { self.advance(); Type::Named("free".to_string()) }
            Token::Alloc => { self.advance(); Type::Named("alloc".to_string()) }
            _ => panic!("Unexpected token in type: {:?}", self.current_token),
        }
    }

    fn parse_expr(&mut self) -> Expr {
        if self.current_token == Token::Return {
            self.advance();
            return Expr::Return(Box::new(self.parse_expr()));
        }
        if self.current_token == Token::If {
            self.advance();
            if matches!(&self.current_token, Token::LParen | Token::LBrace) {
                self.advance();
            } else {
                panic!("Expected LParen or LBrace, found {:?}", self.current_token);
            }
            let cond = self.parse_expr();
            if matches!(&self.current_token, Token::RParen | Token::RBrace) {
                self.advance();
            } else {
                panic!("Expected RParen or RBrace, found {:?}", self.current_token);
            }
            let then = self.parse_expr();
            let mut els = None;
            if self.current_token == Token::Else {
                self.advance();
                els = Some(Box::new(self.parse_expr()));
            }
            return Expr::If(Box::new(cond), Box::new(then), els);
        }
        if self.current_token == Token::While {
            self.advance();
            if matches!(&self.current_token, Token::LParen | Token::LBrace) {
                self.advance();
            } else {
                panic!("Expected LParen or LBrace, found {:?}", self.current_token);
            }
            let cond = self.parse_expr();
            if matches!(&self.current_token, Token::RParen | Token::RBrace) {
                self.advance();
            } else {
                panic!("Expected RParen or RBrace, found {:?}", self.current_token);
            }
            let body = self.parse_expr();
            return Expr::While(Box::new(cond), Box::new(body));
        }

        self.parse_put_expr()
    }

    fn parse_put_expr(&mut self) -> Expr {
        let mut e = self.parse_logical_expr();
        loop {
            match &self.current_token {
                Token::Pipe => {
                    self.advance();
                    e = Expr::Compose(Box::new(e), Box::new(self.parse_logical_expr()));
                }
                Token::Put => {
                    self.advance();
                    e = Expr::Put(Box::new(e), Box::new(self.parse_logical_expr()));
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

    fn parse_logical_expr(&mut self) -> Expr {
        let mut e = self.parse_comparison_expr();
        loop {
            let kind = match &self.current_token {
                Token::AndAnd => BinOpKind::And,
                _ => break,
            };
            self.advance();
            e = Expr::BinOp(kind, Box::new(e), Box::new(self.parse_comparison_expr()));
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
            Token::LParen => {
                self.advance();
                let e = self.parse_expr();
                self.expect(Token::RParen);
                Expr::Paren(Box::new(e))
            }
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
                let mut clock = None;
                if self.current_token == Token::Lt {
                    self.advance();
                    if let Token::Ident(n) = &self.current_token {
                        clock = Some(n.clone());
                        self.advance();
                    }
                    self.expect(Token::Gt);
                }
                Expr::Next(Box::new(self.parse_expr()), clock)
            }
            Token::Prev => {
                self.advance();
                let mut clock = None;
                if self.current_token == Token::Lt {
                    self.advance();
                    if let Token::Ident(n) = &self.current_token {
                        clock = Some(n.clone());
                        self.advance();
                    }
                    self.expect(Token::Gt);
                }
                Expr::Prev(Box::new(self.parse_expr()), clock)
            }
            Token::Alloc => {
                self.advance();
                self.expect(Token::Lt);
                let _ty = self.parse_type();
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
                self.expect(Token::Gt);
                if matches!(&self.current_token, Token::LParen | Token::LBrace) {
                    self.advance();
                } else {
                    panic!("Expected LParen or LBrace, found {:?}", self.current_token);
                }
                let mut args = Vec::new();
                while !matches!(&self.current_token, Token::RParen | Token::RBrace) {
                    args.push(self.parse_expr());
                    if self.current_token == Token::Comma {
                        self.advance();
                    }
                }
                if matches!(&self.current_token, Token::RParen | Token::RBrace) {
                    self.advance();
                } else {
                    panic!("Expected RParen or RBrace, found {:?}", self.current_token);
                }
                Expr::Alloc(_ty, args, dur)
            }
            Token::Free => {
                self.advance();
                if matches!(&self.current_token, Token::LParen | Token::LBrace) {
                    self.advance();
                } else {
                    panic!("Expected LParen or LBrace, found {:?}", self.current_token);
                }
                let e = self.parse_expr();
                if matches!(&self.current_token, Token::RParen | Token::RBrace) {
                    self.advance();
                } else {
                    panic!("Expected RParen or RBrace, found {:?}", self.current_token);
                }
                Expr::Free(Box::new(e))
            }
            Token::Checked => {
                self.advance();
                if matches!(&self.current_token, Token::LParen | Token::LBrace) {
                    self.advance();
                } else {
                    panic!("Expected LParen or LBrace, found {:?}", self.current_token);
                }
                let e = self.parse_expr();
                if matches!(&self.current_token, Token::RParen | Token::RBrace) {
                    self.advance();
                } else {
                    panic!("Expected RParen or RBrace, found {:?}", self.current_token);
                }
                Expr::Checked(Box::new(e))
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
            Token::StringLit(s) => {
                let val = s.clone();
                self.advance();
                Expr::ConstString(val)
            }
            Token::Co | Token::IntType | Token::FloatType | Token::BoolType | Token::CharType | Token::VoidType | Token::Optic | Token::Traversal | Token::Rec | Token::Atomic | Token::Struct | Token::Later | Token::Pointer => {
                let ty = self.parse_type();
                match &self.current_token {
                    Token::Ident(name) => {
                        let name = name.clone();
                        self.advance();
                        if self.current_token == Token::Assign {
                            self.advance();
                            let val = self.parse_expr();
                            Expr::LocalDecl(name, ty, Box::new(val))
                        } else {
                            Expr::Var(name)
                        }
                    }
                    _ => {
                        panic!("Expected identifier after type in expr, found {:?}", self.current_token);
                    }
                }
            }
            Token::Ident(n) => {
                let mut lex_copy = self.lexer.clone();
                let next = lex_copy.next_token();
                let is_decl = match next {
                    Token::Ident(_) | Token::Star | Token::At | Token::PerfGrade => {
                        let mut next_lex = lex_copy.clone();
                        let after_next = next_lex.next_token();
                        match after_next {
                            Token::Assign | Token::Star | Token::Ident(_) | Token::Semi | Token::At | Token::PerfGrade | Token::LParen | Token::LBrace => true,
                            _ => false,
                        }
                    }
                    _ => false,
                };

                if is_decl {
                    let ty = self.parse_type();
                    let name = match &self.current_token {
                        Token::Ident(n) => n.clone(),
                        _ => panic!("Expected identifier after type in expr, found {:?}", self.current_token),
                    };
                    self.advance();
                    if self.current_token == Token::Assign {
                        self.advance();
                        let val = self.parse_expr();
                        Expr::LocalDecl(name, ty, Box::new(val))
                    } else {
                        Expr::Var(name)
                    }
                } else {
                    let name = n.clone();
                    self.advance();
                    if self.current_token == Token::Assign {
                        self.advance();
                        Expr::Assign(name, Box::new(self.parse_expr()))
                    } else if self.current_token == Token::LParen || self.current_token == Token::LBrace || (self.current_token == Token::Lt && {
                        let mut temp_lex = self.lexer.clone();
                        matches!(temp_lex.next_token(), Token::Ident(_)) &&
                        matches!(temp_lex.next_token(), Token::Gt) &&
                        matches!(temp_lex.next_token(), Token::LParen | Token::LBrace)
                    }) {
                        // Handle function calls: name(args) or generic calls: name<Type>(args)
                        let mut type_param = None;
                        if self.current_token == Token::Lt {
                            self.advance();  // Advance past <
                            if let Token::Ident(n) = &self.current_token {
                                type_param = Some(n.clone());
                                self.advance();  // Advance past type name
                            }
                            self.expect(Token::Gt);  // Expect >
                        }
                        // Now expect ( for function arguments
                        if self.current_token == Token::LParen || self.current_token == Token::LBrace {
                            self.advance();  // Advance past ( or {
                        } else {
                            panic!("Expected LParen after function name, found {:?}", self.current_token);
                        }
                        let mut args = Vec::new();
                        while !matches!(&self.current_token, Token::RParen | Token::RBrace) {
                            args.push(self.parse_expr());
                            if self.current_token == Token::Comma {
                                self.advance();
                            }
                        }
                        if matches!(&self.current_token, Token::RParen | Token::RBrace) {
                            self.advance();
                        } else {
                            panic!("Expected RParen or RBrace, found {:?}", self.current_token);
                        }
                        // TODO: Store type_param in Expr::Call if needed
                        Expr::Call(Box::new(Expr::Var(name)), args)
                    } else {
                        Expr::Var(name)
                    }
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
                    Type::Optic(_inner, assoc, _, _) => {
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

    #[test]
    fn test_extern_c_parser() {
        let input = "extern C { void* malloc(int size); void free(void* ptr); }";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        assert_eq!(prog.decls.len(), 1);
        match &prog.decls[0] {
            Decl::ExternC(decls) => {
                assert_eq!(decls.len(), 2);
            }
            _ => panic!("Expected ExternC declaration"),
        }
    }

    #[test]
    fn test_traversal_type_parser() {
        let input = "traversal int* arr;";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        assert_eq!(prog.decls.len(), 1);
        match &prog.decls[0] {
            Decl::Global(_, ty, _) => {
                match ty {
                    Type::Traversal(inner, _, _, _) => {
                        assert_eq!(**inner, Type::Optic(Box::new(Type::Int), None, None, None));
                    }
                    _ => panic!("Expected Traversal type"),
                }
            }
            _ => panic!("Expected Global declaration"),
        }
    }
}
