# OCCT-Light — Module Charter

This document lists the maintained OCCT-Light modules, their purpose, and build toggles.
The source of truth for module registration is `cmake/OCCTLRegistry.cmake`.

Companion docs: [ARCHITECTURE.md](./ARCHITECTURE.md) · [ABI_PATTERNS.md](./ABI_PATTERNS.md) · [BREPGRAPH_AS_CANONICAL.md](./BREPGRAPH_AS_CANONICAL.md) · [BINDINGS.md](./BINDINGS.md).

## 1. Dependency shape

- `core` is always enabled.
- `geom` and `topo` are enabled by default.
- Other modules are opt-in via `OCCTL_BUILD_<MODULE>` options.

## 2. Implemented modules

### `core` (always on)
- Purpose: runtime lifecycle, status/error model, base POD types.
- Option: implicit (always linked).
- Headers: `occtl_core.h`, umbrella `occtl.h`.

### `geom` (default ON)
- Purpose: geometric POD operations, curve/curve2d/surface representation APIs.
- Option: `OCCTL_BUILD_GEOM` (ON).
- Headers: `occtl_geom.h`, `occtl_curves.h`, `occtl_curves2d.h`, `occtl_surfaces.h`.

### `topo` (default ON)
- Purpose: BRepGraph lifecycle, identity, topology queries, iterators, builders.
- Option: `OCCTL_BUILD_TOPO` (ON).
- Headers: `occtl_topo.h`, `occtl_topo_types.h`, `occtl_topo_build.h`, `occtl_topo_relation.h`.

### `prim` (default OFF)
- Purpose: primitive and feature construction (solid/sweep/sketch/feature families).
- Option: `OCCTL_BUILD_PRIM` (OFF).
- Headers: `occtl_prim.h`, `occtl_prim_solid.h`, `occtl_prim_sketch.h`, `occtl_prim_sweep.h`, `occtl_prim_feature.h`.

### `text` (default OFF)
- Purpose: text measurement and text-to-shape topology generation.
- Option: `OCCTL_BUILD_TEXT` (OFF).
- Header: `occtl_text.h`.

### `bool` (default OFF)
- Purpose: boolean operations (`fuse`, `cut`, `common`, `section`, `split`) with history.
- Option: `OCCTL_BUILD_BOOL` (OFF).
- Header: `occtl_bool.h`.

### `mesh` (default OFF)
- Purpose: triangulation generation and mesh views/buffers.
- Option: `OCCTL_BUILD_MESH` (OFF).
- Header: `occtl_mesh.h`.

### `heal` (default OFF)
- Purpose: healing and same-domain unification.
- Option: `OCCTL_BUILD_HEAL` (OFF).
- Header: `occtl_heal.h`.

### `io_brep` (default OFF)
- Purpose: native BRep read/write.
- Option: `OCCTL_BUILD_IO_BREP` (OFF).
- Header: `occtl_io_brep.h`.

### `io_step` (default OFF)
- Purpose: STEP read/write.
- Option: `OCCTL_BUILD_IO_STEP` (OFF).
- Header: `occtl_io_step.h`.

### `io_iges` (default OFF)
- Purpose: IGES read/write.
- Option: `OCCTL_BUILD_IO_IGES` (OFF).
- Header: `occtl_io_iges.h`.

### `io_stl` (default OFF)
- Purpose: STL read/write.
- Option: `OCCTL_BUILD_IO_STL` (OFF).
- Header: `occtl_io_stl.h`.

### `de` (default OFF)
- Purpose: unified data-exchange dispatch and format routing.
- Option: `OCCTL_BUILD_DE` (OFF).
- Header: `occtl_de.h`.

### `io_obj` (default OFF)
- Purpose: Wavefront OBJ read/write.
- Option: `OCCTL_BUILD_IO_OBJ` (OFF).
- Header: `occtl_io_obj.h`.

### `io_gltf` (default OFF)
- Purpose: glTF 2.0/GLB read/write.
- Option: `OCCTL_BUILD_IO_GLTF` (OFF).
- Header: `occtl_io_gltf.h`.

### `io_vrml` (default OFF)
- Purpose: VRML read/write.
- Option: `OCCTL_BUILD_IO_VRML` (OFF).
- Header: `occtl_io_vrml.h`.

### `io_ply` (default OFF)
- Purpose: PLY exchange support.
- Option: `OCCTL_BUILD_IO_PLY` (OFF).
- Header: `occtl_io_ply.h`.

### `viz` (default OFF)
- Purpose: interactive/offscreen visualization and picking.
- Option: `OCCTL_BUILD_VIZ` (OFF).
- Header: `occtl_viz.h`.

## 3. Preset mapping (current)

- `core-only`: `core`
- `geom-only`: `core + geom`
- `minimal`: `core + geom + topo + prim`
- `cad`: `minimal + bool + mesh + heal + text + io_brep + io_step + io_stl + de`
- `full`: `cad + io_iges + io_obj + io_gltf + io_vrml + io_ply`
- `full-with-viz`: `full + viz`

## 4. Notes

- Public C ABI headers never expose OCCT or STL types.
- `TopoDS_*` remains internal-only; public topology is graph + ids.
- Module docs should be updated alongside `cmake/OCCTLRegistry.cmake` changes.
