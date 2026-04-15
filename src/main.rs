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
    if args.len() < 2 {
        eprintln!("Usage: optico <file.oco> [--goir-tier=<diag|hybrid|cap>]");
        return;
    }

    let filepath = &args[1];
    let mut tier = "diag";
    for arg in &args[2..] {
        if arg.starts_with("--goir-tier=") {
            tier = &arg["--goir-tier=".len()..];
        }
    }

    let content = fs::read_to_string(filepath).expect("Failed to read input file");
    let mut parser = Parser::new(&content);
    let prog = parser.parse_program();

    let mut sema = Sema::new();
    if Path::new("beliefs.json").exists() {
        sema.load_beliefs("beliefs.json");
    }
    // sema.check_program(&prog);

    let cir = CIRLowerer::lower_program(&prog);
    let code = CodeGenerator::generate(&cir);

    // Write generated IR to file
    let llvm_path = Path::new(filepath).with_extension("ll");
    fs::write(&llvm_path, &code).expect("Failed to write LLVM IR file");

    println!("--- OptiCo CIR for {} ---", filepath);
    println!("{}", code);

    if llvm_path.exists() {
        println!("\n--- Driving GOIR Compilation Pipeline ---");
        let goir_passes = "go-init,go-propagate,go-check-insert,go-mem-intrinsic,go-lower";
        let output_ll = Path::new(filepath).with_extension("goir.ll");
        let output_obj = Path::new(filepath).with_extension("o");
        let output_bin = Path::new(filepath).with_extension("bin");

        let opt_bin = if Command::new("opt").arg("--version").status().is_ok() { "opt".to_string() } else { "/usr/lib/llvm-18/bin/opt".to_string() };
        let llc_bin = if Command::new("llc").arg("--version").status().is_ok() { "llc".to_string() } else { "/usr/lib/llvm-18/bin/llc".to_string() };
        let clang_bin = "clang-18";

        // 1. Run opt with GOIR passes
        let opt_status = Command::new(&opt_bin)
            .arg("-load-pass-plugin=build/passes/libGOIRPasses.so")
            .arg(format!("-passes={}", goir_passes))
            .arg(format!("-go-tier={}", match tier { "diag" => "0", "hybrid" => "1", "cap" => "2", _ => "0" }))
            .arg("-S")
            .arg(&llvm_path)
            .arg("-o")
            .arg(&output_ll)
            .status();

        println!("  Running opt: {:?} with plugin build/passes/libGOIRPasses.so", opt_bin);
        if let Ok(s) = opt_status {
            if s.success() {
                println!("  [1/3] opt: Successfully instrumented -> {:?}", output_ll);

                // 2. llc to object code
                let llc_status = Command::new(&llc_bin)
                    .arg("-filetype=obj")
                    .arg(&output_ll)
                    .arg("-o")
                    .arg(&output_obj)
                    .status();

                if let Ok(s2) = llc_status {
                    if s2.success() {
                        println!("  [2/3] llc: Generated object file -> {:?}", output_obj);

                        // 3. Link with libgoirrt
                        let clang_status = Command::new(clang_bin)
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
