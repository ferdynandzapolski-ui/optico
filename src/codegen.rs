use crate::cir::*;
use crate::ast::*;
use std::collections::HashMap;

pub struct CodeGenerator {
    temp_count: u32,
    func_count: u32,
    function_ret_types: HashMap<String, Type>,
    local_vars: HashMap<String, String>, // var_name -> llvm_temp_name
}

impl CodeGenerator {
    pub fn new() -> Self {
        Self {
            temp_count: 0,
            func_count: 0,
            function_ret_types: HashMap::new(),
            local_vars: HashMap::new(),
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
        // Clear local variables for new function
        self.local_vars.clear();

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
            // Parameters are also local variables
            self.local_vars.insert(param_name.clone(), temp);
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
            Expr::LocalDecl(var_name, var_type, init_expr) => {
                let llvm_ty = self.type_to_llvm(var_type);
                let temp = self.new_temp();
                let (init_code, init_val) = self.gen_expr_value(init_expr, indent);
                self.local_vars.insert(var_name.clone(), temp.clone());
                format!("{}  %{} = alloca {}\n{}  store {} {}, {}* %{}\n",
                    indent_str, temp, llvm_ty, init_code, llvm_ty, init_val, llvm_ty, temp)
            }
            Expr::Assign(var_name, value_expr) => {
                let (value_code, value_val) = self.gen_expr_value(value_expr, indent);
                if let Some(temp) = self.local_vars.get(var_name) {
                    format!("{}{}  store i32 {}, i32* %{}\n", indent_str, value_code, value_val, temp)
                } else {
                    // Assume global for now
                    format!("{}{}  store i32 {}, i32* @{}\n", indent_str, value_code, value_val, var_name)
                }
            }
            Expr::BinOp(_, _, _) => {
                let (code, _) = self.gen_expr_value(expr, indent);
                code
            }
            Expr::Put(target, value) => {
                let (value_code, value_val) = self.gen_expr_value(value, indent);
                if let Expr::Var(var_name) = &**target {
                    format!("{}  store i32 {}, i32* @{}", indent_str, value_val, var_name)
                } else {
                    // For now, only handle global variable assignments
                    String::new()
                }
            }
            Expr::FieldAssign(base, value) => {
                let (base_code, base_ptr) = self.gen_expr_value(base, indent);
                let (value_code, value_val) = self.gen_expr_value(value, indent);
                format!("{}{}  store i32 {}, i32* %{}\n", indent_str, value_code, value_val, base_ptr)
            }
            Expr::IndexAssign(base, index, value) => {
                // For index assignment: base[index] = value
                // Get the pointer to the base array
                let (base_code, base_ptr) = self.gen_expr_value(base, indent);
                let (index_code, index_val) = self.gen_expr_value(index, indent);
                let (value_code, value_val) = self.gen_expr_value(value, indent);
                
                // Generate getelementptr to get pointer to array element
                let elem_ptr = self.new_temp();
                format!("{}{}{}{}  %{} = getelementptr i32, i32* {}, i32 {}\n  store i32 {}, i32* %{}\n",
                    base_code, index_code, value_code, indent_str, elem_ptr, base_ptr, index_val, value_val, elem_ptr)
            }
            Expr::If(cond, then_branch, else_branch) => {
                let (cond_code, cond_val) = self.gen_expr_value(cond, indent);
                let then_label = format!("then_{}", self.temp_count);
                let else_label = format!("else_{}", self.temp_count);
                let end_label = format!("end_{}", self.temp_count);
                self.temp_count += 1;

                let mut code = cond_code;
                code.push_str(&format!("{}  br i1 {}, label %{}, label %{}\n\n",
                    "  ".repeat(indent), cond_val, then_label, else_label));

                // Then branch
                code.push_str(&format!("{}:\n", then_label));
                code.push_str(&self.gen_expr(then_branch, indent));
                code.push_str(&format!("{}  br label %{}\n\n", "  ".repeat(indent), end_label));

                // Else branch
                code.push_str(&format!("{}:\n", else_label));
                if let Some(else_expr) = else_branch {
                    code.push_str(&self.gen_expr(else_expr, indent));
                }
                code.push_str(&format!("{}  br label %{}\n\n", "  ".repeat(indent), end_label));

                // End
                code.push_str(&format!("{}:\n", end_label));
                code
            }
            Expr::While(cond, body) => {
                let loop_label = format!("loop_{}", self.temp_count);
                let body_label = format!("body_{}", self.temp_count);
                let end_label = format!("end_loop_{}", self.temp_count);
                self.temp_count += 1;

                let mut code = format!("{}  br label %{}\n\n", "  ".repeat(indent), loop_label);

                // Loop condition check
                code.push_str(&format!("{}:\n", loop_label));
                let (cond_code, cond_val) = self.gen_expr_value(cond, indent + 1);
                code.push_str(&cond_code);
                code.push_str(&format!("{}  br i1 {}, label %{}, label %{}\n\n",
                    "  ".repeat(indent + 1), cond_val, body_label, end_label));

                // Loop body
                code.push_str(&format!("{}:\n", body_label));
                code.push_str(&self.gen_expr(body, indent + 1));
                code.push_str(&format!("{}  br label %{}\n\n", "  ".repeat(indent + 1), loop_label));

                // End of loop
                code.push_str(&format!("{}:\n", end_label));
                code
            }
            Expr::Call(callee, args) => {
                if let Expr::Var(func_name) = &**callee {
                    let (code, _) = self.gen_expr_value(expr, indent);
                    code
                } else {
                    String::new()
                }
            },
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
            Type::Pointer(_, _, _) => "ptr",
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
            Expr::Var(name) => {
                if let Some(temp_name) = self.local_vars.get(name) {
                    // Clone the temp name to avoid holding the borrow during new_temp() call
                    let temp_name_clone = temp_name.clone();
                    let load_temp = self.new_temp();
                    let code = format!("  %{} = load i32, i32* %{}\n", load_temp, temp_name_clone);
                    (code, format!("%{}", load_temp))
                } else {
                    // Parameter or global
                    ("".to_string(), format!("%{}", name))
                }
            }
            Expr::BinOp(kind, left, right) => {
                let (left_code, left_val) = self.gen_expr_value(left, indent);
                let (right_code, right_val) = self.gen_expr_value(right, indent);
                let temp = self.new_temp();

                let (op, ret_ty) = match kind {
                    BinOpKind::Add => ("add", "i32"),
                    BinOpKind::Sub => ("sub", "i32"),
                    BinOpKind::Mul => ("mul", "i32"),
                    BinOpKind::Div => ("sdiv", "i32"),
                    BinOpKind::Eq => ("icmp eq", "i1"),
                    BinOpKind::Ne => ("icmp ne", "i1"),
                    BinOpKind::Lt => ("icmp slt", "i1"),
                    BinOpKind::Gt => ("icmp sgt", "i1"),
                    BinOpKind::Le => ("icmp sle", "i1"),
                    BinOpKind::Ge => ("icmp sge", "i1"),
                    BinOpKind::And => ("and", "i32"),
                    _ => ("add", "i32"),
                };

                let code = format!("{}{}  %{} = {} {} {}, {}\n",
                    left_code, right_code, temp, op, ret_ty, left_val, right_val);
                (code, format!("%{}", temp))
            }
            Expr::LocalDecl(var_name, var_type, init_expr) => {
                let llvm_ty = self.type_to_llvm(var_type);
                let temp = self.new_temp();
                let (init_code, init_val) = self.gen_expr_value(init_expr, indent);
                let code = format!("  %{} = alloca {}\n{}  store {} {}, {}* %{}\n",
                    temp, llvm_ty, init_code, llvm_ty, init_val, llvm_ty, temp);
                // Store the allocated temp for this variable (clone to keep ownership for return)
                self.local_vars.insert(var_name.clone(), temp.clone());
                (code, format!("%{}", temp)) // Return the allocated pointer
            }
            Expr::Assign(var_name, value_expr) => {
                let (value_code, value_val) = self.gen_expr_value(value_expr, indent);
                // TODO: Need to track variable locations for proper assignment
                let code = format!("{}  ; Assignment to {} = {}\n", value_code, var_name, value_val);
                (code, "0".to_string()) // Assignment doesn't have a value
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