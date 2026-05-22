# OCCT-Light — Architecture

## 1. What this document is

This is the top-level architecture for **OCCT-Light**, a C-ABI wrapper around [Open CASCADE Technology](https://dev.opencascade.org/). It exists so that Python, C#, JS/TS, WASM, Rust, Go, Swift, and any other language with a C foreign-function interface can drive OCCT through a single, stable, language-agnostic surface — without re-translating C++ types per binding. Before the first release, ABI-breaking cleanups are allowed when they improve the final headless CAD interface.

For day-to-day rules, read the companions:

- **[ABI_PATTERNS.md](./ABI_PATTERNS.md)** — handle, error, string, sequence, options, callback, and C++ veneer conventions.
- **[BREPGRAPH_AS_CANONICAL.md](./BREPGRAPH_AS_CANONICAL.md)** — why BRepGraph (not TopoDS) is the topology in the public API, how IDs are exposed, and the internal TopoDS round-trip protocol.
- **[MODULES.md](./MODULES.md)** — per-module charter: what each `OCCTL_BUILD_*` option covers, which OCCT toolkits it needs, and module-to-module dependencies.
- **[BINDINGS.md](./BINDINGS.md)** — binding strategy: the uniform recipe every per-language facade (Python, C#, JS/TS, Rust, Go, Swift) follows over the C ABI, packaging, and the checklist for adding a new language.

## 2. Goals and non-goals

### Goals

1. **One stable C ABI**, hand-written and reviewed, that every binding consumes directly. Not generated from C++.
2. **No OCCT or STL types in public headers** — ever. The public surface depends on `<stdint.h>` and `<stddef.h>` only.
3. **BRepGraph as canonical topology**, with `TopoDS_*` confined to internal round-trips for algorithms that don't yet have graph-native variants.
4. **Modular at CMake configuration time**: per-subsystem opt-in (`OCCTL_BUILD_<MODULE>`); core is always linked, everything else is optional.
5. **Header-only C++ veneer** (`occtl-hpp`) so C++ consumers get RAII and exceptions for free without any extra linkage.
6. **Multi-language by design**: Python (cffi), C# (P/Invoke), JS/TS (N-API + Emscripten), Rust (bindgen), all driven from the same C headers without per-language hand-written glue beyond ergonomic facades.

### Non-goals

- Feature parity with the entirety of OCCT. We expose what's needed for programmatic CAD work and grow from there.
- OCAF and XCAF integration. These are deferred; the wrapper does **not** depend on `TKCAF`, `TKLCAF`, `TKCDF`, `TKXCAF`, `TKBin*`, `TKXml*`, `TKStd*`, or `TKTObj*` for any module shipped in v1.
- Source compatibility with the existing OCCT-Light C++ MVP. The MVP is a useful precedent; the new ABI is a fresh design.
- Hiding BRepGraph concepts that have intrinsic value (Definition vs Usage, CoEdge half-edges, persistent UIDs). We document them, we don't paper over them.

## 3. ABI shape

OCCT-Light uses a stable C-ABI shape that is practical for every binding language:
- versioned options/info structs (`struct_version`, `p_next`);
- explicit status returns and thread-local error state;
- out-parameter-first signatures;
- opaque handles plus explicit free functions;
- append-only enum evolution and reserved future values.

## 4. Layering

```
┌──────────────────────────────────────────────────────────┐
│  Bindings: cffi, P/Invoke, N-API, embind, …              │
├──────────────────────────────────────────────────────────┤
│  occtl-hpp     (header-only C++ veneer; RAII, exceptions)│
├──────────────────────────────────────────────────────────┤
│  occtl         (pure C ABI — the only stable surface)    │
├──────────────────────────────────────────────────────────┤
│  occtl_internal  (C++17 implementation; not exported)    │
├──────────────────────────────────────────────────────────┤
│  OCCT (BRepGraph, BRep, Geom, BRepAlgoAPI, BRepMesh, …)  │
└──────────────────────────────────────────────────────────┘
```

**Hard rules between layers:**

- Public C headers (`include/occtl/*.h`) include `<stdint.h>`, `<stddef.h>`, and other `occtl_*.h`. Nothing else. Enforced by a CI grep.
- `occtl-hpp` is `#include`-only. It links nothing of its own; it just wraps the C ABI. Mismatch with the C headers is a build failure.
- `occtl_internal` is C++17. It freely uses OCCT, STL, NCollection, exceptions. Its symbols are hidden; nothing in `src/` is `OCCTL_API`-marked. **The golden rule for all modules that link OCCT: delegate every computation to OCCT types. Convert POD ABI structs to `gp_*` / `BRep_Tool` / `BRepAlgoAPI_*` types at the function entry point, call OCCT, convert the result back. Thin ToGp/FromGp conversion helpers are the canonical bridge — see `src/geom/GeomMath.hxx`. Reimplementing OCCT math in C++17 is explicitly forbidden.**
- OCCT itself is linked privately through the enabled module object libraries into the single feature-set library. Consumers link only the selected `OCCTL::occtl` / component compatibility targets.

## 5. Repository layout

```
include/occtl/        public C headers (extern "C", umbrella occtl.h)
include/occtl-hpp/    header-only C++ veneer (umbrella occtl.hpp)
src/<module>/         internal C++ implementation; never exported
tests/<module>/       gtest C++ per module
bindings/<lang>/      per-language facades (python, csharp, rust, node, wasm)
docs/design/          the six documents
cmake/                module helpers, find scripts, package config
CMakePresets.json     minimal / cad / full / full-with-viz / core-only / geom-only
```

Implemented modules own `src/<module>/`, `tests/<module>/`, and public headers under `include/occtl/` plus optional C++ veneer headers under `include/occtl-hpp/`. Planned modules stay in the registry as configure-time errors until their implementation lands.

## 6. Build model

- **CMake 3.23+**, C++17, Ninja by default.
- One physical library per configured feature set: `occtl-core`, `occtl-geom`, `occtl-minimal`, `occtl-cad`, `occtl-full`, and `occtl-full-viz`.
- Module targets (`OCCTL::core`, `OCCTL::topo`, etc.) are compatibility/component targets over the selected physical library, not separate DSOs/archives.
- `core` is implicit and always linked. Others toggle via `OCCTL_BUILD_<MODULE>=ON|OFF`. Defaults: `GEOM` and `TOPO` are `ON`; everything else is `OFF`.
- Module state, option names, source directories, binding feature names, and OCCT toolkit requirements live in `cmake/OCCTLRegistry.cmake`. The registry drives option creation, planned-module configure errors, OCCT toolkit discovery, enabled subdirectories, summary output, and installed feature metadata.
- Each enabled module installs its own public header(s); the umbrella `occtl.h` `#include`s only what's enabled.
- The build emits and installs `OCCTLFeatures.json` with the selected physical library name, enabled components, binding features, enabled C/C++ headers, ABI version, and OCCT toolkit list. Bindings use it together with `abi.json`.
- Presets in `CMakePresets.json`:
  - `core-only` — smallest; no OCCT required.
  - `geom-only` — `core + geom`; used while iterating on geom.
  - `minimal` — `core + geom + topo + prim`.
  - `cad` — minimal + `bool + mesh + heal + io_brep + io_step + io_stl + de`.
  - `full` — every implemented module except `viz`.
  - `full-with-viz` — full + `viz` (heavy OS deps).
- OCCT is a private implementation dependency for shared builds. Static installs expose the required OCCT toolkit targets through CMake's normal `LINK_ONLY` dependency model.

## 7. Versioning

| Surface | Mechanism |
| --- | --- |
| Library version | `OCCTL_VERSION_MAJOR/MINOR/PATCH` macros + `occtl_runtime_version(uint32_t* M, uint32_t* m, uint32_t* p)`. SemVer at the wrapper level, independent of OCCT version. |
| ABI version | `OCCTL_ABI_VERSION` integer macro and `occtl_runtime_abi_version(uint32_t*)`. Bumped only on hard breakage. |
| Struct evolution | Every `*_create_info_t` carries `uint32_t struct_version` as its first field, and a reserved `const void* p_next` for future extension chains. Initializer macros and runtime `*_init` functions both set the version. |
| Symbol evolution | New behavior ships under `_v2` symbols when the old symbol cannot be preserved bit-for-bit. Old symbols stay until the next major bump. |
| Enum evolution | Enums append values, never reorder. Each public enum carries `OCCTL_<ENUM>_RESERVED_FUTURE = 0x7fffffff` to force `int32_t` storage. |
| OCCT version coupling | Recorded in `occtl_runtime_occt_version()` for diagnostics; not part of the wrapper's ABI promise. |

The deprecation rule starts at the first release. During the pre-release phase, public symbols may be renamed, reshaped, or removed to keep the final API coherent; after the first release, nothing in the public surface silently changes behavior.

## 8. Threading model

- **Graphs are not thread-safe for mutation.** A graph is single-writer; readers are safe only after the writer publishes (the wrapper documents acquire/release semantics around `occtl_batch_commit`).
- **Concurrent reads of distinct graphs** are safe.
- **Last-error storage is thread-local.** Calling `occtl_error_last()` in thread A returns A's last error; never B's.
- **Cancellation and progress** are first-class: long-running calls (mesh, boolean, healing) accept an optional `occtl_progress_t*` token. The token is itself thread-safe (the canceller may live on a different thread from the worker).
- **OCCT internal threading** (TBB, parallel meshing) is enabled by default.

## 9. Error model (summary; details in ABI_PATTERNS)

Every public function returns `occtl_status_t`. `OCCTL_OK == 0` so `if (s) goto fail;` works naturally in C callers. On failure, a thread-local `occtl_error_t` carries:

```c
typedef struct occtl_error {
  occtl_status_t status;
  const char*    message;     // UTF-8, library-owned, valid until next call on this thread
  occtl_uid_t    source;      // optional UID of the offending object; zeroed if N/A
  uint32_t       extended;    // Extended subcode, optional
} occtl_error_t;
```

Exceptions never cross the C boundary. `OcctL::Core::Guard` catches `Standard_Failure` (root of all OCCT exceptions), `std::exception`, and `...`, translates each to a status code, and populates the thread-local. See [ABI_PATTERNS.md §5](./ABI_PATTERNS.md).

## 10. Memory model (summary; details in ABI_PATTERNS)

One allocator boundary at the ABI:

- **Caller-allocates** in the two-call sequence pattern (`fn(..., NULL, &n)` then allocate, refill).
- **Library-allocates** everything else. Every library-allocated object has a matching `occtl_*_free` (NULL-tolerant, idempotent).

## 11. Out-of-scope

The hard rules in `AGENTS.md` cover what cannot appear in public headers (OCCT/STL types, exceptions, OCAF/XCAF). Two further omissions worth naming explicitly: no header-leaked OCCT macros (`Standard_EXPORT`, `DEFINE_STANDARD_HANDLE`), and no Tcl/Draw integration (that stays in OCCT's test harness).

## 12. Implementation order

Implemented: `core`, `geom`, `topo`, `prim`, `text`, `bool`, `mesh`, `heal`, `io_brep`, `io_step`, `io_iges`, `io_stl`, `io_obj`, `io_gltf`, `io_vrml`, `io_ply`, `de`, `viz`, and the Python / C# / Node / WASM / Rust / Go / Java binding trees. Planned: Swift and deeper native-window smoke coverage on Linux/Windows.

Each module ships with gtest coverage and at least one binding-language smoke test before being declared complete.
