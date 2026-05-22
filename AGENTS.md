# AGENTS.md — OCCT-Light

This file is the canonical guide for AI coding assistants — Claude Code, GitHub Copilot, OpenCode, Codex CLI, Cursor, Sourcegraph Amp, and any other agent that reads `AGENTS.md`. Editor-specific files (`CLAUDE.md`, `.github/copilot-instructions.md`) reference back to this one.

If you are a human reader, this is a fast tour of the project; the long-form is in [`docs/design/`](docs/design/).

---

## What this project is

**OCCT-Light** is a modular **C-ABI wrapper** around [Open CASCADE Technology](https://dev.opencascade.org/), designed as the canonical bridge for Python, C#, JS/TS, WASM, Rust, Go, Java, and any language with a C foreign-function interface. Cross-language reach is the entire point.

Two layered surfaces:

1. **Public C ABI** (`include/occtl/*.h`) — `extern "C"`, opaque handles, POD structs, status codes. **No STL or OCCT types ever appear in public headers.** Every binding language consumes these directly.
2. **Header-only C++ veneer** (`include/occtl-hpp/*.hpp`) — RAII handles, exceptions translated from status codes. `#include`-only, links nothing of its own.

The internal C++ implementation (`src/`) calls OCCT directly and is never exported.

**Topology.** OCCT-Light uses **BRepGraph** (OCCT's modern incidence-table topology DAG) as the canonical shape representation. `TopoDS_*` is internal-only — used as a round-trip target for algorithms that don't yet have BRepGraph-native variants (Booleans, fillets), invisible to ABI consumers.

---

## Read these before changing anything

In order:

1. [`docs/design/ARCHITECTURE.md`](docs/design/ARCHITECTURE.md) — goals, layering, build, threading, versioning.
2. [`docs/design/ABI_PATTERNS.md`](docs/design/ABI_PATTERNS.md) — the ABI rulebook: handles, errors, strings, sequences, options, allocators, callbacks.
3. [`docs/design/BREPGRAPH_AS_CANONICAL.md`](docs/design/BREPGRAPH_AS_CANONICAL.md) — why BRepGraph not TopoDS, identity model, TopoDS round-trip protocol.
4. [`docs/design/MODULES.md`](docs/design/MODULES.md) — per-module charter and dependencies.
5. [`docs/design/CODING_STYLE.md`](docs/design/CODING_STYLE.md) — public-C / internal-C++ / veneer / test style split, Doxygen rules.
6. [`docs/design/BINDINGS.md`](docs/design/BINDINGS.md) — binding strategy: the uniform two-layer recipe (auto-generated raw FFI + hand-written idiomatic facade) for Python, C#, JS/TS, and future languages; load-time ABI handshake; error / handle / iterator / span translation contract; packaging; the §8 checklist for adding a language.

When the design docs and your training intuitions disagree, **the design docs win.** If you think a doc is wrong, propose an edit; don't quietly violate it.

---

## Hard rules (will fail review)

These are the rules that cause the most damage when violated, listed up front so you can't miss them.

1. **No OCCT or STL types in `include/occtl/*.h`.** No `Handle(...)`, no `TopoDS_*`, no `Standard_*`, no `NCollection_*`, no `<string>`, no `<vector>`. Public C headers `#include` only `<stdint.h>`, `<stddef.h>`, and other `occtl_*.h` files. CI greps for this.
2. **Delegate all computation to OCCT inside `src/`.** Arithmetic, geometric, and topological work must go through OCCT types — `gp_*`, `BRep_Tool`, `BRepLib_*`, `BRepAlgoAPI_*`, `BRepMesh_*`, etc. — rather than being reimplemented in C++17. The implementation pattern is: convert POD ABI types → OCCT types at the entry point, call OCCT operations, convert back. See `src/geom/GeomMath.hxx` (`ToGp`/`FromGp` helpers) and `src/geom/extern_c.cxx` for the canonical example. Hand-rolled math is a maintenance liability and a drift from OCCT's tested, numerically robust code. Violations will be rejected in review.
3. **No `TopoDS_*` mentioned anywhere in the public C surface or the C++ veneer.** Internal-only. The wrapper round-trips through TopoDS invisibly — users never know.
4. **No exceptions across `extern "C"`.** Every entry point is wrapped in `OcctL::Core::Guard` which catches `Standard_Failure`, `std::exception`, and `...` and translates them to a status code.
5. **No OCAF / XCAF code.** Not in public headers, not in module dependencies, not in tests. STEP/IGES privately link a minimal OCAF subset, walled off; no symbol leaks. See `MODULES.md` §20.
6. **`bool` is forbidden in public C signatures.** Use `int32_t` (0/1). The C++ veneer takes `bool` and converts.
7. **No SWIG.** The C ABI is the binding interface; per-language facades use cffi / P/Invoke / N-API / Embind / bindgen / cgo.
8. **Every public function has a Doxygen block** with `@param[in/out]` (with **owns it / borrows it** for pointers), `@retval` for every code it can return, `@threadsafe`, and `@sa`. See `CODING_STYLE.md` §2.2.
9. **Every options/info struct carries `uint32_t struct_version` first and `const void* p_next` second.** Versioning is non-negotiable; we got it from Vulkan and libgit2 because it works.
10. **Don't create new design docs without asking.** The six in `docs/design/` are the design surface; new docs fragment intent. Edit the existing ones.
11. **Don't add features the user didn't request.** If you're "improving" while fixing a bug, stop. Three similar lines beat a premature abstraction. (See [the OCCT CLAUDE.md](../OCCT/CLAUDE.md) on minimalism — same rule applies here.)

---

## Style at a glance

Two style worlds, separated by the `extern "C"` boundary. Full rules live in [`docs/design/CODING_STYLE.md`](docs/design/CODING_STYLE.md):

| Surface | Style | §  |
| --- | --- | --- |
| Public C ABI (`include/occtl/*.h`) | `lower_snake_case`, `occtl_*` prefix, full Doxygen | §2 |
| C++ veneer (`include/occtl-hpp/*.hpp`) | STL-shaped API; OCCT identifier naming inside (`theParam` / `aLocal` / `myField`) | §3 |
| Internal C++ (`src/**/*.cxx,.hxx`) | OCCT style: `the/a/my/THE_`, `//===` separators (98 `=`), no `auto` for typing, `const` on unmodified value params | §4 |
| Tests (`tests/**/*.cpp`) | gtest, no separators, `a<Local>` naming, `<Method>_<Scenario>_<Expected>` test names | §5 |
| CMake | lowercase, `target_*` over directory-level | §6 |

---

## Build, test, and verify

Build directory convention: `build/<preset>/`. Presets: `core-only`, `geom-only`, `minimal`, `cad`, `full`, `full-with-viz` (see [`docs/design/ARCHITECTURE.md §6`](docs/design/ARCHITECTURE.md)). Any preset other than `core-only` needs `-DOCCT_DIR=/path/to/opencascade/lib/cmake/opencascade`.

```bash
cmake --preset minimal -DOCCT_DIR=/…/lib/cmake/opencascade
cmake --build --preset minimal
ctest --preset minimal               # add -L <module> for one module's tests
```

After changes:

1. Build the affected preset cleanly; `ctest` must be all-green.
2. If you added a public function, run `grep -RE '#include[[:space:]]+<' include/occtl/` and confirm only `<stdint.h>`, `<stddef.h>`, and `"occtl_*.h"` appear.
3. If you added or changed a public symbol, update `include/occtl-hpp/<module>.hpp` and the matching tests.

---

## Common tasks

### Adding a public function

1. Declare in `include/occtl/occtl_<module>.h`. **Full Doxygen** with all the tags listed in `CODING_STYLE.md` §2.2. Pointer params get `[in] / [out] / [inout]` plus an explicit "owns it" or "borrows it" tag; **value-typed params get `@param[in]` only** — no ownership tag (the type owns itself).
2. Implement the `extern "C"` shim in `src/<module>/extern_c.cxx`, wrapping the body in `OcctL::Core::Guard`.
3. Add a method to the C++ veneer at `include/occtl-hpp/<module>.hpp` that calls the C function and translates errors via `occtl::check`.
4. Add a gtest covering at least: success path, NULL-arg rejection, version-mismatch (if applicable), and one realistic failure. Status-returning functions should also have one test that asserts `occtl_error_last()->message` is non-empty after a failure.

### Adding a new module

1. Read `MODULES.md` to confirm the module is in scope and to copy the dependency declaration.
2. Add `option(OCCTL_BUILD_<MODULE> "..." OFF)` to `cmake/OCCTLOptions.cmake`.
3. Add the public header `include/occtl/occtl_<module>.h` with Doxygen, gated by `OCCTL_HAS_<MODULE>` in the umbrella.
4. Create `src/<module>/CMakeLists.txt` calling `occtl_add_module(<module> SOURCES ... PRIVATE_LINK ...)`.
5. Add `add_subdirectory(src/<module>)` (gated on the option) to the top-level `CMakeLists.txt`.
6. Add `tests/<module>/CMakeLists.txt` and at least a smoke test.
7. Update `MODULES.md` if the actual scope diverged from the planned scope.

### Adding a new public type or enum

1. Define in the relevant `occtl_<module>.h` with Doxygen.
2. Reserve `_RESERVED_FUTURE = 0x7fffffff` for enums.
3. For options structs: `uint32_t struct_version` first, `const void* p_next` second, `*_VERSION_1` macro and `*_INIT` static initializer plus a runtime `*_init` function.
4. Add a veneer wrapper in the C++ veneer.

### Adding a new public function that takes/returns a sequence

Follow the menu in `ABI_PATTERNS.md` §10:

- Unknown size, immutable snapshot → **two-call buffer pattern** (NULL/size sizing call, then refill).
- Bulk numeric data, hot path → **span/view** (zero-copy pointer + count, lifetime tied to parent).
- Heterogeneous trees → **opaque iterator** with `_create / _next / _free`.
- Visitor callback as alternative for the same data.

Don't invent a new pattern. The four shapes cover everything.

---

## Topology — read this before touching `topo`

OCCT-Light's topology is **BRepGraph**, not `TopoDS`. The public type is `occtl_graph_t*`; identity is `occtl_node_id_t` (transient) and `occtl_uid_t` (persistent across `Compact()`). Algorithms that internally need `TopoDS` (Booleans, fillets, offsets) round-trip via `BRepGraph::Shapes().Shape(...)` + `BRepGraph_Builder::Add(...)` — invisible to ABI callers. Color / name metadata lives on BRepGraph layers, never OCAF/XCAF.

Full design and TopoDS round-trip protocol: [`docs/design/BREPGRAPH_AS_CANONICAL.md`](docs/design/BREPGRAPH_AS_CANONICAL.md). Per-module charter (what's shipped vs pending): [`docs/design/MODULES.md §5`](docs/design/MODULES.md).

---

## Bindings

| Lang | FFI mechanism |
| --- | --- |
| Python | cffi (ABI mode) |
| C# | Roslyn source generator + `[LibraryImport]`, .NET 8+ |
| Node | N-API + `node-addon-api` |
| WASM | Emscripten + Embind |
| Java | JNA + Maven |

Per-language facades live under `bindings/<lang>/`. Rust, Go, Java, and Swift are still evolving. **No SWIG** (hard rule #7).

**Coverage is 1:1.** Every binding wraps every `OCCTL_API` symbol declared in `include/occtl/*.h`. The Tier-3 symbol-coverage test in each binding (`test_symbol_coverage.py`, `SymbolCoverageTests.cs`, `tests/coverage.test.ts`, `SymbolCoverageTest.java`) fails the build when an idiomatic wrapper is missing, so additions to the C ABI cannot land without propagating to every language.

**Architecture.** Each facade is two layers: an **auto-generated raw FFI** that is 1:1 with `include/occtl/*.h` (consumes `build/abi.json` produced by `tools/abi_dump.py`) — **private**; plus a **hand-written idiomatic layer** that is the binding's stable surface. Every facade implements the same ten-step contract (ABI version handshake → error translation → RAII handles → strong nominal IDs → versioned options → native iterators → zero-copy spans → UTF-8 strings → callback pinning → threading).

**Parity grid.** `tests/binding_parity/` carries shared scenarios. `ctest -L parity` runs every enabled binding's runner against every scenario; equivalent runs in Python, C#, Node, WASM, and Java must produce matching output.

Full plan, per-language layouts, packaging, parity-test grid, and the checklist for a new language: [`docs/design/BINDINGS.md`](docs/design/BINDINGS.md). Quick index of the language/approach pairs: [`docs/design/ABI_PATTERNS.md §17`](docs/design/ABI_PATTERNS.md).

---

## Things you might want to do but should ask first

These look fine in isolation but tend to drift the design:

- Adding any new top-level directory.
- Adding or removing a module (talk it through against `MODULES.md` first).
- Bumping `OCCTL_ABI_VERSION` (only on hard breakage).
- Bumping the C++ standard above C++17 in implementation files.
- Removing or renaming any public symbol.
- Adding a build-system dependency (TBB, Boost, anything new).
- Touching the AGPL license file.

Things you should **not** ask, just do (when appropriate):

- Fix a typo or clear bug.
- Add a missing test.
- Improve a Doxygen block to match `CODING_STYLE.md`.
- Reformat to match `.clang-format`.
- Update outdated paths or commands in docs.

---

## License

[AGPL-3.0-or-later](LICENSE_AGPL_30.txt). Patches must be license-compatible. OCCT itself is LGPL-2.1; OCCT-Light links it as an external dependency without bundling.
