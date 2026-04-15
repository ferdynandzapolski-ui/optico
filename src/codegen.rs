use crate::cir::*;

pub struct CodeGenerator;

impl CodeGenerator {
    pub fn generate(ops: &[CIROp]) -> String {
        let mut out = String::new();
        out.push_str("target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128\"\n");
        out.push_str("target triple = \"x86_64-pc-linux-gnu\"\n\n");

        out.push_str("declare ptr @malloc(i64)\n");
        out.push_str("declare void @free(ptr)\n\n");

        out.push_str("define i32 @main() {\n");
        out.push_str("  %1 = alloca i32, align 4\n");
        out.push_str("  store i32 0, ptr %1, align 4\n");

        let mut counter = 0;
        for op in ops {
            out.push_str(&Self::gen_op(op, 1, &mut counter));
        }

        out.push_str("  ret i32 0\n");
        out.push_str("}\n");
        out
    }

    fn gen_op(op: &CIROp, indent: usize, counter: &mut usize) -> String {
        let pad = "  ".repeat(indent);
        let id = *counter;
        *counter += 1;
        match op {
            CIROp::Load(n) => {
                format!("{}%tmp_ptr_{} = load ptr, ptr %p_{}, align 8\n", pad, id, n)
            }
            CIROp::Store(n, v) => {
                let mut s = Self::gen_op(v, indent, counter);
                let last_id = *counter - 1;
                s.push_str(&format!("{}%p_{} = alloca ptr, align 8\n", pad, n));
                s.push_str(&format!("{}store ptr %tmp_val_{}, ptr %p_{}, align 8\n", pad, last_id, n));
                s
            }
            CIROp::Alloc(ty, _args, _dur) => {
                let size = match ty {
                    crate::ast::Type::Int => 4,
                    _ => 8,
                };
                format!("{}%tmp_val_{} = call ptr @malloc(i64 {})\n", pad, id, size)
            }
            CIROp::Free(e) => {
                let mut s = Self::gen_op(e, indent, counter);
                let last_id = *counter - 1;
                s.push_str(&format!("{}call void @free(ptr %tmp_ptr_{})\n", pad, last_id));
                s
            }
            CIROp::Block(ops) => {
                let mut s = String::new();
                for op in ops {
                    s.push_str(&Self::gen_op(op, indent, counter));
                }
                s
            }
            CIROp::Put(e1, e2) => {
                let mut s = Self::gen_op(e2, indent, counter);
                let id2 = *counter - 1;
                s.push_str(&Self::gen_op(e1, indent, counter));
                let id1 = *counter - 1;
                s.push_str(&format!("{}%tmp_loaded_{} = load i32, ptr %tmp_val_{}, align 4\n", pad, id, id2));
                s.push_str(&format!("{}store i32 %tmp_loaded_{}, ptr %tmp_ptr_{}, align 4\n", pad, id, id1));
                s
            }
            CIROp::Get(e) => {
                Self::gen_op(e, indent, counter)
            }
            CIROp::ConstInt(v) => {
                format!("{}%tmp_val_{}_raw = alloca i32, align 4\n{}store i32 {}, ptr %tmp_val_{}_raw, align 4\n{}%tmp_val_{} = load ptr, ptr %tmp_val_{}_raw, align 8\n", pad, id, pad, v, id, pad, id, id)
            }
            _ => format!("{}; unhandled op {:?}\n", pad, op),
        }
    }
}
