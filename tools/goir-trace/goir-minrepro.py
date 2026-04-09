#!/usr/bin/env python3
import json
import argparse
import sys

def generate_hint(trace_file):
    events = []
    try:
        with open(trace_file, 'r') as f:
            for line in f:
                if line.strip():
                    events.append(json.loads(line))
    except Exception as e:
        print(f"Error reading trace file: {e}")
        return

    check_fails = [ev for ev in events if ev.get('event') == 'check_fail']
    if not check_fails:
        print("No check failures found in trace.")
        return

    print("--- Minimized Reproducer Hints ---")
    for ev in check_fails:
        fail = ev.get('fail', {})
        site = ev.get('site', {})
        print(f"To reproduce the {fail.get('component')} failure:")
        print(f"  Focus on: {site.get('file')}:{site.get('line')} in function {site.get('fn')}")
        print(f"  Pointer: {ev.get('ptr')}")
        print(f"  Grade at failure: {json.dumps(ev.get('grade'))}")
        print("-" * 30)

def main():
    parser = argparse.ArgumentParser(description="GOIR Minimized Reproducer Hint Generator")
    parser.add_argument("trace_file", help="Path to JSONL trace file")
    args = parser.parse_args()
    generate_hint(args.trace_file)

if __name__ == "__main__":
    main()
