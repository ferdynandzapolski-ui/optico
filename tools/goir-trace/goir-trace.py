#!/usr/bin/env python3
import json
import argparse
import sys

def summarize_trace(trace_file):
    events = []
    try:
        with open(trace_file, 'r') as f:
            for line in f:
                if line.strip():
                    events.append(json.loads(line))
    except Exception as e:
        print(f"Error reading trace file: {e}")
        return

    print(f"--- GOIR Trace Summary: {trace_file} ---")
    print(f"Total events: {len(events)}")

    counts = {}
    for ev in events:
        etype = ev.get('event', 'unknown')
        counts[etype] = counts.get(etype, 0) + 1

    print("\nEvent counts:")
    for etype, count in counts.items():
        print(f"  {etype}: {count}")

    check_fails = [ev for ev in events if ev.get('event') == 'check_fail']
    if check_fails:
        print("\nCheck Failures:")
        for ev in check_fails:
            fail = ev.get('fail', {})
            site = ev.get('site', {})
            print(f"  [{fail.get('component')}] {fail.get('detail')} at {site.get('file')}:{site.get('line')} (ptr: {ev.get('ptr')})")

def main():
    parser = argparse.ArgumentParser(description="GOIR Trace Summarizer")
    parser.add_argument("trace_file", help="Path to JSONL trace file")
    args = parser.parse_args()
    summarize_trace(args.trace_file)

if __name__ == "__main__":
    main()
