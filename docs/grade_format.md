# GOIR Grade Format and Trace Schema

## Grade Schema

A GOIR graded pointer value is conceptually:

`gptr τ  ≜  (addr : ptr τ, g : grade)`

The grade is a product of components aligned to known enforcement mechanisms:

| Component | Purpose | Typical backend realization | Notes |
|---|---|---|---|
| `G_bounds` | Spatial safety (within allocation/subobject bounds) | SoftBound-style base/end checks; CHERI bounds | Spatial checking is foundational for temporal checking assumptions ([SoftBound PLDI’09], [CETS ISMM’10]) |
| `G_life` | Temporal safety (allocated vs freed; epoch) | CETS-style alloc_id/epoch | CETS proves temporal safety under stated conditions ([CETS ISMM’10]) |
| `G_perms` | Authorization (read/write/free/exec) | CHERI permissions; software checks | Capability-backed in CHERI ([CHERI TR-947]) |
| `G_prov` | Provenance policy state (PNVI variants) | Runtime exposure sets; conservative “unknown provenance” | Must be policy-parameterized ([Memarian et al., POPL’19], [WG14 N2364]) |
| `G_alias` (optional) | Aliasing/ownership discipline | Optional SB-like tags; or “none” | If enabled, must be defined by a concrete model (often UB-to-trap) |

### Suggested concrete grade record (logical)

```text
grade ::= {
  bounds: { base:u64, end:u64, kind: BoundsKind },
  life:   { alloc_id:u32, epoch:u32, alive:bool },
  perms:  { r:bool, w:bool, f:bool, x:bool },
  prov:   { policy: ProvPolicy, tag:u32, exposed:bool },
  alias:  { model: AliasModel, token:u64 }   // optional / may be unused
}
```

## JSON Trace Schema (developer-facing)

Each event is one JSON object per line (JSONL). Required fields:

```json
{
  "ts": 1712530000,
  "tier": "diag|hybrid|cap",
  "policy": {"prov": "pnvi-plain|pnvi-ae-udi", "alias": "none|sb-like"},
  "event": "init|check_fail|prov_expose|inttoptr_resolve|memcpy_taint",
  "site": {"file": "t.c", "line": 42, "fn": "foo", "llvm": "%12"},
  "ptr": "0x7f12abcd...",
  "grade": {"base":"0x..","end":"0x..","alloc_id":1,"epoch":3,"perms":"RW","prov_tag":7,"flags":123},
  "fail": {"component":"bounds|life|perms|prov|alias|unknown", "detail":"..."}
}
```
