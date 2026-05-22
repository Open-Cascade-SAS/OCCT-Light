# OCCT-Light

> A modular C-ABI wrapper around [Open CASCADE Technology](https://dev.opencascade.org/), designed as the canonical bridge for Python, C#, JS/TS, WASM, Rust, Go, Java, and any language with a C foreign-function interface.

OCCT-Light exposes a **pure C ABI** — opaque handles, POD structs, error codes, no STL or OCCT types in public headers — with an optional **header-only C++ veneer** for ergonomic C++ consumption (RAII handles, exceptions translated from status codes). It uses **BRepGraph** (OCCT's modern incidence-table topology) as the canonical shape representation, not `TopoDS`. Modules are toggled at CMake configure time. The project is still pre-release, so public API breaks are allowed when they make the final headless CAD interface cleaner.

## Why this exists

OCCT is a powerful C++ CAD library, but its public API is C++. Cross-language integrations often need repeated type translation and per-language maintenance.

OCCT-Light addresses that with one hand-written, reviewed C ABI consumed by every language facade.

See the design docs in [`docs/design/`](docs/design/) for the full rationale.

## Design documents

Read these in order:

1. [`ARCHITECTURE.md`](docs/design/ARCHITECTURE.md) — big-picture overview, layering, repository layout, build model, threading, versioning.
2. [`ABI_PATTERNS.md`](docs/design/ABI_PATTERNS.md) — the ABI rulebook: handles, errors, strings, sequences, iterators, options, allocators, callbacks, C++ veneer rules.
3. [`BREPGRAPH_AS_CANONICAL.md`](docs/design/BREPGRAPH_AS_CANONICAL.md) — why BRepGraph (not TopoDS) is the topology, the C representation of NodeId/UID/RefId, the invisible TopoDS round-trip protocol.
4. [`MODULES.md`](docs/design/MODULES.md) — per-module charter: scope, OCCT toolkits linked, public headers, CMake options, dependencies.
5. [`CODING_STYLE.md`](docs/design/CODING_STYLE.md) — public-C vs internal-C++ style split, Doxygen conventions, file layout.
6. [`BINDINGS.md`](docs/design/BINDINGS.md) — binding strategy: the uniform recipe every per-language facade (Python, C#, JS/TS, Rust, Go, Java, Swift) follows over the C ABI, packaging, test parity, and the checklist for adding a new language.

## Quick build

OCCT-Light requires an existing OCCT installation. Point CMake at it via `OCCT_DIR`:

```bash
cmake --preset minimal -DOCCT_DIR=/path/to/occt/install/lib/cmake/opencascade
cmake --build --preset minimal
ctest --preset minimal
```

Available presets:

| Preset | Modules | Use case |
| --- | --- | --- |
| `core-only` | core | Foundation iteration |
| `geom-only` | core + geom | Iterating on geom |
| `minimal` | core + geom + topo + prim | Headless geometric modeling, smallest binary |
| `cad` | minimal + bool + mesh + heal + text + io_brep + io_step + io_stl + de | Typical CAD workflow without viz |
| `full` | cad + io_iges + io_obj + io_gltf + io_vrml + io_ply | Everything implemented that doesn't need OS graphics deps |
| `full-with-viz` | full + viz | Interactive desktop CAD frontend |

Each configured feature set now builds one physical library: `libocctl-core`, `libocctl-geom`, `libocctl-minimal`, `libocctl-cad`, `libocctl-full`, or `libocctl-full-viz`. The module targets (`OCCTL::topo`, `OCCTL::mesh`, etc.) remain as CMake compatibility/component targets that point at that single library. Planned modules fail configuration if enabled before their implementation lands. CMake also emits `OCCTLFeatures.json` beside the package config so bindings and downstream tools can discover the selected library, enabled components, headers, and OCCT toolkit baseline.

## API docs (Doxygen)

Build HTML docs:

```bash
./docs/scripts/build-doxygen-docs.sh
```

Open:

- `docs/api/html/index.html` (main HTML site)
- `docs/api/doxygen-warnings.log` (Doxygen warnings report)

## Public API at a glance

A graph can be built by stacking the topo builders (`occtl_topo_make_vertex` / `_edge` / `_wire` / `_face` / …) or loaded through an enabled I/O module. Once you have one, count its faces:

```c
#include <occtl/occtl.h>

void print_face_count(const occtl_graph_t* g) {
  occtl_node_iter_t* it = NULL;
  if (occtl_graph_face_iter_create(g, &it) != OCCTL_OK)
    return;

  size_t          n = 0;
  occtl_node_id_t id;
  while (occtl_node_iter_next(it, &id) == OCCTL_OK)
    ++n;
  occtl_node_iter_free(it);

  printf("faces: %zu\n", n);
}
```

Same loop in the C++ veneer (range-for, RAII, exceptions on non-OK status):

```cpp
#include <occtl-hpp/occtl.hpp>

void print_face_count(const occtl::Graph& g) {
  std::size_t n = 0;
  for (occtl::NodeId face : g.faces())
    ++n;
  std::cout << "faces: " << n << '\n';
}
```

## Quick start per language

The binding trees consume the same C ABI. Each follows the uniform contract in [`BINDINGS.md §4`](docs/design/BINDINGS.md) — load-time ABI handshake, typed errors, RAII handles, strong nominal IDs, versioned options, native iterators, zero-copy spans.

```python
# Python — pip install occtl
import occtl
with occtl.Graph() as g:
    box = occtl.prim.make_box(g, dx=10.0, dy=10.0, dz=5.0)
    print("faces:", g.nb_faces, "edges:", g.nb_edges, "vertices:", g.nb_vertices)
```

```csharp
// C# — dotnet add package OcctL  (.NET 8+)
using OcctL;
using var g = new Graph();
NodeId box = Prim.MakeBox(g, dx: 10.0, dy: 10.0, dz: 5.0);
Console.WriteLine($"faces={g.NbFaces} edges={g.NbEdges} vertices={g.NbVertices}");
```

```ts
// Node — npm install occtl  (Node 20+)
import * as occtl from "occtl";
using g = new occtl.Graph();
const box = g.makeBox({ dx: 10, dy: 10, dz: 5 });
console.log("faces", g.nbFaces, "edges", g.nbEdges, "vertices", g.nbVertices);
```

```ts
// WASM (browser/Node) — npm install @occtl/wasm
import { Occtl } from "@occtl/wasm";
const occtl = await Occtl.load();
using g = new occtl.Graph();
const box = g.makeBox({ dx: 10, dy: 10, dz: 5 });
console.log("faces", g.nbFaces, "edges", g.nbEdges, "vertices", g.nbVertices);
```

```java
// Java (JNA)
try (org.occtl.Graph g = new org.occtl.Graph()) {
  org.occtl.Prim.makeBox(g, new org.occtl.BoxInfo(10.0, 10.0, 5.0));
  System.out.println("faces=" + g.faceCount() + " edges=" + g.edgeCount()
                     + " vertices=" + g.vertexCount());
}
```

Per-binding READMEs live under [`bindings/python/`](bindings/python/), [`bindings/csharp/`](bindings/csharp/), [`bindings/node/`](bindings/node/), [`bindings/wasm/`](bindings/wasm/), [`bindings/rust/`](bindings/rust/), [`bindings/go/`](bindings/go/), and [`bindings/java/`](bindings/java/). Swift is on the roadmap — see [`BINDINGS.md §5.5`](docs/design/BINDINGS.md).

For a unified cross-language workspace map and maintenance commands, see
`bindings/README.md`:

```bash
python3 tools/scripts/bindings.py overview
python3 tools/scripts/bindings.py audit
python3 tools/scripts/bindings.py clean --dry-run
```

## Community package preparation scripts

Use the script-first packaging helpers under [`tools/scripts/`](tools/scripts/) as the user-facing flow. Implementation stays in [`tools/packaging/`](tools/packaging/). CI/Actions, if used later, should only call these scripts:

```bash
# macOS / Linux
tools/scripts/build_binding_packages.sh --targets python-wheel,python-conda,csharp-nuget,node-npm,java-maven,rust-crate,go-module

# dry-run plan only
tools/scripts/build_binding_packages.sh --dry-run
```

```powershell
# Windows PowerShell
.\tools\scripts\build_binding_packages.ps1 --targets python-wheel,csharp-nuget,node-npm,java-maven,rust-crate,go-module
```

The script supports per-ecosystem targets:

- `python-wheel` and `python-conda`
- `csharp-nuget`
- `node-npm` and `wasm-npm`
- `java-maven`
- `rust-crate`
- `go-module`

Each run writes `build/binding-packages/binding-packages-manifest.json` with
artifact paths and SHA-256 checksums for delivery handoff.

Unified CMake tool wrapper:

```bash
cmake --build build/minimal --target occtl-bindings-packages-dry-run
cmake --build build/minimal --target occtl-bindings-packages
```

## Project layout

```
include/occtl/      Public C headers (extern "C")
include/occtl-hpp/  Public C++ veneer (header-only)
src/                Internal C++ implementation; calls OCCT directly
tests/              gtest suite per module
bindings/           Per-language facades (python, csharp, node, wasm, rust, go, java; swift later — see docs/design/BINDINGS.md)
tools/              ABI dump/schema, header-style checks, Guard audits
docs/design/        These design documents
cmake/              CMake helpers, find scripts, package config
```

## License

[AGPL-3.0-or-later](LICENSE_AGPL_30.txt).

OCCT itself is licensed under LGPL-2.1; OCCT-Light links it as an external dependency without bundling.
