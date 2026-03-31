#[derive(Debug, Clone, PartialEq)]
pub enum Type {
    Int,
    Float,
    Bool,
    Char,
    Void,
    Struct(Vec<(String, Type)>),
    Resource(Vec<(String, Type)>),
    Co(Box<Type>),             // co τ
    Optic(Box<Type>),          // optic τ*
    Later(Box<Type>),          // I τ
    RecOptic(Box<Type>),       // rec optic τ*
    AtomicOptic(Box<Type>),    // atomic optic τ*
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
    ResourceDecl(String, Box<Expr>), // resource r = e
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
    Global(String, Type, Option<Expr>),
}

#[derive(Debug, Clone, PartialEq)]
pub struct Program {
    pub decls: Vec<Decl>,
}
