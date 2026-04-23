use crate::cir::*;
use crate::ast::*;
use std::collections::HashMap;
use std::collections::HashMap;

pub struct CodeGenerator {
    temp_count: u32,
    func_count: u32,
    function_ret_types: HashMap<String, Type>,
}

impl CodeGenerator {
    pub fn new() -> Self {
        Self {
            temp_count: 0,
            func_count: 0,
            function_ret_types: HashMap::new(),
        }
    }

    pub fn generate_program(&mut self, prog: &Program) -> String {
        // Populate function return types
        for decl in &prog.decls {
            match decl {
                Decl::Func { name, ret_type, .. } => {
                    self.function_ret_types.insert(name.clone(), ret_type.clone());
                }
                Decl::ExternC(extern_decls) => {
                    for ext_decl in extern_decls {
                        if let Decl::Func { name, ret_type, .. } = ext_decl {
                            self.function_ret_types.insert(name.clone(), ret_type.clone());
                        }
                    }
                }
                _ => {} // Ignore other declarations
            }
        }

        let mut out = String::from("; ModuleID = 'opticoc'\n");
        out.push_str("target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128\"\n");
        out.push_str("target triple = \"x86_64-pc-linux-gnu\"\n\n");

        // Generate extern declarations
        for decl in &prog.decls {
            if let Decl::ExternC(extern_decls) = decl {
                for ext_decl in extern_decls {
                    if let Decl::Func { name, params, ret_type, .. } = ext_decl {
                        out.push_str(&self.gen_extern_func(name, params, ret_type));
                    }
                }
            }
        }

        // Generate global variables
        for decl in &prog.decls {
            if let Decl::Global(name, ty, _) = decl {
                let llvm_ty = self.type_to_llvm(ty);
                out.push_str(&format!("@{} = global {} 0\n", name, llvm_ty));
            }
        }

        out.push_str("\n");

        // Generate function definitions
        for decl in &prog.decls {
            if let Decl::Func { name, params, ret_type, body } = decl {
                out.push_str(&self.gen_func_def(name, params, ret_type, body));
            }
        }

        out
    }

    fn gen_extern_func(&self, name: &str, params: &[(String, Type)], ret_type: &Type) -> String {
        let ret_ty = self.type_to_llvm(ret_type);
        let param_str = params.iter()
            .map(|(_, ty)| self.type_to_llvm(ty))
            .collect::<Vec<_>>()
            .join(", ");
        format!("declare {} @{}({})\n", ret_ty, name, param_str)
    }

    fn gen_func_decl(&self, name: &str, params: &[(String, Type)], ret_type: &Type) -> String {
        let ret_ty = self.type_to_llvm(ret_type);
        let param_str = params.iter()
            .map(|(n, ty)| format!("{} %{}", self.type_to_llvm(ty), n))
            .collect::<Vec<_>>()
            .join(", ");
        format!("declare {} @{}({})\n", ret_ty, name, param_str)
    }

    fn gen_func_def(&mut self, name: &str, params: &[(String, Type)], ret_type: &Type, body: &Expr) -> String {
        let ret_ty = self.type_to_llvm(ret_type);
        let param_str = params.iter()
            .map(|(n, ty)| format!("{} %{}", self.type_to_llvm(ty), n))
            .collect::<Vec<_>>()
            .join(", ");

        let mut out = format!("define {} @{}({}) {{\n", ret_ty, name, param_str);

        // Allocate parameters
        for (param_name, param_type) in params {
            let llvm_ty = self.type_to_llvm(param_type);
            let temp = self.new_temp();
            out.push_str(&format!("  %{} = alloca {}\n", temp, llvm_ty));
            out.push_str(&format!("  store {} %{}, {}* %{}\n", llvm_ty, param_name, llvm_ty, temp));
        }

        // Generate body
        if matches!(body, Expr::Return(_)) {
            // Explicit return statement
            out.push_str(&self.gen_expr(body, 1));
        } else if matches!(body, Expr::Block(_)) {
            if let Expr::Block(exprs) = body {
                if exprs.len() == 1 && !matches!(exprs[0], Expr::Return(_)) {
                    // Single expression block - treat as expression body
                    let (code, val) = self.gen_expr_value(&exprs[0], 1);
                    out.push_str(&code);
                    if ret_type == &Type::Void {
                        out.push_str("  ret void\n");
                    } else {
                        out.push_str(&format!("  ret i32 {}\n", val));
                    }
                } else {
                    // Block body
                    out.push_str(&self.gen_expr(body, 1));
                    // Default return
                    if ret_type == &Type::Void {
                        out.push_str("  ret void\n");
                    } else {
                        out.push_str("  ret i32 0\n");
                    }
                }
            }
        } else {
            // Expression body - generate code and return result
            let (code, val) = self.gen_expr_value(body, 1);
            out.push_str(&code);
            if ret_type == &Type::Void {
                out.push_str("  ret void\n");
            } else {
                out.push_str(&format!("  ret i32 {}\n", val));
            }
        }

        out.push_str("}\n\n");
        out
    }

    fn gen_expr(&mut self, expr: &Expr, indent: usize) -> String {
        let indent_str = "  ".repeat(indent);
        match expr {
            Expr::Var(_) => {
                // Variable access doesn't generate code by itself
                String::new()
            }
            Expr::BinOp(_, _, _) => {
                let (code, _) = self.gen_expr_value(expr, indent);
                code
            }
            Expr::Put(target, value) => {
                let (value_code, value_val) = self.gen_expr_value(value, indent);
                if let Expr::Var(var_name) = &**target {
                    format!("{}  store i32 {}, i32* @{}", value_code, value_val, var_name)
                } else {
                    // For now, only handle global variable assignments
                    String::new()
                }
            }
            Expr::Call(callee, args) => {
                if let Expr::Var(func_name) = &**callee {
                    let (code, _) = self.gen_expr_value(expr, indent);
                    code
                } else {
                    String::new()
                }
            }
            }
            Expr::Return(expr) => {
                let (code, val) = self.gen_expr_value(expr, indent);
                format!("{}{}  ret i32 {}\n", code, indent_str, val)
            }
            Expr::Block(exprs) => {
                let mut out = String::new();
                for expr in exprs {
                    out.push_str(&self.gen_expr(expr, indent));
                }
                out
            }
            _ => String::new(),
        }
    }

    fn type_to_llvm(&self, ty: &Type) -> &'static str {
        match ty {
            Type::Int => "i32",
            Type::Void => "void",
            Type::Char => "i8",
            Type::Bool => "i1",
            _ => "i32", // Default
        }
    }

    fn new_temp(&mut self) -> String {
        self.temp_count += 1;
        format!("t{}", self.temp_count)
    }

    fn gen_expr_value(&mut self, expr: &Expr, indent: usize) -> (String, String) {
        // Returns (generated_code, result_register)
        match expr {
            Expr::ConstInt(val) => ("".to_string(), val.to_string()),
            Expr::Var(name) => ("".to_string(), format!("%{}", name)),
            Expr::BinOp(kind, left, right) => {
                let (left_code, left_val) = self.gen_expr_value(left, indent);
                let (right_code, right_val) = self.gen_expr_value(right, indent);
                let temp = self.new_temp();

                let op = match kind {
                    BinOpKind::Add => "add",
                    BinOpKind::Sub => "sub",
                    BinOpKind::Mul => "mul",
                    BinOpKind::Div => "sdiv",
                    _ => "add",
                };

                let indent_str = "  ".repeat(indent);
                let code = format!("{}{}  %{} = {} i32 {}, {}\n",
                    left_code, right_code, temp, op, left_val, right_val);
                (code, format!("%{}", temp))
            }
            Expr::Put(target, value) => {
                let (value_code, value_val) = self.gen_expr_value(value, indent);
                if let Expr::Var(var_name) = &**target {
                    let code = format!("{}  store i32 {}, i32* @{}", value_code, value_val, var_name);
                    (code, "0".to_string()) // Assignment doesn't have a value
                } else {
                    ("".to_string(), "0".to_string())
                }
            }
            Expr::Call(callee, args) => {
                if let Expr::Var(func_name) = &**callee {
                    let mut arg_codes = String::new();
                    let mut arg_vals = Vec::new();

                    for arg in args {
                        let (arg_code, arg_val) = self.gen_expr_value(arg, indent);
                        arg_codes.push_str(&arg_code);
                        arg_vals.push(arg_val);
                    }

                    if let Some(ret_type) = self.function_ret_types.get(func_name) {
                        if *ret_type == Type::Void {
                            let arg_str = arg_vals.iter().map(|v| format!("i32 {}", v)).collect::<Vec<_>>().join(", ");
                            let code = format!("{}  call void @{}({})\n",
                                arg_codes, func_name, arg_str);
                            (code, "0".to_string())
                        } else {
                            let temp = self.new_temp();
                            let arg_str = arg_vals.iter().map(|v| format!("i32 {}", v)).collect::<Vec<_>>().join(", ");
                            let code = format!("{}  %{} = call i32 @{}({})\n",
                                arg_codes, temp, func_name, arg_str);
                            (code, format!("%{}", temp))
                        }
                    } else {
                        // Default to i32 return
                        let temp = self.new_temp();
                        let arg_str = arg_vals.iter().map(|v| format!("i32 {}", v)).collect::<Vec<_>>().join(", ");
                        let code = format!("{}  %{} = call i32 @{}({})\n",
                            arg_codes, temp, func_name, arg_str);
                        (code, format!("%{}", temp))
                    }
                } else {
                    ("".to_string(), "0".to_string())
                }
            }

                    let arg_str = arg_vals.iter().map(|v| format!("i32 {}", v)).collect::<Vec<_>>().join(", ");

                    if func_name == "print_int" {
                        // print_int is void
                        let code = format!("{}  call void @{}({})\n",
                            arg_codes, func_name, arg_str);
                        (code, "0".to_string()) // Dummy value since void
                    } else {
                        let temp = self.new_temp();
                        let code = format!("{}  %{} = call i32 @{}({})\n",
                            arg_codes, temp, func_name, arg_str);
                        (code, format!("%{}", temp))
                    }
                } else {
                    ("".to_string(), "0".to_string())
                }
            }
            _ => ("".to_string(), "0".to_string()),
        }
    }

    fn gen_op(&self, op: &CIROp, indent: usize) -> String {
        let pad = "  ".repeat(indent);
        match op {
            CIROp::Load(n) => format!("{}load {}", pad, n),
            CIROp::Store(n, v) => format!("{}store {} = (\n{}{})", pad, n, self.gen_op(v, indent + 1), pad),
            CIROp::Get(v) => format!("{}get (\n{}{})", pad, self.gen_op(v, indent + 1), pad),
            CIROp::Put(e1, e2) => format!("{}put (\n{}{},\n{}{})", pad, self.gen_op(e1, indent + 1), pad, self.gen_op(e2, indent + 1), pad),
            CIROp::Compose(e1, e2) => format!("{}compose (\n{}{},\n{}{})", pad, self.gen_op(e1, indent + 1), pad, self.gen_op(e2, indent + 1), pad),
            CIROp::Alloc(ty, args, dur) => {
                let mut s = format!("{}alloc<{:?}, {:?}> (\n", pad, ty, dur);
                for arg in args {
                    s.push_str(&self.gen_op(arg, indent + 1));
                    s.push_str(",\n");
                }
                s.push_str(&format!("{})", pad));
                s
            }
            CIROp::Free(e) => format!("{}free (\n{}{})", pad, self.gen_op(e, indent + 1), pad),
            CIROp::Fetch(fp) => format!("{}fetch {:?}", pad, fp),
            CIROp::Persist(v, dur) => format!("{}persist (\n{}{},\n{}  {:?})", pad, self.gen_op(v, indent + 1), pad, pad, dur),
            CIROp::Checkpoint(n) => format!("{}checkpoint {}", pad, n),
            CIROp::Next(e, clock) => format!("{}next<{:?}> (\n{}{})", pad, clock, self.gen_op(e, indent + 1), pad),
            CIROp::Prev(e, clock) => format!("{}prev<{:?}> (\n{}{})", pad, clock, self.gen_op(e, indent + 1), pad),
            CIROp::Call(n, args) => {
                let mut s = format!("{}call {} (\n", pad, n);
                for arg in args {
                    s.push_str(&self.gen_op(arg, indent + 1));
                    s.push_str(",\n");
                }
                s.push_str(&format!("{})", pad));
                s
            }
            CIROp::Spawn(e) => format!("{}spawn (\n{}{})", pad, self.gen_op(e, indent + 1), pad),
            CIROp::Block(ops) => {
                let mut s = format!("{}block {{\n", pad);
                for op in ops {
                    s.push_str(&self.gen_op(op, indent + 1));
                    s.push_str("\n");
                }
                s.push_str(&format!("{}}}", pad));
                s
            }
            CIROp::StructDecl(name, fields) => {
                let mut s = format!("{}struct {} {{\n", pad, name);
                for (f_name, f_ty) in fields {
                    s.push_str(&format!("{}  {:?} {};\n", pad, f_ty, f_name));
                }
                s.push_str(&format!("{}}}", pad));
                s
            }
            CIROp::ResourceDecl(name, ty, val) => {
                format!("{}resource {:?} {} = (\n{}{})", pad, ty, name, self.gen_op(val, indent + 1), pad)
            }
            CIROp::ProtocolDecl(name, states) => {
                let mut s = format!("{}protocol {} {{\n", pad, name);
                for state in states {
                    s.push_str(&format!("{}  state {} {{\n", pad, state.name));
                    for (o_name, o_ty) in &state.optics {
                        s.push_str(&format!("{}    {:?} {};\n", pad, o_ty, o_name));
                    }
                    s.push_str(&format!("{}  }}\n", pad));
                }
                s.push_str(&format!("{}}}", pad));
                s
            }
        }
    }
}