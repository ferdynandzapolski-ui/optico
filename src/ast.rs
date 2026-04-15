#[derive(Debug, Clone, PartialEq)]
pub struct PerfGrade {
    pub latency_us: u32,
    pub cache_lines: u32,
    pub bandwidth_gbps: u32,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Type {
    Int,
    Float,
    Bool,
    Char,
    Void,
    Struct(Vec<(String, Type)>),
    Named(String),             // RFC 001: Reference to a named struct
    Resource(Vec<(String, Type)>, Option<String>, Option<String>, Option<crate::persistence::Durability>), // fields, state, protocol, durability
    ProtocolOptic(String, String, Box<Type>), // protocol, state, inner
    Co(Box<Type>, Option<crate::persistence::Durability>, Option<String>), // co Δ τ (with optional ContextID)
    Optic(Box<Type>, Option<String>, Option<f64>, Option<PerfGrade>),          // optic τ* (with optional resource association, confidence, and perf grade)
    Traversal(Box<Type>, Option<String>, Option<f64>, Option<PerfGrade>),      // traversal τ*
    Later(Box<Type>, Option<String>),          // I<k> τ
    RecOptic(Box<Type>, Option<String>, Option<f64>, Option<PerfGrade>),       // rec optic τ* (with optional resource association, confidence, and perf grade)
    AtomicOptic(Box<Type>, Option<String>, Option<f64>, Option<PerfGrade>),    // atomic optic τ* (with optional resource association, confidence, and perf grade)
    Pointer(Box<Type>, String, Option<f64>), // pointer<τ, ContextID, confidence>
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum BinOpKind {
    Add, Sub, Mul, Div,
    Eq, Ne, Lt, Gt, Le, Ge,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Expr {
    Var(String),
    ConstInt(i64),
    ConstFloat(f64),
    ConstBool(bool),
    ConstChar(char),
    ConstString(String),
    Access(Box<Expr>, String), // e.l
    Call(Box<Expr>, Vec<Expr>), // e1(e2, ...)
    If(Box<Expr>, Box<Expr>, Option<Box<Expr>>), // if (e1) e2 [else e3]
    BinOp(BinOpKind, Box<Expr>, Box<Expr>),
    Index(Box<Expr>, Box<Expr>), // e1[e2]
    Next(Box<Expr>, Option<String>),           // next<k> e
    Prev(Box<Expr>, Option<String>),           // prev<k> e
    Alloc(Type, Vec<Expr>, Option<crate::persistence::Durability>),    // alloc<τ>(...)
    Free(Box<Expr>),           // free(e)
    Get(Box<Expr>),            // *e
    Put(Box<Expr>, Box<Expr>), // e1 := e2
    Compose(Box<Expr>, Box<Expr>), // e1 | e2
    ResourceDecl(String, Box<Expr>), // resource r = e (local)
    Unsafe(Box<Expr>),         // unsafe { e }
    Checked(Box<Expr>),        // checked (e)
    Spawn(Box<Expr>),          // spawn { e }
    Block(Vec<Expr>),          // { e1; e2; ... }
    LocalDecl(String, Type, Box<Expr>), // τ x = e
    Assign(String, Box<Expr>), // x = e
    Return(Box<Expr>),
}

#[derive(Debug, Clone, PartialEq)]
pub struct ProtocolState {
    pub name: String,
    pub optics: Vec<(String, Type)>,
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
    Protocol {
        name: String,
        states: Vec<ProtocolState>,
    },
    Global(String, Type, Option<Expr>),
    ExternC(Vec<Decl>),
}

#[derive(Debug, Clone, PartialEq)]
pub struct Program {
    pub decls: Vec<Decl>,
}
