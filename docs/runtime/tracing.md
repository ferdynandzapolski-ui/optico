# GOIR Tracing System

The GOIR tracing system provides detailed JSONL logs of safety-related events during program execution.

## Enabling Tracing

To enable tracing, set the `GOIR_TRACE_FILE` environment variable to the desired output path:

```bash
export GOIR_TRACE_FILE=trace.jsonl
./my_program
```

If not set, traces are printed to `stderr`.

## Trace CLI Tools

### goir-trace.py

Summarizes a trace file:

```bash
./tools/goir-trace/goir-trace.py trace.jsonl
```

### goir-minrepro.py

Generates hints for creating a minimal reproducer from a failing trace:

```bash
./tools/goir-trace/goir-minrepro.py trace.jsonl
```

## Event Types

- `init`: Grade initialization (e.g., at allocation).
- `check_fail`: A safety check failed (bounds, life, etc.).
- `prov_expose`: Pointer provenance was exposed to an integer.
- `inttoptr_resolve`: An integer was resolved back to a pointer with provenance.
- `memcpy_taint`: Memory was tainted during a bulk copy.
