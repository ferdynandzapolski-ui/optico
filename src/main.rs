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

    for arg in args.iter().skip(1) {
        if arg.starts_with("--goir-tier=") {
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
        eprintln!("Usage: optico <file1.oco> [file2.oco ...] [--goir-tier=<diag|hybrid|cap>] [--goir-provenance-policy=<pnvi-plain|pnvi-ae>]");
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

    let mut sema = Sema::new();
    if Path::new("beliefs.json").exists() {
        sema.load_beliefs("beliefs.json");
    }
    sema.check_program(&prog);

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

        let mut opt_bin = "opt".to_string();
        let mut llc_bin = "llc".to_string();
        let mut clang_bin = "clang".to_string();

        if Command::new("opt").arg("--version").output().is_err() {
            if Path::new("/usr/lib/llvm-18/bin/opt").exists() { opt_bin = "/usr/lib/llvm-18/bin/opt".to_string(); }
            else if Path::new("/usr/bin/opt-18").exists() { opt_bin = "/usr/bin/opt-18".to_string(); }
        }
        if Command::new("llc").arg("--version").output().is_err() {
            if Path::new("/usr/lib/llvm-18/bin/llc").exists() { llc_bin = "/usr/lib/llvm-18/bin/llc".to_string(); }
            else if Path::new("/usr/bin/llc-18").exists() { llc_bin = "/usr/bin/llc-18".to_string(); }
        }
        if Command::new("clang").arg("--version").output().is_err() {
            if Path::new("/usr/lib/llvm-18/bin/clang").exists() { clang_bin = "/usr/lib/llvm-18/bin/clang".to_string(); }
            else if Path::new("/usr/bin/clang-18").exists() { clang_bin = "/usr/bin/clang-18".to_string(); }
        }

        let prov_policy_val = match prov_policy {
            "pnvi-plain" => "0",
            "pnvi-ae" => "1",
            _ => "0",
        };

        // 1. Run opt with GOIR passes
        let opt_status = Command::new(opt_bin)
            .arg("-load-pass-plugin=build/passes/libGOIRPasses.so")
            .arg(format!("-passes={}", goir_passes))
            .arg(format!("-go-tier={}", match tier { "diag" => "0", "hybrid" => "1", "cap" => "2", _ => "0" }))
            .arg(format!("-go-prov-policy={}", prov_policy_val))
            .arg("-S")
            .arg(&llvm_path)
            .arg("-o")
            .arg(&output_ll)
            .status();

        if let Ok(s) = opt_status {
            if s.success() {
                println!("  [1/3] opt: Successfully instrumented -> {:?}", output_ll);

                // 2. llc to object code
                let llc_status = Command::new(llc_bin)
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
