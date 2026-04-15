use crate::ast::*;

#[derive(Debug, Clone, PartialEq)]
pub enum CIROp {
    Load(String),
    Store(String, Box<CIROp>),
    Get(Box<CIROp>),
    Put(Box<CIROp>, Box<CIROp>),
    Compose(Box<CIROp>, Box<CIROp>),
    Alloc(Type, Vec<CIROp>, Option<crate::persistence::Durability>),
    Free(Box<CIROp>),
    Fetch(crate::persistence::Fingerprint),
    Persist(Box<CIROp>, Option<crate::persistence::Durability>),
    Checkpoint(String), // Session/SSFG checkpoint name
    Next(Box<CIROp>, Option<String>),
    Prev(Box<CIROp>, Option<String>),
    Call(String, Vec<CIROp>),
    Spawn(Box<CIROp>),
    Block(Vec<CIROp>),
    ConstInt(i64),
    StructDecl(String, Vec<(String, Type)>), // RFC 001
    ResourceDecl(String, Type, Box<CIROp>),  // RFC 002
    ProtocolDecl(String, Vec<ProtocolState>),
}

pub struct CIRLowerer;

impl CIRLowerer {
    pub fn lower_program(prog: &Program) -> Vec<CIROp> {
        let mut ops = Vec::new();
        for decl in &prog.decls {
            match decl {
                Decl::Func { body, .. } => {
                    ops.push(Self::lower_expr(body));
                }
                Decl::Struct { name, fields } => {
                    ops.push(CIROp::StructDecl(name.clone(), fields.clone()));
                }
                Decl::Protocol { name, states } => {
                    ops.push(CIROp::ProtocolDecl(name.clone(), states.clone()));
                }
                Decl::Resource { name, ty, val } => {
                    ops.push(CIROp::ResourceDecl(name.clone(), ty.clone(), Box::new(Self::lower_expr(val))));
                }
                Decl::Global(name, _, val) => {
                    if let Some(v) = val {
                        ops.push(CIROp::Store(name.clone(), Box::new(Self::lower_expr(v))));
                    } else {
                        // For uninitialized globals, we could emit a placeholder
                    }
                }
                Decl::ExternC(decls) => {
                    for d in decls {
                        match d {
                            Decl::Func { name, params: _, ret_type: _, body: _ } => {
                                // Extern functions might not have bodies or just mock ones
                                ops.push(CIROp::Call(name.clone(), vec![]));
                            }
                            _ => {}
                        }
                    }
                }
            }
        }
        ops
    }

    pub fn lower_expr(expr: &Expr) -> CIROp {
        match expr {
            Expr::LocalDecl(n, _, e) => CIROp::Store(n.clone(), Box::new(Self::lower_expr(e))),
            Expr::Var(n) => CIROp::Load(n.clone()),
            Expr::Next(e, clock) => CIROp::Next(Box::new(Self::lower_expr(e)), clock.clone()),
            Expr::Prev(e, clock) => CIROp::Prev(Box::new(Self::lower_expr(e)), clock.clone()),
            Expr::Alloc(ty, args, dur) => CIROp::Alloc(ty.clone(), args.iter().map(Self::lower_expr).collect(), dur.clone()),
            Expr::Free(e) => CIROp::Free(Box::new(Self::lower_expr(e))),
            Expr::Get(e) => CIROp::Get(Box::new(Self::lower_expr(e))),
            Expr::Put(e1, e2) => CIROp::Put(Box::new(Self::lower_expr(e1)), Box::new(Self::lower_expr(e2))),
            Expr::Compose(e1, e2) => CIROp::Compose(Box::new(Self::lower_expr(e1)), Box::new(Self::lower_expr(e2))),
            Expr::Block(exprs) => CIROp::Block(exprs.iter().map(Self::lower_expr).collect()),
            Expr::Spawn(e) => CIROp::Spawn(Box::new(Self::lower_expr(e))),
            Expr::ConstInt(v) => CIROp::ConstInt(*v),
            Expr::Call(e, args) => {
                if let Expr::Var(n) = &**e {
                    CIROp::Call(n.clone(), args.iter().map(Self::lower_expr).collect())
                } else {
                    CIROp::Block(vec![]) // simplified
                }
            }
            Expr::Assign(n, e) => CIROp::Store(n.clone(), Box::new(Self::lower_expr(e))),
            _ => CIROp::Block(vec![]),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cir_lower() {
        let expr = Expr::Next(Box::new(Expr::Var("x".to_string())), None);
        let cir = CIRLowerer::lower_expr(&expr);
        assert_eq!(cir, CIROp::Next(Box::new(CIROp::Load("x".to_string())), None));
    }
}
