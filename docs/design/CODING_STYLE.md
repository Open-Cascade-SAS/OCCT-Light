# OCCT-Light — Coding Style and Documentation

> Two surfaces, two styles. The public **C ABI** uses idiomatic C conventions (snake_case, `occtl_*` prefix). The internal **C++ implementation** follows OCCT conventions (`theParam`, `aLocal`, `myField`, file separators). Both surfaces are aggressively documented.

Companion docs: [ARCHITECTURE.md](./ARCHITECTURE.md) · [ABI_PATTERNS.md](./ABI_PATTERNS.md) · [BREPGRAPH_AS_CANONICAL.md](./BREPGRAPH_AS_CANONICAL.md) · [MODULES.md](./MODULES.md) · [BINDINGS.md](./BINDINGS.md).

---

## 1. Why two styles

Public C is `lower_snake_case` because that's what C compilers and binding generators expect; internal C++ follows OCCT (`thePar` / `aLocal` / `myField`) so it reads the same way for OCCT contributors. The `extern "C"` shim is the boundary; the two guides never collide inside the same identifier.

---

## 2. Public C surface (`include/occtl/`)

### 2.1 Naming

| Element | Pattern | Example |
| --- | --- | --- |
| Function | `occtl_<noun>_<verb>` | `occtl_graph_create`, `occtl_error_clear` |
| Type | `occtl_<noun>_t` | `occtl_graph_t`, `occtl_status_t` |
| Enum value | `OCCTL_<DOMAIN>_<VALUE>` | `OCCTL_OK`, `OCCTL_KIND_FACE` |
| Macro | `OCCTL_<NAME>` | `OCCTL_API`, `OCCTL_VERSION_MAJOR` |
| Parameter | `lower_snake_case` | `out_graph`, `node_id`, `info` |
| Out parameter | prefixed `out_` | `out_graph`, `out_required` |

The full convention is in [ABI_PATTERNS.md §1](./ABI_PATTERNS.md#1-naming).

Constructor verbs are split between `_create_<variant>` (allocates an out-handle the caller owns) and `_make_<entity>` (mutates a graph and returns a borrowed node id, mirroring OCCT's `BRep*API_Make*` family). The two are not interchangeable; see [ABI_PATTERNS.md §1.1](./ABI_PATTERNS.md#11-constructor-verbs-_create-vs-_make) for the table and rationale.

### 2.2 Doxygen for every public symbol

Every public function, type, and enum carries a Doxygen block in C-style `/** */`:

```c
/**
 * Creates an empty topology graph.
 *
 * The returned graph is heap-allocated; the caller owns it and must
 * release it with #occtl_graph_free.  The graph starts with zero
 * entities and uses the BRepGraph default constructor.
 *
 * @param[out] out_graph Owns it.  Must be non-NULL.  On success
 *                       receives a valid handle (never NULL); on
 *                       failure set to NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_graph is NULL.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_graph_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_create(occtl_graph_t** out_graph);
```

Required tags on every public function:

| Tag | When |
| --- | --- |
| `@param[in]` / `@param[out]` / `@param[in,out]` | Every parameter. **Pointer params** carry `owns it` or `borrows it` plus NULL semantics. **Value-typed params** (POD structs and scalars passed by value) get `@param[in]` only — no ownership tag, no NULL clause; the value owns itself. |
| `@retval` | Every status code the function can return. List them all. |
| `@threadsafe` | `Yes`, `No`, or a one-line qualifier (e.g. `Concurrent reads of distinct graphs are safe`). |
| `@sa` | Related functions (`_free` mate, alternative call, etc.). |

**Versioned `*_init` setters** (the runtime initialisers paired with each `_INIT` static initialiser) carry the full block too — no brief-only one-liners. They take a single caller-allocated struct, write defaults into it, and return `void`. Required shape:

```c
/**
 * Runtime initialiser for #<info_type>.
 *
 * Sets all fields to #<MACRO>_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 */
OCCTL_API void OCCTL_CALL <info_type>_init(<info_type>* info);
```

`info` is `Borrows it` because the caller retains ownership of the memory the function writes into — the function does not allocate, retain, or free anything.

Worked example for a value-in / value-out function (no pointers, no status code, so no `@retval`):

```c
/**
 * Euclidean distance between two 3D points.
 *
 * @param[in] a  First point.
 * @param[in] b  Second point.
 *
 * @return Non-negative distance; zero when @c a == @c b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_point3_midpoint, occtl_vector3_magnitude
 */
OCCTL_API double OCCTL_CALL occtl_point3_distance(occtl_point3_t a, occtl_point3_t b);
```

Required tags on every public type:

```c
/**
 * Opaque handle to a topology graph.
 *
 * Created by #occtl_graph_create and released by #occtl_graph_free.
 * Concurrent mutation is not supported; concurrent reads of distinct
 * graphs are safe. See [BREPGRAPH_AS_CANONICAL.md] for the underlying
 * model and identity semantics.
 *
 * @threadsafe Single-writer; multiple-reader after publication.
 */
typedef struct occtl_graph occtl_graph_t;
```

Required tags on every public enum:

```c
/**
 * Status code returned by every public function.
 *
 * @c OCCTL_OK is always 0; @c OCCTL_FAILED(s) and @c OCCTL_SUCCEEDED(s)
 * macros are provided.
 *
 * New values may be appended in future versions; consumers must treat
 * unknown values as failures.
 */
typedef enum occtl_status {
  OCCTL_OK                = 0,  /**< Success. */
  OCCTL_ERROR             = 1,  /**< Generic failure; see #occtl_error_last. */
  /* … */
} occtl_status_t;
```

### 2.3 What goes in a docstring

- **What the function does** — one sentence first, then a paragraph if needed.
- **Lifetime contract** for every pointer (owns it / borrows it / valid until X).
- **Validation rules** — NULL semantics, version semantics, range expectations.
- **Every status code** the function can produce, with the condition that triggers it.
- **Threadsafety** — be specific. "Yes" or "No" alone is not enough if there's a subtlety.
- **Related calls** — the `_free` mate, the alternative shape (callback vs iterator), the `_v2` if applicable.

### 2.4 What does NOT go in a docstring

- Implementation notes ("uses BRepGraph::ShapesView internally") — those go in the .cpp file.
- Backstory or history ("was added in v0.3 because…") — those go in the changelog.
- Pseudocode — write a runnable example in `examples/` instead.

### 2.5 Header file layout

```c
/* SPDX-License-Identifier: AGPL-3.0-or-later */
/**
 * @file occtl_<module>.h
 * @brief OCCT-Light: <module> public API.
 *
 * <one-paragraph module overview>
 */

#ifndef OCCTL_<MODULE>_H
#define OCCTL_<MODULE>_H

#include <stdint.h>
#include <stddef.h>

#include "occtl_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* … */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_<MODULE>_H */
```

Headers use **no** section separator comments and no free comments. The only accepted comments are SPDX, Doxygen blocks/lines, and include-guard end comments. Each declaration or group stands on its own with its Doxygen documentation block.

**Forbidden in all headers:**
- Unicode box-drawing characters (`─`, `━`, `┄`, etc.) in comments — use plain ASCII only.
- `//===`, `// ---`, `/* ==== */` or any other separator decoration.
- "Free" comments (not Doxygen-attached to a declaration). Every comment block must be a `/// @brief` / `//! @brief` on the symbol it documents, or a `///<` / `//!<` trailing brief.

---

## 3. Public C++ veneer (`include/occtl-hpp/`)

The veneer mirrors the C ABI in modern, ergonomic C++. Identifier naming for **parameters, locals, and members follows OCCT** (`the*` / `a*` / `my*`) — same as the internal C++ — so the C++ identifier rules do not change when you cross from `src/` into `include/occtl-hpp/`. The visible STL-flavour comes from the *shape* of the API (lowercase namespace, `snake_case` methods, STL types, exceptions), not from the identifiers inside it.

| Element | Pattern | Example |
| --- | --- | --- |
| Namespace | `occtl::<sub>` (lowercase, distinct from internal `OcctL::*`) | `occtl::core`, `occtl::topo` |
| Class | `PascalCase` | `Graph`, `Error`, `NodeId` |
| Public method | `snake_case` (STL-shape) | `graph.face_count()`, `node.kind()` |
| Free function | `snake_case` | `version()`, `abi_version()` |
| Method parameter | `the<Name>` | `theInfo`, `theEnabled`, `theStatus` |
| Local variable | `a<Name>` / `an<Name>` | `aMajor`, `anError`, `aHandle` |
| Class member | `my<Name>` | `myInitialised`, `myCode` |
| Constant | `THE_<NAME>` (TU-static) or `constexpr` | `THE_DEFAULT_TIMEOUT` |

The veneer uses STL freely: `std::span`, `std::string_view`, `std::optional`, exceptions. Headers are `.hpp`. Doxygen blocks use `///` or `/** */` consistently per file.

The veneer never re-documents C ABI semantics — it `@see`-references the C entry point.

**Forbidden in veneer headers:**
- Unicode box-drawing characters in any comment.
- `//===` separators or any other ASCII-art section dividers. Veneer headers use no separators (same rule as public C headers).
- "Free" comments not Doxygen-attached to a class, method, or member. Every `//` comment must be `/// @brief` on the symbol it documents, or `///<` trailing on the same line.

**Why the split.** Method names in the veneer are `snake_case` because that is the shape STL/Boost/range-v3 consumers expect to type. *Identifier* naming (params/locals/members) is OCCT because the same rule must hold across the entire `.hxx` / `.cxx` / `.hpp` set — there is no value in two parallel naming worlds for the same `the*` concept.

---

## 4. Internal C++ implementation (`src/`)

This is OCCT-aligned territory. Style follows the project's OCCT-oriented implementation conventions.

### 4.1 Naming

| Element | Pattern | Example |
| --- | --- | --- |
| Class | `PascalCase`, no `Package_` prefix (we use namespaces instead) | `Graph`, `ErrorState` |
| Public method | `PascalCase` | `Initialize()`, `Shape()` |
| Private method | `camelCase` | `cacheLookup()` |
| Method parameter | `the<Name>` | `theGraph`, `theTolerance` |
| Local variable | `a<Name>` / `an<Name>` | `aBuilder`, `anIndex` |
| Class member | `my<Name>` | `myHandle`, `myInitialised` |
| Struct member (POD) | `<Name>` | `Code`, `Message` |
| Global / TU-static | `THE_<NAME>` | `THE_OCCT_VERSION_STRING` |
| Namespace | `OcctL`, `OcctL::Core`, `OcctL::Topo` | (PascalCase, single-word segments preferred) |

### 4.2 Modern C++ subset

- **C++17** is the baseline. C++20 features are off-limits in implementation files.
- Range-based `for`, structured bindings, `if constexpr`, `std::optional`, `std::string_view` are encouraged where they read better.
- `auto` is **only** allowed for: lambda types, range-based `for` over containers whose element type is verbose, and structured bindings. Never for "I don't want to type the type."
- `nullptr`, never `NULL` or `0` for pointers.
- `[[nodiscard]]` on factory and query methods that return values the caller would obviously want to inspect.

### 4.3 Containers

- Prefer **`std::vector`** and **`std::array`** at internal C++ boundaries (the implementation never crosses the C ABI). NCollection types are used only when interoperating with OCCT APIs that expect them.
- **Never `NCollection_Sequence`** — its linked-list shape is a performance trap. Use `std::vector` or `NCollection_Vector`.
- Strings: `std::string` / `std::string_view`. Convert to `TCollection_AsciiString` at the OCCT boundary.

### 4.4 Memory and ownership

- `std::unique_ptr` for sole ownership across function boundaries inside the implementation. `Handle(...)` for OCCT reference-counted types.
- Raw pointers are only borrowed references; never own.
- `[[nodiscard]] static` factory methods returning `std::unique_ptr<T>` for non-OCCT objects.

### 4.5 Const correctness

- `const` on every value parameter that is not modified, **including in the .hxx and .cxx**.
- `const` on every method that does not modify the object.
- `constexpr` for compile-time constants.

### 4.6 Header documentation (`//!`)

OCCT-style Doxygen with `//!` lines:

```cpp
//! Translates a C++ exception caught at the ABI boundary into an
//! occtl_status_t and populates the thread-local error.
//! @param[in] theException the caught std::exception or Standard_Failure
//! @return the translated status code; never OCCTL_OK
occtl_status_t TranslateException (const std::exception& theException);
```

Conventions:

- `@param[in]` / `@param[out]` / `@param[in,out]` on every parameter.
- `@return` for non-`void` returns.
- One-sentence summary first.
- No backstory, no implementation notes — those live next to the implementation.
- **Every comment is Doxygen-attached to a symbol.** "Free" comments (not `//!`-attached to a function, variable, type, or `@file` / `@namespace` / `@name` group) are forbidden. If a comment is worth writing, it belongs on the declaration it describes.
- **No unicode box-drawing characters** in any comment. ASCII only.

### 4.7 Source file separators

Each method is preceded by **exactly** this 100-character separator (`//` followed by 98 `=` signs) and an empty line:

```cpp
//==================================================================================================

void ErrorState::SetError (const occtl_status_t theStatus,
                           const char* const    theMessage)
{
  // … impl …
}

//==================================================================================================

void ErrorState::Clear()
{
  // … impl …
}
```

Do **not** use the old-style `// purpose: … // function: …` block. The empty line after the separator is required.

**Forbidden patterns in `.cxx` / `.hxx` files:**
- **Three-line section-header blocks** with a label sandwiched between separators:

  ```cpp
  // BAD — never do this:
  //===================================================================================================
  // Vertex queries
  //===================================================================================================
  ```

  Section labels between separators are not separators — they are orphan comments.
  Each function definition gets its own standalone `//===` line; grouping is implicit.

- **Unicode box-drawing characters** in any comment (`─`, `━`, `┄`, etc.). ASCII only.
- **Dash separators** (`// ---`, `// ———`, etc.). Only `//` + 98 `=` is valid.
- **Separators with the wrong `=` count.** Count matters: 98, not 97, not 99.
  The line is exactly 100 characters: `//` plus 98 `=`.

### 4.8 Comments inside method bodies

Default to **none**. Add a one-line comment only when:

- The *why* is non-obvious (a hidden constraint, a workaround for a specific OCCT bug, behavior that would surprise a reader).
- A multi-step section needs a high-level signpost.

Never:

- Explain *what* the code does — well-named identifiers do that.
- Reference history or prior versions ("was X, now Y", "previously …", "fixed bug Z").
- Write multi-paragraph comment blocks. One short line, max two.

Keep comments minimal and focused on non-obvious constraints.

### 4.9 File layout

```cpp
// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Capgemini Engineering Research and Development.

#include "ErrorState.hxx"

#include <Standard_Failure.hxx>

#include <cstring>
#include <mutex>

namespace OcctL::Core
{

//==================================================================================================

ErrorState::ErrorState()
: myStatus(OCCTL_OK)
{
}

//==================================================================================================

void ErrorState::SetError (const occtl_status_t theStatus,
                           const char* const    theMessage)
{
  myStatus  = theStatus;
  myMessage = theMessage != nullptr ? theMessage : "";
}

}  // namespace OcctL::Core
```

Header includes are grouped: project headers first (`""`), then OCCT headers, then standard library. One blank line between groups.

### 4.10 Exception barrier

Every `extern "C"` entry point is wrapped in `OcctL::Core::Guard`:

```cpp
//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_create (occtl_graph_t** const theOutGraph)
{
  return OcctL::Core::Guard ([&]() -> occtl_status_t {
    if (theOutGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutGraph = nullptr;
    *theOutGraph = new occtl_graph();
    return OCCTL_OK;
  });
}
```

`Guard` catches `Standard_Failure` (root of all OCCT exceptions), `std::exception`, and `...` and populates the thread-local error via `TranslateException`. **No exception ever escapes `extern "C"`.**

When a specific OCCT failure needs a cleaner status code than `OCCTL_INTERNAL`, catch it explicitly *inside* the lambda — the inner catch runs before Guard's outer catch:

```cpp
try
{
  gp_Dir anAxis(theDir.x, theDir.y, theDir.z);  // throws Standard_ConstructionError on zero vec
  // …
}
catch (const Standard_Failure& theEx)
{
  OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, theEx.what());
  return OCCTL_GEOMETRY_INVALID;
}
```

Catch `Standard_Failure` (not a narrower subclass) to guarantee the catch fires regardless of which OCCT exception type is thrown.

### 4.11 OCCT as the computation engine

Every module that links OCCT delegates to OCCT — see AGENTS hard rule #2 in the repository root. The pattern is three lines: convert POD → OCCT, call OCCT, convert back.

```cpp
const gp_Pnt aA = OcctL::Geom::ToGp(theA);
const gp_Pnt aB = OcctL::Geom::ToGp(theB);
return aA.Distance(aB);
```

Conversion helpers live in `<module>/*Math.hxx` (e.g. `src/geom/GeomMath.hxx`, `src/topo/TopoMath.hxx`):

```cpp
inline gp_Pnt ToGp(const occtl_point3_t& theP) noexcept { return {theP.x, theP.y, theP.z}; }
inline occtl_point3_t FromGp(const gp_Pnt& theP) noexcept { return {theP.X(), theP.Y(), theP.Z()}; }
```

**Forbidden** (unless no OCCT equivalent): hand-rolled vector / matrix / transformation math, custom curve / surface evaluation, magic tolerance thresholds. **Acceptable:** field copies in `ToGp` / `FromGp`, integer index work, STL container management for non-geometric data.

**Tolerances** come from OCCT's named constants — `Precision::Confusion()` (linear), `Precision::SquareConfusion()` (squared), `Precision::Angular()` (radians), `gp::Resolution()` (the threshold OCCT enforces inside `gp_Dir(gp_Vec)` / `gp_Vec::Normalize()`). When a shim pre-validates a vector before handing it to OCCT, copy OCCT's exact comparison (`aSqMod <= gp::Resolution() * gp::Resolution()`) — anything looser silently rejects inputs OCCT would have accepted.

---

## 5. Test code (`tests/`)

Tests are gtest-based C++ that drives the **public C** API (or the C++ veneer when testing veneer-specific behavior).

### 5.1 Style

- **No `//===` separators** in test files. Tests don't need them and they clutter.
- **No section-header comments** of any kind (`// Vertex tests`, `// Edge tests`, etc.). Use blank-line separation between groups of related tests.
- **No unicode box-drawing characters** in comments. ASCII only.
- **No dash-separator blocks** (`// ---`). If you need a visual separator, a single blank line is enough.
- Naming: gtest fixtures `<Module>Test`; test names `<Method>_<Scenario>_<Expected>` (e.g. `Create_NullInfo_ReturnsInvalidArgument`).
- Lean on `EXPECT_EQ` / `EXPECT_NE` / `ASSERT_EQ`. `EXPECT_NEAR` for floating-point with `Precision::Confusion()` or a documented tolerance.
- Use `EXPECT_THROW(..., occtl::Error)` to assert that the C++ veneer translates a non-OK status code into an exception. The veneer is always built with C++ exceptions enabled.
- **Minimal comments.** Same rule as production code; tests are not the place for archaeology.

### 5.2 File layout

```cpp
// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Capgemini Engineering Research and Development.

#include <occtl/occtl_core.h>

#include <gtest/gtest.h>

namespace
{

TEST(ErrorTest, Last_AfterSuccess_StatusIsOk)
{
  occtl_error_clear();
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_EQ(anErr->status, OCCTL_OK);
}

}  // namespace
```

OCCT headers first, blank line, gtest header. Anonymous namespace wraps the tests.

---

## 6. CMake style

- Lowercase commands (`add_library`, `target_link_libraries`).
- One target per `add_*` call; no multi-target one-liners.
- `target_*` over directory-level `*_directories` / `*_link_libraries`.
- Public/Private/Interface explicit on every link. Default to `PRIVATE` and only widen when consumers genuinely need it.
- File lists alphabetized.
- Variables `CamelCase` for project-local; `OCCTL_*` SCREAMING_SNAKE for cache options.
- Comments only where the *why* is non-obvious.

---

## 7. Tooling

| Tool | What it enforces |
| --- | --- |
| `clang-format` | C++ formatting (config in repo root). Public C and internal C++ share one config; the differences are stylistic, not formatting. |
| `clang-tidy` | Modernization checks (`modernize-*`) and bug-prone checks. Suppression files per directory. |
| Public-header hygiene | No `<string>`, no `Handle.hxx`, no `TopoDS_*.hxx`, no STL or OCCT identifiers in `include/occtl/*.h`. Fails the build. |
| Public-header style | No free comments or separator banners in `include/occtl/*.h` or `include/occtl-hpp/*.hpp`; only SPDX, Doxygen, and include-guard comments. |
| Guard audit | Every exported `OCCTL_API occtl_status_t` C shim in `src/**/*.cxx` contains `OcctL::Core::Guard`. |
| Doxygen build | Generates HTML for both public C and the C++ veneer. Warnings as errors. |
| `.editorconfig` | Indent (2 spaces, no tabs), line endings (LF), trailing newline, UTF-8. |
