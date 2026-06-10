# GOIR Metrics Schema

The GOIR runtime collects performance and safety metrics during execution. These are emitted as a JSON object upon program termination or a safety trap.

## Environment Variables

- `GOIR_METRICS_OUT`: Path to the file where metrics should be written. If unset, metrics are written to `stderr`.

## Metrics JSON Object

| Field | Description |
|---|---|
| `check_load` | Number of `__go_check_load` calls executed. |
| `check_store` | Number of `__go_check_store` calls executed. |
| `check_free` | Number of `__go_check_free` calls executed. |
| `trap_bounds` | Number of spatial safety traps triggered. |
| `trap_life` | Number of temporal safety traps triggered. |
| `trap_perms` | Number of authorization traps triggered. |
| `trap_prov` | Number of provenance traps triggered. |
| `shadow_store` | Number of metadata stores to the shadow table. |
| `shadow_load` | Number of metadata loads from the shadow table. |
| `alloc` | Number of allocations (`malloc`, `realloc`) wrapped by GOIR. |
| `missing_grade` | Number of times a missing grade was encountered, forcing a fallback to `TOP`. |

## Example

```json
{
  "check_load": 1240,
  "check_store": 850,
  "check_free": 10,
  "trap_bounds": 0,
  "trap_life": 0,
  "trap_perms": 0,
  "trap_prov": 0,
  "shadow_store": 45,
  "shadow_load": 120,
  "alloc": 15,
  "missing_grade": 2
}
```
