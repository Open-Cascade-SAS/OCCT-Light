# OCCT-Light — BRepGraph as the Canonical Topology

> The single most consequential design decision in OCCT-Light: the public ABI is built on **BRepGraph**, not `TopoDS`. This document explains why, what that means in C, and how algorithms that still need `TopoDS` are handled invisibly.

Companion docs: [ARCHITECTURE.md](./ARCHITECTURE.md) · [ABI_PATTERNS.md](./ABI_PATTERNS.md) · [MODULES.md](./MODULES.md) · [BINDINGS.md](./BINDINGS.md).

---

## 1. Why BRepGraph, not TopoDS

`TopoDS_Shape` is OCCT's classical tree-shaped topology (`TopoDS_TShape*` + `TopLoc_Location`). For a binding-driven wrapper it's a poor fit: identity is by pointer (not stable across operations), adjacency queries need auxiliary maps, metadata requires OCAF/XCAF, and geometry is per-shape rather than per-definition.

**BRepGraph** is OCCT's incidence-table DAG with explicit Definition vs Usage separation, typed `NodeId`/`UID`/`RefId`/`RefUID` identity, deduplicated representation storage, `VersionStamp` invalidation, and a Layer system for metadata. Read/write/traverse/sew/check/mesh/render are graph-native; Booleans, fillets, and offsets still round-trip through `TopoDS` internally — invisible to ABI users (see §12). Headers live in OCCT 8.0.0 under `src/ModelingData/TKBRep/BRepGraph/` and `src/ModelingData/TKBRep/BRepGraphInc/`.

OCCT 8.0.0 splits the BRepGraph surface across toolkits: `BRepGraph` / `BRepGraphInc` are in `TKBRep`, `BRepGraphAlgo` / `BRepGraphCheck` are in `TKTopAlgo`, `BRepGraphMesh` is in `TKMesh`, `BRepGraphDE` is in `TKDE`, and `BRepGraphPrs` / `AIS_BRepGraph` are in `TKV3d`.

Choosing BRepGraph as canonical buys: one persistent identity (`UID`) usable by every external system, O(1) adjacency, color/name/custom metadata on layers without OCAF, and insulation from `TopoDS` invariants as more OCCT algorithms migrate.

## 2. The contract

Five sentences:

1. The public ABI exposes `occtl_graph_t` plus value-typed `occtl_node_id_t`, `occtl_ref_id_t`, `occtl_uid_t`, `occtl_rep_id_t`.
2. `TopoDS_*` is **never** named in any public header.
3. Every entry point that accepts a shape accepts a graph + node id, never a TopoDS handle.
4. Every algorithm exit point produces graph + node id(s), never a TopoDS handle.
5. Algorithms that internally require TopoDS round-trip transparently; the user sees graph in, graph out.

Compliance with this contract is a CI grep:

```bash
grep -RE '(TopoDS|TopLoc|BRepBuilderAPI|BRepAlgoAPI)' include/occtl/ \
  && exit 1 || exit 0
```

## 3. C representation of typed IDs

OCCT's BRepGraph uses statically typed IDs (`BRepGraph_VertexId`, `BRepGraph_EdgeId`, ...) that are template-tagged with their kind. In C we don't have templates, so we carry the kind explicitly inside a 64-bit packed value:

```c
typedef enum occtl_node_kind {
  OCCTL_KIND_INVALID    = 0,
  OCCTL_KIND_SOLID      = 1,
  OCCTL_KIND_SHELL      = 2,
  OCCTL_KIND_FACE       = 3,
  OCCTL_KIND_WIRE       = 4,
  OCCTL_KIND_EDGE       = 5,
  OCCTL_KIND_VERTEX     = 6,
  OCCTL_KIND_COMPOUND   = 7,
  OCCTL_KIND_COMPSOLID  = 8,
  OCCTL_KIND_COEDGE     = 9,
  OCCTL_KIND_PRODUCT    = 10,
  OCCTL_KIND_OCCURRENCE = 11,
  OCCTL_NODE_KIND_RESERVED_FUTURE = 0x7fffffff
} occtl_node_kind_t;

typedef struct occtl_node_id { uint64_t bits; } occtl_node_id_t;  /* transient */
typedef struct occtl_ref_id  { uint64_t bits; } occtl_ref_id_t;   /* transient */
typedef struct occtl_uid     { uint64_t bits; } occtl_uid_t;      /* persistent */

/* Layout (all four, private to src/topo/TopoMath.hxx):
 *   bits[63:56] : kind tag (occtl_*_kind_t cast to uint8_t)
 *   bits[55:48] : reserved (must be 0)
 *   bits[47:0]  : payload (index for node/ref/rep ids; counter for uid)
 *
 * The bit layout is a PRIVATE implementation detail; no static inline
 * helpers expose it in public headers.  To query the kind of an ID,
 * call the graph function (e.g. occtl_graph_node_kind) which
 * internally unpacks to the OCCT entity type and reads its Kind().
 */

#define OCCTL_NODE_ID_INVALID (occtl_node_id_t{0})
#define OCCTL_REF_ID_INVALID  (occtl_ref_id_t{0})
#define OCCTL_REP_ID_INVALID  (occtl_rep_id_t{0})
#define OCCTL_UID_INVALID     (occtl_uid_t{0})
```

### 3.0a All-zero invalid

The ABI uses **all-zero bits** as the invalid sentinel, so default-initialised values (Python `if not id:`, C# `default(struct)`, `calloc`, `memset(0)`) are invalid. OCCT's `BRepGraph_NodeId::Typed` uses `Index = UINT32_MAX` internally; `src/topo/TopoMath.hxx` translates between the two and is the only file that knows both.

### 3.1 Stability — read this carefully

| Type | Stability | Use for |
| --- | --- | --- |
| `occtl_node_id_t` | **Transient.** Invalidated by `Compact()` on the graph. Live only within a session. | Loops, neighbor queries, anything within a short scope. |
| `occtl_ref_id_t` | **Transient.** Invalidated by `Compact()`. | Addressing reference entries (CoEdge → Wire bindings, etc.) within a session. |
| `occtl_uid_t` | **Persistent.** Survives `Compact()`, survives node removal in many cases. | Wire format, external references (a UI's "selected face"), change history, persistence. |

The wrapper makes the distinction visible everywhere a binding might confuse them. The C++ veneer enforces it at the type system level.

### 3.2 Conversions

```c
OCCTL_API occtl_status_t occtl_graph_node_id_from_uid(
    const occtl_graph_t* g, occtl_uid_t uid, occtl_node_id_t* out);
OCCTL_API occtl_status_t occtl_graph_uid_from_node_id(
    const occtl_graph_t* g, occtl_node_id_t id, occtl_uid_t* out);
```

`occtl_graph_node_id_from_uid` is the canonical "I held a UID across an operation; what's its current node id?" call. Returns `OCCTL_NOT_FOUND` if the entity has been removed.

## 4. The graph object

```c
typedef struct occtl_graph occtl_graph_t;

OCCTL_API occtl_status_t occtl_graph_create(occtl_graph_t** out);

OCCTL_API void occtl_graph_free(occtl_graph_t* g);

/* Pending — not yet shipped:
OCCTL_API occtl_status_t occtl_graph_clone(
    const occtl_graph_t* src, occtl_graph_t** out);
OCCTL_API occtl_status_t occtl_graph_compact(occtl_graph_t* g);
*/
```

**`compact` invalidates all `occtl_node_id_t` and `occtl_ref_id_t` values.** UIDs survive. Bindings should expose this loudly; the C++ veneer offers `Graph::compact()` as an explicit, named call (no implicit invalidation).

A graph holds:

- A set of definitions per node kind.
- Reference entries (usages) per (Definition, parent) pair.
- A representation store (geometry data, deduplicated by handle pointer).
- A registry of layers (color, name, plus user-defined).
- A reverse index map for parent/child queries.
- A `TopoDS_Shape` cache for round-trip algorithms (rebuilt on demand).

## 5. Construction paths into a graph

The user never sees `TopoDS`. Every entry point that creates a graph or adds to one terminates in a node id, not a shape:

```c
/* From files. The I/O modules return graphs. */
OCCTL_API occtl_status_t occtl_io_brep_read(
    const char* path, occtl_graph_t** out, occtl_node_id_t* out_root);
OCCTL_API occtl_status_t occtl_io_step_read(
    const char* path, occtl_graph_t** out, occtl_node_id_t* out_root);
/* … same for iges/stl/obj/gltf/vrml/ply. */

/* From primitives. */
OCCTL_API occtl_status_t occtl_prim_box(
    occtl_graph_t* g,
    const occtl_prim_box_info_t* info,
    occtl_node_id_t* out_solid);

/* From low-level builders (graph-native). */
OCCTL_API occtl_status_t occtl_topo_make_vertex(
    occtl_graph_t* g, const occtl_topo_make_vertex_info_t* info,
    occtl_node_id_t* out_vertex);
OCCTL_API occtl_status_t occtl_topo_make_edge(
    occtl_graph_t* g, const occtl_topo_make_edge_info_t* info,
    occtl_node_id_t* out_edge);
/* … wire, face, shell, solid. */
```

Internally, primitives build a `TopoDS_Shape` via `BRepPrimAPI_*` and feed it into `BRepGraph::ShapesView::Add`. That's an implementation detail — the public path is graph-native.

## 6. Egress paths from a graph

```c
/* To files. */
OCCTL_API occtl_status_t occtl_io_brep_write(
    const occtl_graph_t* g, occtl_node_id_t root, const char* path);
/* … step/iges/stl/obj/gltf/vrml/ply. */

/* To explicit data via the query API (vertex points, edge ranges, surface
 * evaluations, triangulation views). No TopoDS exposed. */
```

There is **no** public `occtl_graph_to_topods()` function. C++ consumers who genuinely need a `TopoDS_Shape` pull it from an internal-only header that is not part of the installed public surface; bindings cannot reach it.

## 7. Geometry access (analogue of `BRep_Tool` / `BRepGraph_Tool`)

Read accessors keyed by node id live in the **`topo` module** (they wrap `BRepGraph_Tool`, not `Geom_*`), so the prefix is `occtl_topo_*`. Examples taken from the shipped header:

```c
/* Vertex */
OCCTL_API occtl_status_t occtl_topo_vertex_point(
    const occtl_graph_t*, occtl_node_id_t vertex, occtl_point3_t* out);
OCCTL_API occtl_status_t occtl_topo_vertex_tolerance(
    const occtl_graph_t*, occtl_node_id_t vertex, double* out);

/* Edge */
OCCTL_API occtl_status_t occtl_topo_edge_range(
    const occtl_graph_t*, occtl_node_id_t edge,
    double* out_first, double* out_last);
OCCTL_API occtl_status_t occtl_topo_edge_eval(
    const occtl_graph_t*, occtl_node_id_t edge,
    double u, occtl_point3_t* out);
OCCTL_API occtl_status_t occtl_topo_edge_eval_d1(
    const occtl_graph_t*, occtl_node_id_t edge,
    double u, occtl_point3_t* out_p, occtl_vector3_t* out_d1);
OCCTL_API occtl_status_t occtl_topo_edge_is_degenerated(
    const occtl_graph_t*, occtl_node_id_t edge, int32_t* out);

/* CoEdge */
OCCTL_API occtl_status_t occtl_topo_coedge_pcurve_eval(
    const occtl_graph_t*, occtl_node_id_t coedge,
    double u, occtl_point2_t* out);
OCCTL_API occtl_status_t occtl_topo_coedge_is_seam(
    const occtl_graph_t*, occtl_node_id_t coedge, int32_t* out);

/* Face */
OCCTL_API occtl_status_t occtl_topo_face_uv_bounds(
    const occtl_graph_t*, occtl_node_id_t face,
    double* out_umin, double* out_umax,
    double* out_vmin, double* out_vmax);
OCCTL_API occtl_status_t occtl_topo_face_eval(
    const occtl_graph_t*, occtl_node_id_t face,
    double u, double v, occtl_point3_t* out);
```

For point-evaluation queries the typed POD output (`occtl_point3_t`, `occtl_vector3_t`) is a value-in / value-out shape — cheap to marshal across every binding.

When a curve or surface needs to be evaluated repeatedly (offset, intersection trace, sampling), the `geom` module exposes opaque `occtl_curve_t*` / `occtl_curve2d_t*` / `occtl_surface_t*` handles with kind-tagged extractors. They are part of the default-on `geom` module — see [`MODULES.md §4`](./MODULES.md) and `include/occtl/occtl_curves.h` / `_curves2d.h` / `_surfaces.h`. The same handles feed `occtl_topo_make_edge` (3D curve) and `occtl_topo_make_face` (surface).

## 8. Mesh / triangulation access

```c
typedef struct occtl_triangulation_view {
  const double*   nodes;          /* xyz, length 3*node_count */
  size_t          node_count;
  const double*   normals;        /* may be NULL if absent */
  const double*   uvs;            /* may be NULL if absent */
  const uint32_t* indices;        /* triplets, length 3*triangle_count */
  size_t          triangle_count;
  occtl_uid_t     source_uid;     /* the face this view came from */
} occtl_triangulation_view_t;

OCCTL_API occtl_status_t occtl_mesh_face_triangulation(
    const occtl_graph_t* g,
    occtl_node_id_t      face,
    occtl_triangulation_view_t* out);
```

Zero-copy. Lifetime: valid until the graph is freed or the face's triangulation is replaced. A binding consumer wraps this in NumPy / `Span<T>` / `TypedArray` directly.

For 3D-edge polygons (curve approximations), `occtl_mesh_edge_polygon3d` returns a similar view. For polygons-on-triangulation (shared between adjacent faces), `occtl_mesh_face_polygon_on_tri` is provided.

## 9. Mutation API shape

The `BRepGraph::EditorView` exposes 40+ low-level mutators. We do **not** mirror them 1:1 in C — that would explode the surface and leak invariants. Instead the wrapper offers:

### 9.1 High-level builders

```c
OCCTL_API occtl_status_t occtl_topo_make_vertex(
    occtl_graph_t*, const occtl_topo_make_vertex_info_t*, occtl_node_id_t* out);
OCCTL_API occtl_status_t occtl_topo_make_edge(
    occtl_graph_t*, const occtl_topo_make_edge_info_t*, occtl_node_id_t* out);
OCCTL_API occtl_status_t occtl_topo_make_wire(
    occtl_graph_t*, const occtl_topo_make_wire_info_t*, occtl_node_id_t* out);
OCCTL_API occtl_status_t occtl_topo_make_face(
    occtl_graph_t*, const occtl_topo_make_face_info_t*, occtl_node_id_t* out);
OCCTL_API occtl_status_t occtl_topo_make_shell(
    occtl_graph_t*, const occtl_topo_make_shell_info_t*, occtl_node_id_t* out);
OCCTL_API occtl_status_t occtl_topo_make_solid(
    occtl_graph_t*, const occtl_topo_make_solid_info_t*, occtl_node_id_t* out);

OCCTL_API occtl_status_t occtl_topo_remove(
    occtl_graph_t*, occtl_node_id_t id);
```

Each `*_info_t` follows the [options-struct convention](./ABI_PATTERNS.md#8-options--info-structs) — versioned, with `p_next`, with `_INIT` macro.

### 9.2 Batched mutations

Multiple mutations should run inside a deferred scope. This maps onto `BRepGraph_DeferredScope` and avoids per-call subtree-generation propagation:

```c
typedef struct occtl_batch occtl_batch_t;

OCCTL_API occtl_status_t occtl_graph_begin_batch(
    occtl_graph_t* g, occtl_batch_t** out);
OCCTL_API occtl_status_t occtl_batch_commit(occtl_batch_t* b);
OCCTL_API occtl_status_t occtl_batch_abort(occtl_batch_t* b);
```

While a batch is open, mutating calls on the graph defer cache invalidation; commit applies it once. Abort discards uncommitted changes (where the underlying mutation is reversible — documented per call). Outside a batch, every mutation is its own implicit scope.

## 10. Layers and metadata (no OCAF needed)

BRepGraph's Layer system is the answer to "where do I attach color/name/custom data" without OCAF. The wrapper exposes:

### 10.1 Built-in layers

```c
typedef struct occtl_color_rgba {
  float r, g, b, a;
} occtl_color_rgba_t;

OCCTL_API occtl_status_t occtl_graph_color_set(
    occtl_graph_t* g, occtl_node_id_t target, occtl_color_rgba_t color);
OCCTL_API occtl_status_t occtl_graph_color_get(
    const occtl_graph_t* g, occtl_node_id_t target, occtl_color_rgba_t* out);
OCCTL_API occtl_status_t occtl_graph_color_unset(
    occtl_graph_t* g, occtl_node_id_t target);

OCCTL_API occtl_status_t occtl_graph_name_set(
    occtl_graph_t* g, occtl_node_id_t target,
    const char* name, size_t name_len);
OCCTL_API occtl_status_t occtl_graph_name_get(
    const occtl_graph_t* g, occtl_node_id_t target,
    char* buf, size_t buf_size, size_t* out_required);
```

Color and name are first-class because they're the metadata everyone needs. They live in BRepGraph layers under the hood, not in side maps on `occtl_graph_t`.

Layer placement rules:

- Persistent user metadata belongs in a BRepGraph layer when it is attached to a `NodeId` / `RefId`, must follow removal / replacement / `Compact()`, or must eventually serialize with a graph snapshot. Name, color, material, units, joints, tags, and exchange labels follow this rule.
- Recomputable derived data belongs in `BRepGraph_TransientCache` / `BRepGraph_RefTransientCache` when it depends on graph contents and can be invalidated by `VersionStamp` / `SubtreeGen`. Bounding boxes, geometry classifications, mass-property summaries, selector acceleration data, and expensive relation-query auxiliaries follow this rule.
- ABI materialization buffers may stay wrapper-local when they exist only to present C-friendly memory, not because OCCT needs the data. Flattened mesh views are the current example: OCCT stores graph mesh data and OCCT-Light caches contiguous ABI buffers keyed by UID + graph stamp.
- Generic BRepGraph utilities that would help more than OCCT-Light should be implemented at the OCCT level. Prefer `BRepGraphAlgo_*`, graph layers in `TKBRep`, or graph cache kinds there over duplicating reusable topology / metadata algorithms in each wrapper module.

Do not add a dependency from the core `topo` module to data-exchange toolkits just to reuse a metadata layer. If a generic name/color/material layer only exists in a DE package, either keep an OCCT-Light-private layer as an ABI bridge or upstream/move the generic layer into OCCT's BRepGraph package.

### 10.2 User-defined layers

Advanced API; documented in a separate "Extending OCCT-Light" guide. The shape:

```c
typedef occtl_status_t (OCCTL_CALL *occtl_graph_metadata_serialize_fn)(
    const void* attribute, char* out_buf, size_t buf_size,
    size_t* out_required, void* user_data);

typedef struct occtl_graph_metadata_create_info {
  uint32_t    struct_version;
  const void* p_next;
  const char* name;                     /* unique layer name */
  size_t      attribute_size;           /* bytes per attribute */
  occtl_graph_metadata_serialize_fn serialize;   /* optional, for I/O */
  void*       user_data;
} occtl_graph_metadata_create_info_t;
```

The wrapper translates layer callbacks (`OnNodeRemoved`, `OnCompact`, `OnNodeModified`) to C callbacks. The wrapper itself catches any error and logs it; layer callbacks must never propagate failure into OCCT (OCCT layer callbacks are `noexcept`).

## 11. Persistence

UIDs are the **wire format identity**. They survive `Compact()`, they survive most node removals, and they're exactly the right key to put into:

- Persistence (BRep file, custom format).
- External references (a UI's "selected face this morning" still resolves this afternoon).
- Change history ("face UID 0x0500…3a was modified between rev 12 and rev 13").
- Diffing two graphs.

Node ids are **session-local**. They're cheap to use within a function or a frame's render loop, but they must not leave the wrapper's scope. The conversion API today is `occtl_graph_node_id_from_uid` / `occtl_graph_uid_from_node_id` (§3.2), with `occtl_graph_uid_table` and `occtl_uid_to_bytes` / `occtl_uid_from_bytes` for a stable UID table snapshot. Reference entries have the same split: `occtl_ref_id_t` is session-local, while `occtl_ref_uid_t` resolves through `occtl_graph_ref_id_from_ref_uid` / `occtl_graph_ref_uid_from_ref_id` and can be bulk-dumped with `occtl_graph_ref_uid_table`. File-format-level UID persistence is still format-specific work for the I/O modules.

## 12. The TopoDS round-trip protocol (internal)

For algorithms that don't yet have BRepGraph-native variants — Booleans, fillets, offsets, pipe operations — the wrapper round-trips:

1. Resolve the input nodes' current `TopoDS_Shape` via `BRepGraph::Shapes().Shape(rootId)` (cached).
2. Run the OCCT algorithm (`BRepAlgoAPI_Fuse`, etc.). Check `IsDone()` and translate to a status.
3. Re-build via `graph.Shapes().Add(resultShape, opts)` to merge the result back into the graph.
4. Return new node ids. Where the algorithm provides modified/generated/deleted provenance, absorb it into the destination graph's history store.

The user's experience: graph in, graph(s) out, no `TopoDS` mentioned. The wrapper documents per algorithm whether the original graph receives the result merged in (preferred) or whether a fresh graph is returned.

History API. Booleans and selected topology/feature operations record history
on the graph that receives the operation result. Booleans merge their result
back into the **same** graph the inputs came from (in-place semantics — simpler
for callers and avoids the NodeId-foreign-graph problem). The full public
surface lives on [`include/occtl/occtl_topo.h`](../../include/occtl/occtl_topo.h)
and [`include/occtl/occtl_bool.h`](../../include/occtl/occtl_bool.h):

```c
OCCTL_API occtl_status_t occtl_bool_fuse(
    occtl_graph_t*               graph,                /* in-place merge target */
    const occtl_node_id_t*       objects, size_t n_objects,
    const occtl_node_id_t*       tools,   size_t n_tools,
    const occtl_bool_options_t*  opts,
    occtl_node_id_t*             out_root);

OCCTL_API occtl_status_t occtl_graph_history_modified(
    const occtl_graph_t* graph, occtl_uid_t input_uid,
    occtl_uid_t* out_buf, size_t cap, size_t* out_count);
OCCTL_API occtl_status_t occtl_graph_history_generated(
    const occtl_graph_t* graph, occtl_uid_t input_uid,
    occtl_uid_t* out_buf, size_t cap, size_t* out_count);
OCCTL_API occtl_status_t occtl_graph_history_deleted_all(
    const occtl_graph_t* graph,
    occtl_uid_t* out_buf, size_t cap, size_t* out_count);
```

History uses UIDs (persistent), not NodeIds (transient), because it's by
definition crossing operation boundaries. Each accessor follows the
two-call buffer pattern (`ABI_PATTERNS.md §10`). The deleted set is
collection-wide — input UIDs do not key into it; callers iterate the
returned UID array directly.

Internally, the wrapper absorbs OCCT-side provenance
(`BRepGraph_History` + `BRepAlgoAPI_HistoryAdapter::Absorb`) into the graph's
own `BRepGraph_History` instance. No separate public history handle exists. The
section 12 protocol ("The TopoDS round-trip protocol (internal)") remains the
source of truth for which algorithms still round-trip
through TopoDS; the in-place-merge signature is the public face of that
protocol.

## 13. Known rough edges and how we handle them

| Issue | Approach |
| --- | --- |
| Boolean ops, fillets, offsets round-trip through TopoDS. | Documented as implementation detail. Will move to graph-native when `BRepGraphAlgo` gains support. The public API does not change when that happens. |
| CoEdge / seam semantics surprise users from a TopoDS background. | We don't hide CoEdges — they're part of the value of BRepGraph's half-edge model. We document them in the user guide with diagrams. The query API names them clearly (`occtl_topo_coedge_*`). |
| Layer callbacks must be `noexcept` per OCCT contract. | Wrapper enforces by translating any C-callback exception or non-OK status into a logged warning; layers never see failures. Documented as part of the layer registration contract. |
| `Compact()` invalidating all NodeIds is a footgun. | The C ABI is explicit: `occtl_graph_compact` is named, never implicit, and documented as invalidating. The C++ veneer wraps it as `Graph::compact()`. UIDs survive. |
| Definition vs Usage distinction is unfamiliar. | Most queries hide it (the high-level API operates on instance-level facts). Where it matters (e.g., "how many places does this surface appear?"), we expose it explicitly via `occtl_topo_definition_*` accessors. |

## 14. Summary

A 64-bit kind-tagged identity reaches every binding without translation, UIDs persist across operations and serialization without OCAF, adjacency is O(1), and metadata lives on layers. The internal TopoDS round-trip is the price; hiding it from users is the design.
