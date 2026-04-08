# Task Status: exec_note

## Overview
- **Status**: Completed
- **Metrics**:
  - Effort (PD): 0.5
  - Acceptance Criteria Met: Yes

## Outputs
- `docs/status/template.md`
- `docs/status/exec_note.md`

## Verification Results
- Topological sort performed manually based on task dependencies.
- Template created and verified.

## Failures & Root Cause Analysis
- None.

## Prompt Parameter Updates
- Target C dialect, OS/ABI, and hardware remain OPEN-ENDED.

## Merge Status
- Merged to main: Yes

---

# Executive Orchestration Note: Topological Execution Order

This section establishes the topological execution order for the GOIR project tasks.

## Topological Sort

1.  **exec_note**: Executive orchestration note for autonomous agents. [Status: Completed]
2.  **fnd_scope_baseline**: Project charter: scope, goals, UB-to-trap stance, policy knobs. [Status: Completed]
3.  **fnd_refs_bundle**: Bundle canonical references and local dev notes. [Status: Completed]
4.  **fnd_repo_scaffold**: Repo scaffolding, build system, toolchain pin strategy. [Status: Completed]
5.  **fnd_goir_spec**: GOIR specification: syntax, type system, grade algebra, lowering strategy. [Status: Completed]
6.  **fnd_grade_format**: Grade record layout, serialization, and trace schema. [Status: Completed]
7.  **diag_clang_flag**: Add -fgraded-optics={diag,hybrid,cap} flag plumbing and module markers.
8.  **diag_intrinsics_or_abi**: Define llvm.go.* intrinsics or stable runtime call ABI.
9.  **diag_runtime_skeleton**: Implement libgoirrt (diag tier): checks, traps, minimal alloc wrappers, trace hooks.
10. **fnd_microbench_suite**: C/LLVM microtests suite for bounds, lifetime, provenance, memcpy, alias edge cases.
11. **diag_go_init_pass_template**: Template A: Implement GoInitPass.
12. **diag_trace_tooling**: Trace logging + minimal viewer (JSONL).
13. **hyb_shadow_design**: Disjoint metadata (shadow) design for pointer grades in memory.
14. **eval_metrics_collector**: Unified metrics collection.
15. **fnd_core_calculus_refmodel**: Core calculus + executable refmodel.
16. **fnd_law_templates**: Formal lifting laws and invariants checklist.
17. **diag_go_propagate_pass_template**: Template B: Implement GoPropagatePass.
18. **diag_go_check_insert_pass_template**: Template C: Implement GoCheckInsertPass.
19. **diag_mem_intrinsic_stub**: Stub handling for llvm.memcpy/memmove/memset in diag tier.
20. **diag_asan_interop**: Define and test interop with AddressSanitizer (ASan) in diag tier.
21. **diag_end_to_end_smoke**: End-to-end smoke: compile/run microc suite in diag tier.
22. **hyb_meta_load_store**: Compiler+runtime integration: store/load pointer grades through memory.
23. **hyb_bounds_impl**: Implement spatial bounds (G_bounds) propagation + checks.
24. **hyb_lifetime_cets**: Implement temporal safety (G_life) via CETS-style alloc_id/epoch.
25. **hyb_memcpy_typed**: Typed/untyped memcpy policy with tainting and pointer-layout metadata preservation.
26. **hyb_provenance_modules**: Implement provenance policy modules for ptrtoint/inttoptr.
27. **hyb_fastpath_perf**: Hybrid performance optimization: fast-path checks, caching, inlining, counters.
28. **hyb_user_docs**: Hybrid-tier docs: workflows, annotations, troubleshooting, known limitations.
29. **eval_spec_juliet_harness_template**: Template F: SPEC/Juliet harness + automated evaluation pipeline.
30. **eval_differential_testing**: Differential testing: baseline vs GOIR vs ASan.
31. **hyb_softbound_cets_interop**: Interop/compat mode with existing SoftBound/CETS builds.
32. **cap_toolchain_setup**: Capability backend setup (CHERI/Morello environment).
33. **eval_ci_matrix**: CI matrix: lit/unit/microc by tier; optional ASan and CHERI smoke.
34. **eval_realworld_ports_hybrid**: Hybrid-tier pilot ports on 1–2 real-world C codebases.
35. **eval_migration_playbook**: Migration playbook: tiers, workflows, policy decisions, and coding guidelines.
36. **eval_release_packaging**: Release engineering: packaging passes/runtime/tools and ABI/versioning policy.
37. **cap_grade_mapping**: Define grade↔capability mapping and invariants.
38. **cap_go_lower_pass_template**: Template D: Implement GoLowerPass for capability targets.
39. **cap_check_elision_policy**: Formalize and test check-elision rules under capability enforcement.
40. **cap_abi_shims**: ABI boundary shims for mixed capability/hybrid components.
41. **cap_runtime_portability**: Port/libgoirrt adaptations for capability targets.
42. **cap_port_pilot**: Capability-backed pilot on a real-world codebase.
43. **ver_proof_stack**: Set up proof toolchain (Coq/Lean/Iris) and CI hook.
44. **ver_formalize_calculus**: Formalize GOIR core calculus syntax + small-step semantics.
45. **ver_grade_algebra_lemmas**: Prove grade algebra properties needed by passes.
46. **ver_type_system**: Formalize typing and ‘progress-to-trap’ safety.
47. **ver_refinement_theorem_template**: Template E: Coq/Lean proof skeleton for refinement.
48. **ver_backend_correspondence**: Backend correspondence: hybrid runtime implements abstract GOIR semantics.
49. **ver_cap_monotonicity_plan**: Capability backend proof plan.
