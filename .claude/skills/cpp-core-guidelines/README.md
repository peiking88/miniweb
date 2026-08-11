# cpp-core-guidelines-skills

A practical **C++ code review / walkthrough** toolkit: review workflow + checklists + rule mapping, organized around the *C++ Core Guidelines* and split into chapter-based references.

- Review entry (skill definition): [SKILL.md](./SKILL.md)
- Review checklist & adoption notes (condensed): [references/C++-Core-Guidelines.md](./references/C++-Core-Guidelines.md)
- Chapter notes: [`references/`](./references/)

## Review Summary (ready-to-use framework)

### 1) Severity levels (recommended)

- **P0 (fix first)**: May cause UB/out-of-bounds/dangling/lifetime bugs/resource leaks/data races/deadlocks/crashes/security issues.
- **P1 (high risk)**: Likely logic bugs, unclear exception/thread-safety guarantees, major maintainability risks.
- **P2 (improvement)**: Consistency, readability, naming/layout, duplication, etc.

### 2) Workflow

1. **Define scope & goals**: files/modules/functions, business intent, constraints (perf hotspots, ABI, platform, etc.).
2. **Risk-driven prioritization**: scan P0 risks first (Lifetime/Bounds/Type safety/concurrency).
3. **Quick chapter scan** (suggested order): P → I → F → C → Enum → R → ES → Per → CP → E → Con → T → CPL → SF → SL → NR → RF → Pro → GSL → NL.
4. **Each finding must include**: location, severity (P0/P1/P2), guideline id & title (e.g. F.16), evidence, impact, fix suggestion, and whether it can be automated.
5. **Produce an action list**: quick wins (one iteration), open questions/assumptions, and follow-up refactoring plan.

### 3) Key checkpoints (checklist excerpt)

- **UB / Lifetime / Bounds**: returning references/pointers to locals, dangling `string_view/span`, out-of-bounds access, strict-aliasing/type-punning issues.
- **Resources & ownership (R)**: avoid explicit `new/delete`; paired APIs (`fopen/fclose`, `lock/unlock`) must be RAII-wrapped; `T*` members must state owning vs observing.
- **Interface semantics (I/F)**: make ownership/nullability/range/exception guarantees/thread-safety explicit; avoid “clever” APIs.
- **Concurrency (CP)**: avoid data races/deadlocks; control thread lifetime (be careful with detach); avoid dangling refs across coroutine suspend points.
- **Error handling (E)**: keep exception vs error-code strategy consistent; avoid global state (e.g. `errno`) as the primary error channel; ensure exception safety.
- **Performance (Per)**: never use UB/data races as “optimizations”; prefer correctness & clarity; optimize hotspots with evidence.

### 4) Tooling & gates (CI recommendations)

- **clang-tidy**: gradually enable `cppcoreguidelines-*`, `modernize-*`, `bugprone-*`, `performance-*`.
- **Sanitizers**: ASan/UBSan by default; TSan periodically for concurrent modules.
- **Gating**: new code must satisfy the core ruleset; legacy code follows the “boy-scout rule” (clean up what you touch).

### 5) Exceptions (template)

When you must violate a rule, record the following in review/docs:

- Rule id (e.g. R.11)
- Reason (perf hotspot/ABI/C API/platform constraint)
- Risk assessment (leaks/exception safety/concurrency impact)
- Mitigations (RAII wrapper, unit tests, sanitizer coverage, explicit annotations)
- Exit plan (replacement path and timeline)

## references/ Index

- [C++-Core-Guidelines.md](./references/C++-Core-Guidelines.md) (adoption principles + master checklist)
- [P-Philosophy.md](./references/P-Philosophy.md)
- [I-Interfaces.md](./references/I-Interfaces.md)
- [F-Functions.md](./references/F-Functions.md)
- [C-Classes-and-class-hierarchies.md](./references/C-Classes-and-class-hierarchies.md)
- [Enum-Enumerations.md](./references/Enum-Enumerations.md)
- [R-Resource-management.md](./references/R-Resource-management.md)
- [ES-Expressions-and-statements.md](./references/ES-Expressions-and-statements.md)
- [Per-Performance.md](./references/Per-Performance.md)
- [CP-Concurrency-and-parallelism.md](./references/CP-Concurrency-and-parallelism.md)
- [E-Error-handling.md](./references/E-Error-handling.md)
- [Con-Constants-and-immutability.md](./references/Con-Constants-and-immutability.md)
- [T-Templates-and-generic-programming.md](./references/T-Templates-and-generic-programming.md)
- [CPL-C-style-programming.md](./references/CPL-C-style-programming.md)
- [SF-Source-files.md](./references/SF-Source-files.md)
- [SL-The-Standard-Library.md](./references/SL-The-Standard-Library.md)
- [NR-Non-Rules-and-myths.md](./references/NR-Non-Rules-and-myths.md)
- [RF-References.md](./references/RF-References.md)
- [Pro-Profiles.md](./references/Pro-Profiles.md)
- [GSL-Guidelines-support-library.md](./references/GSL-Guidelines-support-library.md)
- [NL-Naming-and-layout-suggestions.md](./references/NL-Naming-and-layout-suggestions.md)
