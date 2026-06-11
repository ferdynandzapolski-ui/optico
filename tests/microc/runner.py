#!/usr/bin/env python3
import os
import subprocess
import sys
import argparse
import json

def run_test(test_file, clang_path, rt_lib_path, rt_inc_path):
    test_name = os.path.basename(test_file)
    output_bin = test_file.replace('.c', '.bin')

    # Compile
    compile_cmd = [
        clang_path,
        "-I", rt_inc_path,
        test_file,
        "-L", os.path.dirname(rt_lib_path),
        "-lgoirrt",
        "-o", output_bin
    ]

    try:
        subprocess.check_call(compile_cmd)
    except subprocess.CalledProcessError as e:
        return {"name": test_name, "status": "COMPILE_FAIL", "error": str(e)}

    # Run
    try:
        # Add runtime lib path to LD_LIBRARY_PATH
        env = os.environ.copy()
        env["LD_LIBRARY_PATH"] = os.path.dirname(rt_lib_path) + ":" + env.get("LD_LIBRARY_PATH", "")

        result = subprocess.run([output_bin], capture_output=True, text=True, env=env, timeout=10)

        return {
            "name": test_name,
            "status": "PASS" if result.returncode == 0 else "TRAP",
            "exit_code": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr
        }
    except Exception as e:
        return {"name": test_name, "status": "RUN_FAIL", "error": str(e)}
    finally:
        if os.path.exists(output_bin):
            os.remove(output_bin)

def main():
    parser = argparse.ArgumentParser(description="GOIR MicroC Runner")
    parser.add_argument("--clang", default="clang", help="Path to clang")
    parser.add_argument("--rt-lib", required=True, help="Path to libgoirrt directory")
    parser.add_argument("--rt-inc", required=True, help="Path to goirrt headers")
    parser.add_argument("tests", nargs="+", help="C test files")

    args = parser.parse_args()

    results = []
    for test in args.tests:
        print(f"Running {test}...")
        res = run_test(test, args.clang, args.rt_lib, args.rt_inc)
        results.append(res)
        print(f"  Result: {res['status']}")

    print("\nSummary:")
    for res in results:
        print(f"{res['name']}: {res['status']}")

    with open("microc_results.json", "w") as f:
        json.dump(results, f, indent=2)

if __name__ == "__main__":
    main()
