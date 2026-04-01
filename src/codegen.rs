use crate::cir::*;

pub struct CodeGenerator;

impl CodeGenerator {
    pub fn generate(ops: &[CIROp]) -> String {
        let mut out = String::new();
        for op in ops {
            out.push_str(&Self::gen_op(op, 0));
            out.push('\n');
        }
        out
    }

    fn gen_op(op: &CIROp, indent: usize) -> String {
        let pad = "  ".repeat(indent);
        match op {
            CIROp::Load(n) => format!("{}load {}", pad, n),
            CIROp::Store(n, v) => format!("{}store {} = (\n{}{})", pad, n, Self::gen_op(v, indent + 1), pad),
            CIROp::Get(v) => format!("{}get (\n{}{})", pad, Self::gen_op(v, indent + 1), pad),
            CIROp::Put(e1, e2) => format!("{}put (\n{}{},\n{}{})", pad, Self::gen_op(e1, indent + 1), pad, Self::gen_op(e2, indent + 1), pad),
            CIROp::Compose(e1, e2) => format!("{}compose (\n{}{},\n{}{})", pad, Self::gen_op(e1, indent + 1), pad, Self::gen_op(e2, indent + 1), pad),
            CIROp::Alloc(ty, args) => {
                let mut s = format!("{}alloc<{:?}> (\n", pad, ty);
                for arg in args {
                    s.push_str(&Self::gen_op(arg, indent + 1));
                    s.push_str(",\n");
                }
                s.push_str(&format!("{})", pad));
                s
            }
            CIROp::Free(e) => format!("{}free (\n{}{})", pad, Self::gen_op(e, indent + 1), pad),
            CIROp::Next(e) => format!("{}next (\n{}{})", pad, Self::gen_op(e, indent + 1), pad),
            CIROp::Prev(e) => format!("{}prev (\n{}{})", pad, Self::gen_op(e, indent + 1), pad),
            CIROp::Call(n, args) => {
                let mut s = format!("{}call {} (\n", pad, n);
                for arg in args {
                    s.push_str(&Self::gen_op(arg, indent + 1));
                    s.push_str(",\n");
                }
                s.push_str(&format!("{})", pad));
                s
            }
            CIROp::Spawn(e) => format!("{}spawn (\n{}{})", pad, Self::gen_op(e, indent + 1), pad),
            CIROp::Block(ops) => {
                let mut s = format!("{}block {{\n", pad);
                for op in ops {
                    s.push_str(&Self::gen_op(op, indent + 1));
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
                format!("{}resource {:?} {} = (\n{}{})", pad, ty, name, Self::gen_op(val, indent + 1), pad)
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
