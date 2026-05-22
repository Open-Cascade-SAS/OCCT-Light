# OCCT-Light — ABI Patterns

> The rulebook. Every public symbol, every header, every signature in OCCT-Light follows the conventions in this document. Deviations require an explicit, documented exception.

Companion docs: [ARCHITECTURE.md](./ARCHITECTURE.md) · [BREPGRAPH_AS_CANONICAL.md](./BREPGRAPH_AS_CANONICAL.md) · [MODULES.md](./MODULES.md) · [BINDINGS.md](./BINDINGS.md).

---

## 1. Naming

Naming tables for the public C surface, the C++ veneer, and the internal C++ implementation live in [`CODING_STYLE.md §2.1 / §3 / §4.1`](./CODING_STYLE.md). The short form: public functions are `occtl_<noun>_<verb>`, public types `occtl_<noun>_t`, public enums `OCCTL_<DOMAIN>_<VALUE>`, public macros `OCCTL_<NAME>`. The `occtl` prefix is reserved across every public-facing identifier — no aliases.

Module suffix is added when the noun isn't already module-scoped: `occtl_io_step_read`, but `occtl_graph_create` stays as is regardless of which module owns it.

### 1.1 Constructor verbs — domain-driven rule

Functions that construct or produce results are assigned to a verb family based on their domain role:

| Verb family | Use for | Result shape |
| --- | --- | --- |
| `create` | Geometry representations and standalone opaque/service handles. | `out_rep`, `out_handle` |
| `make` | Direct model construction: topology elements, primitives, face builders, patterns. | usually `graph + out_node`, or `out_graph + out_root`. |
| Result/action verbs | Derived-copy operations and feature edits: `transformed`, `mirrored`, `defeature`, `draft_faces`. | usually `out_graph + out_root`. |
| Bare CAD verbs | Well-known operations whose verb is the domain term: `fuse`, `cut`, `common`, `section`, `split`. | operation-specific. |
| Query verbs | `is_*`, `has_*`, `*_kind`, `*_count`, `*_view`, `*_nodes`, `*_ids`, `*_by_*`, `*_from_*`. | read-only or two-call buffers. |
| Repair verbs | In-place topology repair: `sew`, `recompute_same_parameter`, `check`. | mutates or validates existing graph. |

### 1.2 Public-function prefix conventions

Prefixes encode ownership domain:

| Prefix | Owns |
| --- | --- |
| `occtl_graph_` | Graph lifecycle, clone/compact/cache, graph-wide counts, UID tables, metadata layers, tags, units, history, graph-level iteration. |
| `occtl_topo_` | Topological entities, topology relations, topology repair/checking, topology construction, occurrence/product topology, topology feature operations. |
| `occtl_curve_` / `occtl_curve2d_` / `occtl_surface_` | Geometry representation creation, interrogation, extraction, evaluation, approximation/intersection. |

### 1.3 Boolean out-parameter naming

All `int32_t` boolean out-parameters use `out_is_*` or `out_has_*` predicate names:

```c
OCCTL_API occtl_status_t occtl_graph_node_is_closed(
    const occtl_graph_t* g, occtl_node_id_t node, int32_t* out_is_closed);
```

This convention makes the direction and boolean semantics immediately visible at the call site. (The `int32_t` type for booleans is already mandated by §13.)

### 1.4 Field / parameter naming: `*_count` not `*_nb_`

Use `*_count` for all count fields and out-parameters. The `*_nb_` form is not part of the public ABI.

### 1.5 I/O parameter ordering convention

Output parameters precede input parameters (§7). Within the input group:

- **Read paths:** the subject object comes first, then qualifiers.
- **Write (construction) paths:** the graph (mutation target) comes first, then constructor arguments.

This keeps call sites predictable across the entire surface.

### 1.6 STEP/IGES enum prefix

All public enums in the `occtl_io_step` and `occtl_io_iges` modules use the `occtl_io_step_` / `occtl_io_iges_` prefix:

```c
typedef enum occtl_io_step_format {
  OCCTL_IO_STEP_FORMAT_STEP,
  OCCTL_IO_STEP_FORMAT_STEP_XML,
  OCCTL_IO_STEP_FORMAT_RESERVED_FUTURE = 0x7fffffff
} occtl_io_step_format_t;
```

## 2. Symbol export

A single macro controls visibility:

```c
#if defined(_WIN32)
  #if defined(OCCTL_BUILD_SHARED)
    #define OCCTL_API __declspec(dllexport)
  #elif defined(OCCTL_USE_SHARED)
    #define OCCTL_API __declspec(dllimport)
  #else
    #define OCCTL_API
  #endif
  #define OCCTL_CALL __cdecl
#else
  #define OCCTL_API __attribute__((visibility("default")))
  #define OCCTL_CALL
#endif
```

Build flags: `-fvisibility=hidden -fvisibility-inlines-hidden` on Unix; `/Zc:__cplusplus /utf-8` on MSVC. Only `OCCTL_API`-marked symbols leak.

Every public function declaration is `OCCTL_API occtl_status_t OCCTL_CALL occtl_xxx(...);`. The `OCCTL_CALL` matters for Win32-32-bit P/Invoke; we set it explicitly to `__cdecl` and document it.

## 3. Header hygiene

A public header may include only:

- `<stdint.h>`, `<stddef.h>`
- Other `occtl_*.h` headers from this project

It must not include `<string>`, `<vector>`, `<memory>`, `<Handle.hxx>`, `<TopoDS_*.hxx>`, `<NCollection_*.hxx>`, or anything else from C++/STL/OCCT.

A CI check enforces this:

```bash
grep -RE '#include[[:space:]]+<(?!stdint|stddef|occtl_)' include/occtl/ \
  && exit 1 || exit 0
```

The C++ veneer (`occtl-hpp`) may use STL freely; it's an opt-in `#include`.

Each header guards its content with both an include guard and `extern "C"`:

```c
#ifndef OCCTL_TOPO_H
#define OCCTL_TOPO_H

#include <stdint.h>
#include <stddef.h>
#include "occtl_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* … */

#ifdef __cplusplus
}
#endif

#endif /* OCCTL_TOPO_H */
```

## 4. Opaque handles

Heap-allocated objects are exposed as opaque types:

```c
typedef struct occtl_graph occtl_graph_t;
```

The struct definition lives in internal headers; consumers only ever see the typedef and pass `occtl_graph_t*`.

**Lifetime contract** is documented in the docstring of every function returning a pointer:

- **Owns it** — caller is responsible for freeing via the matching `occtl_<noun>_free`. Free functions are NULL-tolerant and idempotent.
- **Borrows it** — pointer is valid only for a documented scope (e.g., "until `parent` is freed or modified"). Caller must not free, must not retain across the documented boundary.

Examples:

```c
/* Owns it: caller must call occtl_graph_free. */
OCCTL_API occtl_status_t occtl_graph_create(occtl_graph_t** out);

OCCTL_API void occtl_graph_free(occtl_graph_t* g);

/* Borrows it: pointer valid until the graph is mutated or freed. */
OCCTL_API const char* occtl_graph_name(
    const occtl_graph_t* g, id_t id);
```

Free functions never return a status; they cannot fail in a way the caller can act on. They log internally if something goes wrong.

## 5. Lightweight value handles

Identity types are passed by value, not allocated:

```c
typedef struct occtl_node_id { uint64_t bits; } occtl_node_id_t;
typedef struct occtl_ref_id  { uint64_t bits; } occtl_ref_id_t;
typedef struct occtl_uid     { uint64_t bits; } occtl_uid_t;
typedef struct occtl_rep_id  { uint64_t bits; } occtl_rep_id_t;
```

Wrapped as a struct (not a bare `uint64_t`) so that the type system catches "vertex id passed where a face id was expected" at the C++ veneer level and so that bindings see a distinct nominal type. The `bits` field is layout-stable but the internal bit layout (kind in [63:56], reserved in [55:48], payload in [47:0]) is a **private implementation detail** confined to `src/topo/TopoMath.hxx`. No `static inline` helpers expose it in public headers — to query the kind of an ID, call the graph function (e.g. `occtl_graph_node_kind`) which internally unpacks to the OCCT entity type.

See [BREPGRAPH_AS_CANONICAL.md §3](./BREPGRAPH_AS_CANONICAL.md) for the full ID model.

## 6. Error reporting

### 6.1 Status codes

```c
typedef enum occtl_status {
  OCCTL_OK = 0,                     /* success */

  OCCTL_ERROR             = 1,      /* generic, see error message */
  OCCTL_INVALID_ARGUMENT  = 2,
  OCCTL_INVALID_HANDLE    = 3,
  OCCTL_NOT_FOUND         = 4,
  OCCTL_OUT_OF_MEMORY     = 5,
  OCCTL_OUT_OF_RANGE      = 6,
  OCCTL_NOT_DONE          = 7,      /* OCCT operation reported IsDone()==false */
  OCCTL_GEOMETRY_INVALID  = 8,
  OCCTL_TOPOLOGY_INVALID  = 9,
  OCCTL_IO_ERROR          = 10,
  OCCTL_FORMAT_ERROR      = 11,
  OCCTL_UNSUPPORTED       = 12,
  OCCTL_CANCELLED         = 13,
  OCCTL_BUFFER_TOO_SMALL  = 14,     /* two-call pattern */
  OCCTL_VERSION_MISMATCH  = 15,     /* struct_version unsupported */
  OCCTL_INTERNAL          = 16,     /* C++ exception caught at boundary */
  OCCTL_WRONG_KIND        = 17,     /* extractor called on a handle holding a different kind */

  OCCTL_STATUS_RESERVED_FUTURE = 0x7fffffff
} occtl_status_t;
```

`OCCTL_OK == 0`. The idiom `if (occtl_xxx(...) != OCCTL_OK) goto fail;` works everywhere. The convenience macros `OCCTL_FAILED(s)` / `OCCTL_SUCCEEDED(s)` are provided.

#### `OCCTL_WRONG_KIND` — when to use it

Reserved for: an extractor (`occtl_*_as_<kind>`, `occtl_*_bspline_*`) is called on an opaque handle for which the data the function unpacks is not present. This covers two cases:

1. The stored kind tag does not match — e.g. `occtl_curve_as_circle` on a line, `occtl_curve_bspline_poles` on an analytic conic.
2. The stored kind tag matches but a required sub-state is false — e.g. `occtl_curve_bspline_weights` on a B-spline whose `IsRational()` is false (no weight array exists to extract).

Do **not** use `OCCTL_WRONG_KIND` for:

- Generic constructor input validation — that is `OCCTL_GEOMETRY_INVALID` or `OCCTL_INVALID_ARGUMENT`.
- Cross-handle errors (mixing a `curve_t` with a `surface_t` API). Those are `OCCTL_INVALID_ARGUMENT`.
- A type-correct, sub-state-correct extraction where the underlying geometry happens to be numerically degenerate — those are `OCCTL_GEOMETRY_INVALID`.

The contract: a caller that has already inspected `kind()` (and, for sub-state-gated extractors, the matching `is_rational` / `is_periodic` flag) and dispatched on it must never see `OCCTL_WRONG_KIND`. It is a programmer-error code, not a data-driven one.

Extended subcodes use a 32-bit value — primary code in the low byte, refinement in the upper bits:

```c
#define OCCTL_EXTENDED(primary, sub)  (((uint32_t)(sub) << 8) | (uint32_t)(primary))
#define OCCTL_PRIMARY(ext)            ((occtl_status_t)((ext) & 0xff))
```

Primary codes are stable forever; extended codes evolve. Bindings filter on the primary; debug tools surface the extended.

### 6.2 Thread-local last error

```c
typedef struct occtl_error {
  occtl_status_t status;
  const char*    message;   /* UTF-8, library-owned, valid until next call on this thread */
  occtl_uid_t    source;    /* zeroed if not applicable */
  uint32_t       extended;  /* extended subcode */
} occtl_error_t;

OCCTL_API const occtl_error_t* occtl_error_last(void);
OCCTL_API void                 occtl_error_clear(void);
OCCTL_API const char*          occtl_status_to_string(occtl_status_t s);
```

`occtl_error_last()` returns a pointer to thread-local storage — it never returns NULL, but `status == OCCTL_OK` means "no error pending". Calling another OCCTL function may overwrite it on this thread; copy out anything you need before the next call.

### 6.3 Translation contract

Every internal entry point is wrapped:

```cpp
// internal pattern, not visible
extern "C" OCCTL_API occtl_status_t occtl_graph_create(occtl_graph_t** out)
{
  return occtl::internal::guard([&] {
    if (!out) return OCCTL_INVALID_ARGUMENT;
    /* … real work … */
    return OCCTL_OK;
  });
}
```

The `guard` helper catches `Standard_Failure`, `std::exception`, and `...`, populates the thread-local error, and returns `OCCTL_INTERNAL`. **No exception ever crosses the C boundary.**

## 7. Out-pointer-first signatures

```c
OCCTL_API occtl_status_t occtl_graph_create(occtl_graph_t** out);
```

Rule: ordinary fallible ABI functions return `occtl_status_t`. Outputs are written through pointer parameters. When there is one primary output, it goes first. When there are several outputs, primary first then secondary, all before inputs.

The only direct-return exceptions are genuinely infallible pure-value helpers in `core` and POD math helpers in `geom`, such as status/runtime/version accessors, `occtl_uid_equal`, and value-to-value point/vector/direction/transform functions. Graph queries, data-exchange helpers, ownership-transfer functions, and anything that validates a handle or user pointer use status + out-parameter.

Inputs are `const`-qualified pointers. Output pointers must not be NULL when the function is called — passing NULL where the API expects an output pointer is `OCCTL_INVALID_ARGUMENT`.

Within the input group, see §1.5 for the read-path / write-path ordering convention.

Convention: name parameters `out_<thing>` for clarity in headers.

## 8. Options / info structs

Every constructor takes a `const <name>_create_info_t*` rather than a long parameter list. This is the pattern that survives evolution:

```c
typedef struct occtl_prim_box_info {
  uint32_t          struct_version;   /* OCCTL_PRIM_BOX_INFO_VERSION_1 */
  const void*       p_next;           /* extension chain, must be NULL today */
  double            dx, dy, dz;       /* dimensions */
} occtl_prim_box_info_t;

#define OCCTL_PRIM_BOX_INFO_VERSION_1 1u

#define OCCTL_PRIM_BOX_INFO_INIT \
  { OCCTL_PRIM_BOX_INFO_VERSION_1, NULL, 1.0, 1.0, 1.0 }

OCCTL_API void occtl_prim_box_info_init(occtl_prim_box_info_t* info);
```

Every options struct has:

1. `uint32_t struct_version` as its first field, set to a `*_VERSION_<N>` macro.
2. `const void* p_next` as its second field, reserved for extension chains. Always `NULL` until an extension is shipped.
3. A static initializer macro (`*_INIT`) for C use.
4. A runtime initializer function (`*_init`) for binding consumers that can't use C macros.
5. Field defaults documented per field. Zero-initialization (calloc + version) is always a valid-but-default-everywhere starting point.

When fields are added in a future version, a new `*_VERSION_2` macro and a new struct layout are introduced; the implementation reads `struct_version` and uses the right layout. Old binaries continue to work.

## 9. Strings

All strings are **UTF-8**. Always.

### 9.1 String inputs

```c
/* NUL-terminated */
OCCTL_API occtl_status_t occtl_io_step_read(const char* path, ...);

/* Length-counted, when the input may not be NUL-terminated */
OCCTL_API occtl_status_t occtl_graph_name_set(occtl_graph_t*, occtl_node_id_t,
                                              const char* name, size_t name_len);
```

Length-counted variants are preferred for any user-supplied content; NUL-terminated is fine for paths and small identifiers from the program itself.

### 9.2 String outputs — two-call buffer pattern

```c
OCCTL_API occtl_status_t occtl_graph_describe(
    const occtl_graph_t* g,
    char*  buf,            /* may be NULL on the sizing call */
    size_t buf_size,       /* size of buf in bytes; ignored if buf is NULL */
    size_t* out_required); /* required size in bytes including the trailing NUL */
```

Caller calls once with `buf == NULL` to learn the size, allocates, calls again. Returns `OCCTL_BUFFER_TOO_SMALL` if `buf_size` was insufficient (with `*out_required` set), `OCCTL_OK` otherwise. The output is always NUL-terminated when `OCCTL_OK` is returned.

### 9.3 Borrowed strings

When the library can hand back a pointer to its own storage (format names, layer names, error messages), the function returns `const char*` directly with documented lifetime:

```c
/* Borrowed: valid until the next call to any occtl function on this thread. */
OCCTL_API const char* occtl_status_to_string(occtl_status_t s);

/* Borrowed: valid until the graph is freed or the layer is removed. */
OCCTL_API const char* occtl_graph_name_get(const occtl_graph_t*, id_t);
```

### 9.4 Library-owned heap strings

Used only when the two-call pattern is awkward (e.g., complex variable-length tree dumps):

```c
typedef struct occtl_string occtl_string_t;
OCCTL_API const char* occtl_string_data(const occtl_string_t*);
OCCTL_API size_t      occtl_string_size(const occtl_string_t*);
OCCTL_API void        occtl_string_free(occtl_string_t*);
```

Prefer the two-call pattern. `occtl_string_t` exists for the cases where it's genuinely better.

## 10. Sequences and aggregate reads

Pick from a fixed menu, per use case:

### 10.1 Two-call buffer (unknown size, immutable snapshot)

```c
OCCTL_API occtl_status_t occtl_topo_solid_faces(
    const occtl_graph_t* g,
    occtl_node_id_t      solid,
    occtl_node_id_t*     out_faces,    /* may be NULL on sizing call */
    size_t               capacity,
    size_t*              out_count);
```

Same shape as the string variant. Returns `OCCTL_BUFFER_TOO_SMALL` if `capacity` is too small. Count out-parameters use `*_count` (§1.4); `*_nb_*` names are not part of the public ABI.

> **Steer:** for sparse-iteration sources (e.g. BRepGraph iterators that omit removed nodes), prefer §10.3 — the two-call buffer would otherwise walk the structure twice (once to size, once to fill) for data the caller usually iterates once and discards. Use §10.1 for stable snapshots whose length is genuinely unknown until the call has run, not for pull-style enumeration.

### 10.2 Span / view (zero-copy hot path)

```c
typedef struct occtl_triangulation_view {
  const double*   nodes;          /* xyz, length 3*node_count */
  size_t          node_count;
  const double*   normals;        /* may be NULL if absent */
  const uint32_t* indices;        /* triplets, length 3*triangle_count */
  size_t          triangle_count;
} occtl_triangulation_view_t;

OCCTL_API occtl_status_t occtl_mesh_face_triangulation(
    const occtl_graph_t* g,
    occtl_node_id_t      face,
    occtl_triangulation_view_t* out);
```

The view points into library-owned memory. **Lifetime is documented per call** — typically "valid until the graph is mutated or freed". This is what lets NumPy, `Span<T>`, and `TypedArray` consumers wrap the data zero-copy.

### 10.3 Opaque iterator (sparse / lazy / unbounded)

The canonical shape is one opaque type, many factories, one shared `_next` / `_free` pair when every factory yields the same value type. The shipped topo iterators use this exact shape (one `occtl_node_iter_t`, 19 `*_iter_create` factories, shared `occtl_node_iter_next` / `_free`):

```c
typedef struct occtl_node_iter occtl_node_iter_t;

OCCTL_API occtl_status_t occtl_graph_face_iter_create(
    const occtl_graph_t* g, occtl_node_iter_t** out);

OCCTL_API occtl_status_t occtl_topo_faces_of_shell_iter_create(
    const occtl_graph_t* g, occtl_node_id_t shell, occtl_node_iter_t** out);

OCCTL_API occtl_status_t occtl_node_iter_next(
    occtl_node_iter_t* iter, occtl_node_id_t* out_id);   /* OCCTL_NOT_FOUND when done */

OCCTL_API void occtl_node_iter_free(occtl_node_iter_t* iter);
```

Why one type rather than per-kind types: the alternative (`occtl_face_iter_t`, `occtl_edge_iter_t`, …) multiplies the binding footprint by the number of factories. With a uniform output type (`occtl_node_id_t`) only the source varies; collapsing to one type halves binding-side stub code and keeps `_next` / `_free` on the cache line for every iterator.

End-of-iteration sentinel: `_next` returns `OCCTL_OK` on each step; `OCCTL_NOT_FOUND` once exhausted, with `OCCTL_NODE_ID_INVALID` written to the out-param. Subsequent calls remain `OCCTL_NOT_FOUND` (idempotent end). When a different value type is needed (e.g. an iterator that yields `(NodeId, Transform)` for child-explorer traversal), introduce a new opaque type for that family rather than overloading the existing one.

`occtl_error_last()` on the end-of-iteration path: **do not populate it**. Iterator exhaustion is the contract's success terminus, not an error. Reserve `occtl_error_last()` for argument-level failures (NULL inputs, uninitialised handles) so clients that poll the error state every call do not see a phantom "iterator exhausted" message at the natural end of every loop.

Lifetime / invalidation: documented per factory. Topo iterators borrow from the source graph; adding nodes or freeing the graph while an iterator is live is undefined; removing a not-yet-visited node is well-defined and omitted (matching OCCT's `IsRemoved` semantics).

Distinct types are correct when the yield differs: an iterator that yields strings, or `(node, score)`, or transformed shapes from a child-explorer, gets its own opaque struct and its own `_next` / `_free`.

### 10.4 Visitor callback (alternative to iterator)

```c
typedef occtl_status_t (OCCTL_CALL* occtl_node_visitor_t)(occtl_node_id_t node, void* user_data);

OCCTL_API occtl_status_t occtl_graph_for_each(
    const occtl_graph_t* g, uint64_t kind_mask,
    occtl_node_visitor_t fn, void* user_data);
```

Both shapes coexist for the same data when feasible, because callbacks across FFI are awkward in some languages (notably WASM) and iterators are awkward in others.

### 10.5 Aggregate-view read (rich entity inspection)

When an opaque entity has **five or more correlated fields** that callers tend to read together — most often a curve / surface / face introspection — expose a single function that fills a caller-allocated POD struct in one call, with borrowed pointers into library-owned storage for any variable-length parts.

This is the stable OCCT-Light shape for aggregate reads:

- caller-allocated outer struct with explicit versioning fields;
- one call fills all correlated scalars and borrowed spans;
- borrowed pointers carry explicit parent-tied lifetime rules.

OCCT-Light combines them: caller-allocated outer + `struct_version` + `p_next` + borrowed inner spans whose lifetime tracks the parent entity. No allocation by the library; no paired free; one FFI call replaces ten.

#### 10.5.1 Naming

| Element | Pattern | Example |
| --- | --- | --- |
| View type | `occtl_<entity>_<kind>_t` (no `_view` suffix; see below) | `occtl_curve_bspline_t` |
| Extractor | `occtl_<entity>_as_<kind>(entity, &out)` | `occtl_curve_as_bspline` |
| Version macro | `OCCTL_<ENTITY>_<KIND>_VERSION_<N>` | `OCCTL_CURVE_BSPLINE_VERSION_1` |
| Static initializer | `OCCTL_<ENTITY>_<KIND>_INIT` | `OCCTL_CURVE_BSPLINE_INIT` |
| Init function | `occtl_<entity>_<kind>_init(POD*)` | `occtl_curve_bspline_init` |

The output type does not carry a `_view` suffix: whether internal pointers are borrowed (lifetime = parent) or absent (fixed-shape POD) is documented per function. Callers learn the contract from the docstring, not the name. This keeps the existing `_as_circle` / `_as_line` / `_as_ellipse` family intact — they are already aggregate-view reads with no borrowed pointers, and the name shape is unchanged.

#### 10.5.2 Canonical example (B-spline curve)

```c
#define OCCTL_CURVE_BSPLINE_VERSION_1 1u

typedef struct occtl_curve_bspline {
  uint32_t        struct_version;     /* INPUT — caller declares which version they understand. */
  const void*     p_next;             /* Reserved; must be NULL today. */

  /* Scalars. */
  int32_t         degree;
  int32_t         is_rational;        /* 0/1 */
  int32_t         is_periodic;        /* 0/1 */
  int32_t         is_closed;          /* 0/1 */
  size_t          pole_count;
  size_t          knot_count;         /* unique knots; equal to length of `knots` and `multiplicities` */
  size_t          flat_knot_count;    /* sum of multiplicities */

  /* Borrowed spans — lifetime = parent curve, until it is freed or mutated.
   * Set to NULL when the corresponding sub-state is absent (e.g. `weights` is
   * NULL when `is_rational == 0`). */
  const occtl_point3_t* poles;             /* pole_count elements */
  const double*         weights;           /* pole_count elements, or NULL */
  const double*         knots;             /* knot_count unique knots */
  const int32_t*        multiplicities;    /* knot_count multiplicities */
  const double*         flat_knots;        /* flat_knot_count knots */
} occtl_curve_bspline_t;

#define OCCTL_CURVE_BSPLINE_INIT \
  { OCCTL_CURVE_BSPLINE_VERSION_1, NULL, \
    0, 0, 0, 0, 0, 0, 0, \
    NULL, NULL, NULL, NULL, NULL }

OCCTL_API void occtl_curve_bspline_init(occtl_curve_bspline_t* out);

OCCTL_API occtl_status_t occtl_curve_as_bspline(
    const occtl_curve_t*      curve,
    occtl_curve_bspline_t*    out);
```

Caller use, C:

```c
occtl_curve_bspline_t bspline = OCCTL_CURVE_BSPLINE_INIT;
if (occtl_curve_as_bspline(curve, &bspline) == OCCTL_OK) {
  for (size_t i = 0; i < bspline.pole_count; ++i) {
    use(bspline.poles[i]);
  }
}
/* No release call — pointers are borrowed; valid until `curve` is freed/mutated. */
```

Caller use, binding (cffi):

```python
bspline = lib.occtl_curve_bspline_t()
lib.occtl_curve_bspline_init(bspline)
ok(lib.occtl_curve_as_bspline(curve, bspline))
poles = ffi.buffer(bspline.poles, bspline.pole_count * ffi.sizeof('occtl_point3_t'))  # zero-copy
```

#### 10.5.3 Lifetime contract

Borrowed pointers in the filled struct are **valid until the parent entity is freed or mutated**. Concretely for a curve view: the pointers are valid until the next call that takes the same `occtl_curve_t*` non-`const` (none today; `occtl_curve_free` invalidates).

Document the contract on every aggregate-view function. The docstring sentence is:

> Pointers in @p out borrow from @p curve and are valid until @p curve is freed.

#### 10.5.4 Versioning

`struct_version` is **input**: the caller declares which version of the layout they understand. The library:

- Returns `OCCTL_VERSION_MISMATCH` if `struct_version` is unrecognised (older library, newer caller).
- Fills only fields up to and including the version the caller declared (newer library, older caller). Fields beyond the declared version are left untouched.
- Always reads only the prefix it understands; never assumes the caller's struct extends further than `struct_version` indicates.

New fields are appended; the layout up to a given `_VERSION_<N>` is frozen forever. `p_next` is reserved for future extension chains and must be NULL today.

#### 10.5.5 Behavior on inapplicable handles

- The opaque handle does not match the kind being extracted (`occtl_curve_as_bspline` on a line) — return `OCCTL_WRONG_KIND` per §6.1. No fields are written.
- The kind matches but a sub-state is false (B-spline whose `IsRational()` is false) — return `OCCTL_OK` and set the optional pointer (`weights`) to `NULL`. **Do not** error on optional fields; bindings rely on the NULL-means-absent sentinel.
- The kind matches but the entity is degenerate (zero poles, etc.) — return `OCCTL_GEOMETRY_INVALID`.

#### 10.5.6 Coexistence with atomized accessors

Atomized accessors (`occtl_curve_bspline_degree`, `_pole_count`, `_poles`, ...) remain when an aggregate-view function exists. They are current API, not a removal path. The aggregate-view is purely additive:

- Bindings that walk many entities use the aggregate (one FFI call per entity).
- Tooling that reads a single field uses the atom (cheaper for a single value).
- Internal implementation: both paths read the same OCCT object. The aggregate function is a thin shim; atoms wrap individual OCCT accessors.

#### 10.5.7 Coexistence with §10.2 single-span view

§10.2 covers the case where the only output is a single span (e.g. mesh triangulation). §10.5 generalises to many fields, some of them spans. When the natural answer is "one span and that's it," prefer §10.2 — fewer types, less boilerplate. When the entity has scalars *and* spans, use §10.5.

#### 10.5.8 When NOT to use aggregate-view

- The entity has 1–4 fields. Atomized accessors are fine; the struct adds noise.
- The caller may write fields back. View structs are read-only by contract; mutation goes through atomized setters or a dedicated builder.
- The caller needs a snapshot stable across mutation (e.g. cross-thread, or "freeze this state and free the parent"). Borrowed pointers do not survive this. A copy-Get variant is a future addition (suffix `_snapshot`, paired with `_snapshot_release`); not adopted in this revision.

#### 10.5.9 Macros — what we provide and what we don't

Per view type, three pieces, mirroring §8:

1. `_VERSION_<N>` integer macro per shipped version of the struct.
2. `_INIT` static-initializer literal that zero-initialises everything except `struct_version`.
3. `_init(POD*)` runtime function for binding consumers that cannot use C macros.

We do **not** provide generic `OCCTL_VIEW_INIT(type, var)` magic, `_Generic` dispatch, or x-macros. Each view type is hand-written; the cookie-cutter is small and explicit. Binding generators read the headers directly and don't need preprocessor introspection.

## 11. Maps and sets

Not exposed. Underlying maps (layer attributes, name registries) are surfaced via either a paired-array snapshot or an opaque iterator. Bindings reconstruct native dicts.

```c
/* Paired-array snapshot (immutable view). */
OCCTL_API occtl_status_t occtl_graph_color_entries(
    const occtl_graph_t* g,
    const occtl_node_id_t** out_keys,
    const occtl_color_rgba_t** out_values,
    size_t* out_count);
```

## 12. Callbacks

Rules:

- `void* user_data` is always the **last** parameter. No exceptions.
- Callbacks are documented `noexcept`. C++ callers must catch their own; the wrapper does not.
- If a callback can communicate failure or cancellation, it returns `occtl_status_t`. `OCCTL_CANCELLED` short-circuits the host operation.
- Calling-convention macro is applied: `typedef occtl_status_t (OCCTL_CALL *occtl_progress_callback_t)(...)`.
- For long-running operations, the wrapper packages progress + cancellation in one struct:

```c
typedef occtl_status_t (OCCTL_CALL *occtl_progress_callback_t)(
    double fraction,           /* 0.0 .. 1.0 */
    const char* stage,         /* UTF-8, library-owned */
    void* user_data);

typedef struct occtl_progress {
  uint32_t                       struct_version;
  const void*                    p_next;
  occtl_progress_callback_t      callback;
  void*                          user_data;
  int32_t                        cancel_requested;  /* set externally; readable atomically */
} occtl_progress_t;
```

Bindings with garbage collectors must keep a strong reference to the wrapper holding `user_data` for the lifetime of the registration.

## 13. Boolean parameters

Use `int32_t` (0 = false, non-zero = true), not `bool`, not `_Bool`. This is the form that crosses every binding without size or marshalling surprises (notably P/Invoke on .NET Framework, Win32-32-bit, and some WASM toolchains). The C++ veneer takes `bool` and converts.

## 14. Enum stability

- New values **append**. Never reorder, never reuse a value.
- Each public enum carries `OCCTL_<ENUM>_RESERVED_FUTURE = 0x7fffffff` so the underlying storage is forced to 32-bit signed.
- Removed enum values keep their slots forever; the symbol stays defined and the function returns `OCCTL_UNSUPPORTED`.
- Bindings consume the enum as `int32_t` and switch on known values; unknown values map to a "future / unrecognized" sentinel.

## 15. Geometry Rep Ownership

Geometry reps (`occtl_curve_t*`, `occtl_surface_t*`, and their
`occtl_rep_id_t` identifiers) are graph-owned implementation objects. Public
callers never own an OCCT geometry object directly. A successful geometry
`create` function stores the rep in the target graph and returns the rep ID
that later topology builders can reference.

The `create` verb is therefore intentional for geometry reps: it creates graph
state, not a standalone heap object that the caller must free. Functions that
turn reps into topology (`occtl_topo_curves_to_wire`,
`occtl_prim_make_face_from_surface`, and related helpers) borrow those rep IDs
from the graph and do not transfer rep ownership.

Bindings should expose rep IDs as strong nominal values tied to the owning
graph. They should not provide a public "free curve" or "free surface" API for
graph-owned reps; releasing the graph releases its reps.

## 16. Versioning

The three dimensions (library SemVer, `OCCTL_ABI_VERSION`, per-struct `struct_version`) are spelled out in [`ARCHITECTURE.md §7`](./ARCHITECTURE.md#7-versioning). What's specific to this doc:

- **Aggregate-view output structs** (§10.5) participate in struct-version evolution exactly like input info structs: caller declares the version, library fills only up to the declared version.
- **Math PODs are not versioned.** `occtl_point3_t`, `occtl_geom_circle_t`, `occtl_axis2_placement_t`, `occtl_oriented_node_t` are value types — passed by value, embedded, stack-allocated. If their shape has to change, ship a new type (`occtl_geom_circle2_t`); don't retrofit a `struct_version` field. Versioning is for *bags*, not for *values*.
- That split decides which `_as_<kind>` extractors follow §10.5: `occtl_curve_as_circle` outputs a math POD and stays as is; `occtl_curve_as_bspline` outputs an inspection bag and follows §10.5.
- Symbol evolution: when behavior must change, ship `_v2`; old symbol is preserved bit-for-bit until a major version bump.

## 17. The C++ veneer (`occtl-hpp`)

A **header-only** C++ wrapper layered over the C ABI. No additional library; nothing links against it; it just makes C++ users productive.

Design rules:

- Each opaque handle gets a thin RAII wrapper (`occtl::Graph`) that holds the `occtl_graph_t*` and frees on destruction. Move-only by default; copy is explicit (`Graph::clone()`).
- Every status code is checked. A non-OK status throws `occtl::Error`, which carries `code`, `message`, `source` (UID), and `extended`.
- Out-pointer-first calls become value-returning members (`Graph Graph::create(const CreateInfo&)`).
- Strings: input as `std::string_view`; output as `std::string` for owned strings, `std::string_view` for borrowed strings (with documented lifetime).
- Sequences: `std::span<const T>` where C++20 is available; otherwise `(const T* data, size_t size)` accessors plus `.to_vector()` materializers.
- Options structs are exposed as C++ aggregates that derive from (or contain) the C struct, with field-style initialization. The veneer constructs the C struct and forwards.
- The veneer is generated/maintained alongside the C headers; a CI step parses the C headers and confirms the veneer covers every public symbol. Mismatch fails the build.

What the veneer does **not** do:

- Add types, methods, or behavior beyond what the C ABI offers.
- Hide enum values, error codes, or status semantics. C++ users get the same vocabulary as cffi/P/Invoke users — just in a different shape.
- Throw exceptions across C ABI boundaries (the C ABI never sees them; the veneer sits above it).

## 18. Bindings strategy

| Language | Approach |
| --- | --- |
| **Python** | `cffi` (ABI mode) + thin Pythonic facade. NumPy interop via §10.2 spans. |
| **C#** | P/Invoke from a Roslyn source generator over the C headers. `Span<T>` for view types. |
| **JS/TS native** | Node N-API addon + TS types generated from the C headers. |
| **JS/TS web** | Emscripten compile of the C surface; thin `Embind` glue. |
| **Rust** | `bindgen` for the unsafe `occtl-sys` crate + hand-written safe `occtl` crate mirroring `occtl-hpp`. |
| **Go** | `cgo` + hand-written safe layer. |
| **Java** | JNA raw layer generated from `build/abi.json` + thin idiomatic facade and parity runner. |
| **Swift** | Direct C-header import; thin Swift facade. |

Per-language facades live in `bindings/<lang>/`; they evolve faster than the C surface and are not part of the ABI.

Full plan — the uniform binding contract every facade implements, per-language layouts, packaging, test parity, and the checklist for adding a new language — is in [BINDINGS.md](./BINDINGS.md). This table is the index; that doc is the body.

## 19. Documentation conventions

Every public function header in `include/occtl/` carries Doxygen-style comments:

```c
/**
 * Creates an empty topology graph.
 *
 * @param[out] out_graph Owns it.  Must be non-NULL.  On success
 *                       receives a valid handle (never NULL); on
 *                       failure set to NULL.
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_graph is NULL.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_graph_free
 */
OCCTL_API occtl_status_t occtl_graph_create(occtl_graph_t** out_graph);
```

Each `@param` says **owns it / borrows it** explicitly. Each function lists every status code it can return. Generated HTML is published alongside binary releases.

## 20. Anti-patterns (do not do these)

- Returning data pointers from a fallible function whose return type is the data, not a status. (Use status; data goes through out-pointer.)
- Using `bool` or `_Bool` in public signatures.
- Including STL or OCCT headers from public headers.
- Returning `Handle(...)` typedefs, even as `void*`. Wrap in an opaque struct.
- Passing variable-length data via NUL-terminated pointer + no length, when the data could legitimately contain a NUL byte.
- Returning a pointer to a stack-allocated buffer (i.e. `static thread_local char buf[256]`) for arbitrary string output. Use the two-call pattern.
- Throwing exceptions across `extern "C"`. Ever.
- Adding a function without documenting whether each pointer is owned or borrowed.
- Exposing an entity's introspection through five or more atomized scalar getters with no aggregate-view companion (§10.5). Bindings end up paying N FFI calls per entity in their inner loops; the per-field accessors are fine *in addition to* the view, not *instead of* it.
