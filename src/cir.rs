use crate::ast::*;

#[derive(Debug, Clone, PartialEq)]
pub enum CIROp {
    Load(String),
    Store(String, Box<CIROp>),
    Get(Box<CIROp>),
    Put(Box<CIROp>, Box<CIROp>),
    Compose(Box<CIROp>, Box<CIROp>),
    Alloc(Type, Vec<CIROp>),
    Free(Box<CIROp>),
    Next(Box<CIROp>),
    Prev(Box<CIROp>),
    Call(String, Vec<CIROp>),
    Spawn(Box<CIROp>),
    Block(Vec<CIROp>),
}

pub struct CIRLowerer;

impl CIRLowerer {
    pub fn lower_program(prog: &Program) -> Vec<CIROp> {
        let mut ops = Vec::new();
        for decl in &prog.decls {
            if let Decl::Func { body, .. } = decl {
                ops.push(Self::lower_expr(body));
            }
        }
        ops
    }

    pub fn lower_expr(expr: &Expr) -> CIROp {
        match expr {
            Expr::Var(n) => CIROp::Load(n.clone()),
            Expr::Next(e) => CIROp::Next(Box::new(Self::lower_expr(e))),
            Expr::Prev(e) => CIROp::Prev(Box::new(Self::lower_expr(e))),
            Expr::Alloc(ty, args) => CIROp::Alloc(ty.clone(), args.iter().map(Self::lower_expr).collect()),
            Expr::Free(e) => CIROp::Free(Box::new(Self::lower_expr(e))),
            Expr::Get(e) => CIROp::Get(Box::new(Self::lower_expr(e))),
            Expr::Put(e1, e2) => CIROp::Put(Box::new(Self::lower_expr(e1)), Box::new(Self::lower_expr(e2))),
            Expr::Compose(e1, e2) => CIROp::Compose(Box::new(Self::lower_expr(e1)), Box::new(Self::lower_expr(e2))),
            Expr::Block(exprs) => CIROp::Block(exprs.iter().map(Self::lower_expr).collect()),
            Expr::Spawn(e) => CIROp::Spawn(Box::new(Self::lower_expr(e))),
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
        let expr = Expr::Next(Box::new(Expr::Var("x".to_string())));
        let cir = CIRLowerer::lower_expr(&expr);
        assert_eq!(cir, CIROp::Next(Box::new(CIROp::Load("x".to_string()))));
    }
}
