# GOIR JSONL Trace Schema

Each event in a GOIR trace is a single JSON object per line.

## Fields

- `ts`: Unix timestamp (seconds).
- `tier`: Enforcement tier (`diag`, `hybrid`, or `cap`).
- `event`: Event type (`init`, `check_fail`, etc.).
- `ptr`: Hex address of the pointer involved.
- `grade`: Snapshot of the grade record.
  - `base`: Base address of the allocation.
  - `end`: End address of the allocation.
  - `alloc_id`: Unique allocation ID.
  - `epoch`: Re-use epoch.
  - `perms`: Permission bitmask.
  - `prov_tag`: Provenance tag.
  - `alias_tok`: Aliasing token.
  - `flags`: Status flags.
- `fail`: (Optional) Failure details.
  - `component`: The grade component that failed (`bounds`, `life`, etc.).
  - `detail`: Human-readable error message.
- `site`: (Optional, usually added by compiler) Source location.
  - `file`: Source file name.
  - `line`: Line number.
  - `fn`: Function name.
  - `llvm`: LLVM value string.
