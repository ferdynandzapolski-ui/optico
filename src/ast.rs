#[derive(Debug, Clone, PartialEq)]
pub enum Type {
    Int,
    Float,
    Bool,
    Char,
    Void,
    Struct(Vec<(String, Type)>),
    Named(String),             // RFC 001: Reference to a named struct
    Resource(Vec<(String, Type)>, Option<String>),
    Co(Box<Type>),             // co τ
    Optic(Box<Type>, Option<String>),          // optic τ* (with optional resource association)
    Later(Box<Type>),          // I τ
    RecOptic(Box<Type>, Option<String>),       // rec optic τ* (with optional resource association)
    AtomicOptic(Box<Type>, Option<String>),    // atomic optic τ* (with optional resource association)
    Pointer(Box<Type>, String), // pointer<τ, ContextID>
}

#[derive(Debug, Clone, PartialEq)]
pub enum Expr {
    Var(String),
    ConstInt(i64),
    ConstFloat(f64),
    ConstBool(bool),
    ConstChar(char),
    Access(Box<Expr>, String), // e.l
    Call(Box<Expr>, Vec<Expr>), // e1(e2, ...)
    Next(Box<Expr>),           // next e
    Prev(Box<Expr>),           // prev e
    Alloc(Type, Vec<Expr>),    // alloc<τ>(...)
    Free(Box<Expr>),           // free(e)
    Get(Box<Expr>),            // *e
    Put(Box<Expr>, Box<Expr>), // e1 := e2
    Compose(Box<Expr>, Box<Expr>), // e1 | e2
    ResourceDecl(String, Box<Expr>), // resource r = e (local)
    Unsafe(Box<Expr>),         // unsafe { e }
    Checked(Box<Expr>),        // checked (e)
    Spawn(Box<Expr>),          // spawn { e }
    Block(Vec<Expr>),          // { e1; e2; ... }
    Assign(String, Box<Expr>), // x = e
}

#[derive(Debug, Clone, PartialEq)]
pub enum Decl {
    Func {
        name: String,
        params: Vec<(String, Type)>,
        ret_type: Type,
        body: Expr,
    },
    Struct {                  // RFC 001: Named struct declaration
        name: String,
        fields: Vec<(String, Type)>,
    },
    Resource {                // RFC 002: Top-level resource declaration
        name: String,
        ty: Type,
        val: Expr,
    },
    Global(String, Type, Option<Expr>),
}

#[derive(Debug, Clone, PartialEq)]
pub struct Program {
    pub decls: Vec<Decl>,
}
