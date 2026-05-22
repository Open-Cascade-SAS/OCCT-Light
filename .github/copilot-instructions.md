# GitHub Copilot Instructions — OCCT-Light

> Copilot does not follow `@import`-style references; this file repeats the must-know rules concretely. The full guidance lives in [`AGENTS.md`](../AGENTS.md) and [`docs/design/`](../docs/design/).

## What this project is

**OCCT-Light** is a modular C-ABI wrapper around [Open CASCADE Technology](https://dev.opencascade.org/), built so that Python, C#, JS/TS, WASM, Rust, Go, and any C-FFI language consume the same hand-written ABI rather than re-translating C++ types per binding.

Two surfaces:

- **Public C ABI** — `include/occtl/*.h`, `extern "C"`, opaque handles, POD structs, status codes. **No STL or OCCT types** in these headers.
- **Header-only C++ veneer** — `include/occtl-hpp/*.hpp`, RAII handles, exceptions translated from status codes.

Topology canonical type is **BRepGraph** (OCCT's modern incidence-table topology). `TopoDS_*` is internal-only and never appears in the ABI.

## Hard rules

1. **Never include STL or OCCT headers from `include/occtl/*.h`.** Allowed: `<stdint.h>`, `<stddef.h>`, other `occtl_*.h` files. Nothing else.
2. **Never reference `TopoDS_*`, `Handle(...)`, `Standard_*`, `NCollection_*`** in any public C header or in the C++ veneer.
3. **Never let a C++ exception cross `extern "C"`.** Wrap every entry point body in `OcctL::Core::Guard`.
4. **No OCAF / XCAF.** Color and name metadata live on BRepGraph layers.
5. **Use `int32_t` for booleans** in public C signatures, never `bool` or `_Bool`.
6. **No SWIG.** Use cffi (Python), P/Invoke (C#), N-API + Embind (JS/TS), bindgen (Rust), cgo (Go).
7. **Every public function gets a Doxygen block** with `@param[in/out]` (annotated owns it / borrows it for pointers), `@retval` for every status code, `@threadsafe`, `@sa`.
8. **Options/info structs** start with `uint32_t struct_version` then `const void* p_next`, with both an `*_INIT` macro and a runtime `*_init` function.
9. **Don't add features the user didn't ask for.** No speculative abstractions. Three similar lines beat a premature helper.

## Style

| Surface | Style |
| --- | --- |
| Public C — `include/occtl/*.h` and the `extern "C"` shims | `lower_snake_case`, `occtl_*` prefix, C `/** */` Doxygen blocks |
| C++ veneer — `include/occtl-hpp/*.hpp` | `namespace occtl`, PascalCase types, `snake_case` methods, STL types, `///` Doxygen. **Identifier naming inside is OCCT**: `theParam`, `aLocal`, `myField` — same as internal C++. |
| Internal C++ — `src/**/*.{hxx,cxx}` | OCCT style: `theParam`, `aLocal`, `myField`, `THE_<NAME>` for TU-statics, `//===` separator (98 `=` after `//`) before every method, no archaeology comments, default to no comments |
| Tests — `tests/**/*.cpp` | gtest, no `//===` separators, `a<Local>` / `an<Local>` variable naming, `<Fixture>.<Method>_<Scenario>_<Expected>` test names |
| CMake — `CMakeLists.txt`, `cmake/*.cmake` | lowercase commands, `target_*` over directory-level, file lists alphabetized |

Internal C++ pattern:

```cpp
namespace OcctL::Core
{

class Runtime
{
public:
  static Runtime& Instance() noexcept;

  occtl_status_t Initialize (const occtl_runtime_init_info_t* theInfo) noexcept;
  void           SetParallel (const bool theFlag) noexcept;

private:
  bool myInitialised;
  bool myParallel;
};

}  // namespace OcctL::Core
```

`extern "C"` shim pattern:

```cpp
//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_runtime_init (const occtl_runtime_init_info_t* theInfo)
{
  return OcctL::Core::Guard ([&]() -> occtl_status_t {
    return OcctL::Core::Runtime::Instance().Initialize (theInfo);
  });
}
```

`auto` is allowed only for: lambda types, structured bindings, and verbose iterator types in range-for. Never to "save typing." Use explicit types everywhere else.

`const` on every value parameter that isn't modified, in both `.hxx` and `.cxx`.

Default to **no comments**. Add a comment only when the *why* is non-obvious. Never reference history ("was X", "previously…", "fixed bug Z"). Never write multi-paragraph comment blocks.

## Build

```bash
cmake --preset core-only             # foundation only, no OCCT needed
cmake --preset geom-only             # core + geom; while iterating on geom
cmake --preset minimal               # core + geom + topo + prim
cmake --preset cad                   # + bool + mesh + heal + io_brep + io_step + io_stl + de
cmake --preset full                  # everything except viz
cmake --preset full-with-viz         # + viz
```

Set `OCCT_DIR=/path/to/opencascade/lib/cmake/opencascade` for any preset above `core-only`.

```bash
cmake --build  --preset <preset>
ctest          --preset <preset>
```

## Topology — read this twice

OCCT-Light uses **BRepGraph**, not TopoDS, as the canonical topology.

- Public type: `occtl_graph_t*`. Identity: `occtl_node_id_t` (transient, invalidated by `Compact()`) and `occtl_uid_t` (persistent across compaction).
- Every algorithm takes graph + node id and returns graph + node id. **Never `TopoDS_Shape` in the ABI.**
- For algorithms that internally need TopoDS (Booleans, fillets, offsets), **round-trip invisibly**: get `BRepGraph::Shapes().Shape(...)`, run the OCCT algorithm, feed result back via `BRepGraph_Builder::Add(...)`. Document the round-trip in implementation comments only.
- Color/name use BRepGraph **Named Layers**. No OCAF, no XCAF.

## Documentation requirements

Every public C function needs a Doxygen block with all of:

```c
/**
 * One-sentence summary.
 *
 * Optional paragraph for nuance.
 *
 * @param[in]  arg     Borrows it / owns it. NULL semantics. Validation rules.
 * @param[out] out_x   Owns it (caller must free with #occtl_x_free). Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  When …
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes / No / qualified.
 *
 * @sa other_function, occtl_x_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_x_create(...);
```

Internal C++ (`.hxx`) uses OCCT-style `//!` Doxygen:

```cpp
//! Translates a caught C++ exception into a status code and populates the
//! thread-local error.
//! @param[in] theException pointer to the caught std::exception, or nullptr
//! @return the translated status code; never OCCTL_OK
occtl_status_t TranslateException (const std::exception* theException) noexcept;
```

## What NOT to suggest

- Adding a SWIG interface file.
- Returning `TopoDS_Shape` from any wrapper function.
- Including `<vector>` or `<string>` in a public C header.
- Adding an OCAF document (`TDocStd_Document`) anywhere.
- Replacing the status-code error model with exceptions across `extern "C"`.
- Adding `using namespace std;` anywhere.
- Generating Markdown documentation files unless asked.

## Where to look for context

- [`AGENTS.md`](../AGENTS.md) — the canonical cross-agent guide.
- [`docs/design/`](../docs/design/) — six design documents that define the project.
- [`docs/design/CODING_STYLE.md`](../docs/design/CODING_STYLE.md) — full style rules.
- [`docs/design/ABI_PATTERNS.md`](../docs/design/ABI_PATTERNS.md) — the ABI rulebook.
- [`docs/design/BREPGRAPH_AS_CANONICAL.md`](../docs/design/BREPGRAPH_AS_CANONICAL.md) — topology design.
- [`docs/design/MODULES.md`](../docs/design/MODULES.md) — module charter.
- [`docs/design/BINDINGS.md`](../docs/design/BINDINGS.md) — per-language facade strategy.

When this file and a design doc disagree, the design doc wins.
