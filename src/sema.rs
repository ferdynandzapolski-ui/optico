use crate::ast::*;
use std::collections::{HashMap, HashSet};

pub struct Sema {
    pub globals: HashMap<String, Type>,
    pub structs: HashMap<String, Vec<(String, Type)>>, // RFC 001: Struct definitions
    pub resources: HashSet<String>,
    pub in_rec_optic: bool,
    pub guarded: bool,
}

impl Sema {
    pub fn new() -> Self {
        Self {
            globals: HashMap::new(),
            structs: HashMap::new(),
            resources: HashSet::new(),
            in_rec_optic: false,
            guarded: false,
        }
    }

    pub fn check_program(&mut self, prog: &Program) {
        for decl in &prog.decls {
            match decl {
                Decl::Global(name, ty, _) => {
                    self.globals.insert(name.clone(), ty.clone());
                }
                Decl::Func { name, ret_type, .. } => {
                    self.globals.insert(name.clone(), ret_type.clone());
                }
                Decl::Struct { name, fields } => {
                    self.structs.insert(name.clone(), fields.clone());
                }
                Decl::Resource { name, ty, .. } => {
                    self.globals.insert(name.clone(), ty.clone());
                    self.resources.insert(name.clone());
                }
            }
        }

        for decl in &prog.decls {
            match decl {
                Decl::Func { body, params, ret_type, .. } => {
                    let mut env = self.globals.clone();
                    for (p_name, p_ty) in params {
                        env.insert(p_name.clone(), p_ty.clone());
                    }

                    let old_in_rec = self.in_rec_optic;
                    self.in_rec_optic = matches!(ret_type, Type::RecOptic(_));
                    self.guarded = false;

                    let mut res_consumed = HashSet::new();
                    let mut local_resources = HashSet::new();

                    self.check_expr(body, &mut env, &mut res_consumed, &mut local_resources);

                    self.in_rec_optic = old_in_rec;
                }
                Decl::Resource { val, .. } => {
                    let mut env = self.globals.clone();
                    let mut res_consumed = HashSet::new();
                    let mut local_resources = HashSet::new();
                    self.check_expr(val, &mut env, &mut res_consumed, &mut local_resources);
                }
                _ => {}
            }
        }
    }

    pub fn check_expr(&mut self, expr: &Expr, env: &mut HashMap<String, Type>, res_consumed: &mut HashSet<String>, local_resources: &mut HashSet<String>) -> Type {
        match expr {
            Expr::Var(n) => {
                if local_resources.contains(n) || self.resources.contains(n) {
                    if res_consumed.contains(n) {
                        panic!("Linearity violation: resource {} used after consume", n);
                    }
                    res_consumed.insert(n.clone());
                }
                env.get(n).cloned().expect(&format!("Undefined variable {}", n))
            }
            Expr::ConstInt(_) => Type::Int,
            Expr::ConstFloat(_) => Type::Float,
            Expr::ConstBool(_) => Type::Bool,
            Expr::ConstChar(_) => Type::Char,
            Expr::Next(e) => {
                let old_guarded = self.guarded;
                self.guarded = true;
                let ty = Type::Later(Box::new(self.check_expr(e, env, res_consumed, local_resources)));
                self.guarded = old_guarded;
                ty
            }
            Expr::Prev(e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                match ty {
                    Type::Later(inner) => *inner,
                    _ => panic!("prev requires later modality"),
                }
            }
            Expr::Alloc(ty, args) => {
                for arg in args {
                    self.check_expr(arg, env, res_consumed, local_resources);
                }
                Type::Co(Box::new(ty.clone()))
            }
            Expr::Free(e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                match ty {
                    Type::Co(_) => Type::Void,
                    _ => panic!("free requires co type"),
                }
            }
            Expr::Get(e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                match ty {
                    Type::Optic(inner) | Type::RecOptic(inner) | Type::AtomicOptic(inner) | Type::Co(inner) => *inner,
                    _ => panic!("get requires optic or co type, found {:?}", ty),
                }
            }
            Expr::Put(e1, e2) => {
                self.check_expr(e1, env, res_consumed, local_resources);
                self.check_expr(e2, env, res_consumed, local_resources);
                Type::Void
            }
            Expr::Block(exprs) => {
                let mut last_ty = Type::Void;
                for e in exprs {
                    last_ty = self.check_expr(e, env, res_consumed, local_resources);
                }
                last_ty
            }
            Expr::Assign(n, e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                if !env.contains_key(n) {
                    env.insert(n.clone(), ty.clone());
                }
                Type::Void
            }
            Expr::ResourceDecl(n, e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                env.insert(n.clone(), ty);
                local_resources.insert(n.clone());
                Type::Void
            }
            Expr::Spawn(e) => {
                self.check_expr(e, env, res_consumed, local_resources);
                Type::Void
            }
            Expr::Call(e, args) => {
                if self.in_rec_optic && !self.guarded {
                   panic!("Unguarded recursive call in rec optic");
                }
                self.check_expr(e, env, res_consumed, local_resources);
                for arg in args {
                    self.check_expr(arg, env, res_consumed, local_resources);
                }
                Type::Void
            }
            Expr::Access(e, field) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                match ty {
                    Type::Named(name) => {
                        let fields = self.structs.get(&name).expect(&format!("Undefined struct {}", name));
                        fields.iter().find(|(f, _)| f == field).map(|(_, t)| t.clone()).expect(&format!("Field {} not found in struct {}", field, name))
                    }
                    Type::Struct(fields) => {
                        fields.iter().find(|(f, _)| f == field).map(|(_, t)| t.clone()).expect(&format!("Field {} not found in anonymous struct", field))
                    }
                    _ => panic!("Access requires struct type, found {:?}", ty),
                }
            }
            Expr::Compose(e1, e2) => {
                self.check_expr(e1, env, res_consumed, local_resources);
                self.check_expr(e2, env, res_consumed, local_resources);
                Type::Int // simplified focus
            }
            Expr::Unsafe(e) => self.check_expr(e, env, res_consumed, local_resources),
            Expr::Checked(e) => self.check_expr(e, env, res_consumed, local_resources),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[should_panic(expected = "Linearity violation")]
    fn test_linearity() {
        let mut sema = Sema::new();
        let mut env = HashMap::new();
        env.insert("f".to_string(), Type::Int);
        let mut consumed = HashSet::new();
        let mut locals = HashSet::new();
        locals.insert("f".to_string());

        let expr = Expr::Block(vec![
            Expr::Var("f".to_string()),
            Expr::Var("f".to_string()),
        ]);

        sema.check_expr(&expr, &mut env, &mut consumed, &mut locals);
    }

    #[test]
    #[should_panic(expected = "Unguarded recursive call")]
    fn test_productivity() {
        let mut sema = Sema::new();
        sema.in_rec_optic = true;
        sema.guarded = false;
        let mut env = HashMap::new();
        env.insert("f".to_string(), Type::Void);
        let mut consumed = HashSet::new();
        let mut locals = HashSet::new();

        let expr = Expr::Call(Box::new(Expr::Var("f".to_string())), vec![]);
        sema.check_expr(&expr, &mut env, &mut consumed, &mut locals);
    }

    #[test]
    fn test_productive_call() {
        let mut sema = Sema::new();
        sema.in_rec_optic = true;
        sema.guarded = false;
        let mut env = HashMap::new();
        env.insert("f".to_string(), Type::Void);
        let mut consumed = HashSet::new();
        let mut locals = HashSet::new();

        let expr = Expr::Next(Box::new(Expr::Call(Box::new(Expr::Var("f".to_string())), vec![])));
        sema.check_expr(&expr, &mut env, &mut consumed, &mut locals);
    }

    #[test]
    fn test_nominal_struct_sema() {
        let mut sema = Sema::new();
        let prog = Program {
            decls: vec![
                Decl::Struct {
                    name: "Point".to_string(),
                    fields: vec![("x".to_string(), Type::Int), ("y".to_string(), Type::Int)],
                },
                Decl::Func {
                    name: "get_x".to_string(),
                    params: vec![("p".to_string(), Type::Named("Point".to_string()))],
                    ret_type: Type::Int,
                    body: Expr::Access(Box::new(Expr::Var("p".to_string())), "x".to_string()),
                }
            ],
        };
        sema.check_program(&prog);
    }
}
