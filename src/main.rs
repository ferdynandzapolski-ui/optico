use app::ast::Program;
use app::parser::Parser;
use app::sema::Sema;
use app::cir::CIRLowerer;
use app::codegen::CodeGenerator;
use std::env;
use std::fs;
use std::process::Command;
use std::path::Path;

fn main() {
    let args: Vec<String> = env::args().collect();
    let mut input_files = Vec::new();
    let mut tier = "diag";
    let mut prov_policy = "pnvi-plain";
    let mut debug_ast = false;

    for arg in args.iter().skip(1) {
        if arg == "--debug-ast" {
            debug_ast = true;
        } else if arg.starts_with("--goir-tier=") {
            tier = &arg["--goir-tier=".len()..];
        } else if arg.starts_with("--goir-provenance-policy=") {
            prov_policy = &arg["--goir-provenance-policy=".len()..];
        } else if arg.starts_with("-") {
            // ignore other flags
        } else {
            input_files.push(arg);
        }
    }

    if input_files.is_empty() {
        eprintln!("Usage: optico <file1.oco> [file2.oco ...] [--goir-tier=<diag|hybrid|cap>]");
        return;
    }

    let mut all_decls = Vec::new();
    for file in &input_files {
        let content = fs::read_to_string(file).expect(&format!("Failed to read input file: {}", file));
        let mut parser = Parser::new(&content);
        let prog = parser.parse_program();
        all_decls.extend(prog.decls);
    }

    let prog = Program { decls: all_decls };
    let filepath = input_files.last().unwrap();

    // Debug: Print AST
    if debug_ast {
        println!("--- AST for {} ---", filepath);
        for decl in &prog.decls {
            println!("{:?}", decl);
        }
        println!();
    }

    let mut sema = Sema::new();
    if Path::new("beliefs.json").exists() {
        sema.load_beliefs("beliefs.json");
    }
    sema.check_program(&prog);

    let mut codegen = CodeGenerator::new();
    let code = codegen.generate_program(&prog);

    // Write generated IR to file
    let llvm_path = Path::new(filepath).with_extension("ll");
    fs::write(&llvm_path, &code).expect("Failed to write LLVM IR file");

    println!("--- OptiCo LLVM IR for {} ---", filepath);
    println!("{}", code);

    if llvm_path.exists() {
        println!("\n--- Driving GOIR Compilation Pipeline ---");
        let goir_passes = "go-init,go-propagate,go-check-insert,go-mem-intrinsic,go-lower";
        let output_ll = Path::new(filepath).with_extension("goir.ll");
        let output_obj = Path::new(filepath).with_extension("o");
        let output_bin = Path::new(filepath).with_extension("bin");

        let tier_val = match tier {
            "diag" => "0",
            "hybrid" => "1",
            "cap" => "2",
            _ => "0",
        };

        // 1. Run opt with GOIR passes
        let opt_status = Command::new("opt")
            .arg("-load-pass-plugin=build/passes/libGOIRPasses.so")
             .arg(format!("-passes={}", goir_passes))
             .arg(format!("-go-tier={}", tier_val))
             .arg("-S")
             .arg(&llvm_path)
             .arg("-o")
             .arg(&output_ll)
             .status();

        if let Ok(s) = opt_status {
            if s.success() {
                println!("  [1/3] opt: Successfully instrumented -> {:?}", output_ll);

                // 2. llc to object code
                let llc_status = Command::new("llc")
                    .arg("-filetype=obj")
                    .arg(&output_ll)
                    .arg("-o")
                    .arg(&output_obj)
                    .status();

                if let Ok(s2) = llc_status {
                    if s2.success() {
                        println!("  [2/3] llc: Generated object file -> {:?}", output_obj);

                        // 3. Link with libgoirrt
                        let clang_status = Command::new("clang")
                            .arg(&output_obj)
                            .arg("-Lbuild/runtime")
                            .arg("-lgoirrt")
                            .arg("-o")
                            .arg(&output_bin)
                            .status();

                        if let Ok(s3) = clang_status {
                            if s3.success() {
                                println!("  [3/3] clang: Linked executable -> {:?}", output_bin);
                                println!("\nCompilation complete. Run with: LD_LIBRARY_PATH=build/runtime {:?}", output_bin);
                            } else {
                                eprintln!("  [3/3] clang: Linking failed");
                            }
                        }
                    } else {
                        eprintln!("  [2/3] llc: Codegen failed");
                    }
                }
            } else {
                eprintln!("  [1/3] opt: Instrumentation failed");
            }
        }
    }
}
