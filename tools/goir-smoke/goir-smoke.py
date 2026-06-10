#!/usr/bin/env python3
import os
import subprocess
import json
import sys

MICRO_C_DIR = "tests/microc"
RUNTIME_LIB = "build/runtime/libgoirrt.so"
CLANG = "clang"

def run_test(test_file):
    name = os.path.basename(test_file)
    print(f"Running {name}...", end=" ", flush=True)

    # Simple compilation without passes for now (just linking runtime)
    exe = f"/tmp/{name}.exe"
    try:
        subprocess.check_call([CLANG, test_file, RUNTIME_LIB, "-Iruntime/include", "-o", exe])
        result = subprocess.run([exe], capture_output=True, text=True, timeout=5)
        if result.returncode == 0:
            print("PASS")
            return True
        else:
            print(f"FAIL (code {result.returncode})")
            return False
    except Exception as e:
        print(f"ERROR: {e}")
        return False

def main():
    if not os.path.exists(RUNTIME_LIB):
        print(f"Error: {RUNTIME_LIB} not found. Build it first.")
        sys.exit(1)

    tests = [os.path.join(MICRO_C_DIR, f) for f in os.listdir(MICRO_C_DIR) if f.endswith(".c")]
    passed = 0
    for t in tests:
        if run_test(t):
            passed += 1

    print(f"\nSummary: {passed}/{len(tests)} tests passed.")

if __name__ == "__main__":
    main()
