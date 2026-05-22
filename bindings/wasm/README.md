# @occtl/wasm — OCCT-Light WASM binding

> **Licensed under AGPL-3.0-or-later.** Linking this package into network-facing software places that software under the AGPL. Read [`LICENSE_AGPL_30.txt`](../../LICENSE_AGPL_30.txt) before redistributing.

The WASM binding compiles [OCCT-Light](../../README.md) into a single `.wasm` module loadable from modern browsers and Node 20+.

## Status

**Phase 1.** Source tree is complete and generator/TypeScript passes. Current blocker is wasm-native linkage: the Emscripten build reaches link and fails on unresolved `occtl_*` symbols unless a wasm-compatible OCCT-Light archive is supplied.

The TypeScript surface is fully typed and `tsc` checks pass without a running WASM bundle.

## ⚠️ HEAPF64 view invalidation — read before using array views

WebAssembly linear memory **can be reallocated at any time**. When that happens, every existing `Float64Array` / `Uint32Array` view into `Module.HEAPF64` (or any other `HEAP*` typed array) **silently points at freed memory**. Reading from a stale view is undefined behaviour.

OCCT-Light's WASM binding solves this by **invalidating views before every C call**:

- Methods that *return* a span give you a `HeapView<T>` (`src/ts/views.ts`), not a raw `Float64Array`.
- The view registers itself with the module-global invalidator.
- The next OCCT-Light call (any function that may allocate WASM memory) invalidates all live views.
- Accessing a stale view throws `WasmHeapInvalidatedError` — by contrast with raw `Float64Array` access, which would silently corrupt.
- Call `view.copy()` immediately to materialise an owned `Float64Array` on the JS heap if you need to outlive the next OCCT-Light call.

**This is the #1 footgun on this binding.** Treat span returns as one-shot reads.

## Layout

```
bindings/wasm/
├── package.json
├── tsconfig.json
├── CMakeLists.txt              ← orchestrator: regenerate + emcmake invocation targets
├── src/
│   ├── native/
│   │   ├── CMakeLists.txt      ← Emscripten build entry; invoked under emcmake
│   │   ├── main.cc             ← Module init wiring
│   │   ├── error.cc            ← status → exception translation
│   │   ├── handles.cc          ← class_<>() bindings for opaque types
│   │   └── raw.cc              ← generated; Embind glue per ABI function
│   └── ts/
│       ├── index.ts            ← public surface (mirrors @occtl/node shape)
│       ├── _raw.ts             ← raw Embind module typings (private)
│       ├── abi.ts              ← Occtl.load() — ABI handshake
│       ├── errors.ts           ← 17 status-code subclasses
│       ├── ids.ts              ← branded NodeId / RefId / Uid / RepId
│       ├── handles.ts          ← Disposable wrappers
│       ├── views.ts            ← HEAPF64View + invalidator
│       ├── core.ts / geom.ts / topo.ts / prim.ts / text.ts / curves.ts / curves2d.ts / surfaces.ts
├── emscripten.cmake            ← Emscripten-specific link/compile flags
├── scripts/setup.sh            ← installs emsdk locally
├── tools/
│   └── generate_facade.ts      ← reads ../../build/abi.json → emits src/native/raw.cc + src/ts/*.ts
└── tests/
    ├── smoke.test.ts
    ├── coverage.test.ts        ← every occtl_* function is wrapped
    └── parity/runner.ts        ← runs build_box.json
```

## Build

```bash
# 1. Provision emsdk (one-time, ~1 GB, several minutes).
./scripts/setup.sh
source ./scripts/emsdk_env.sh    # or whatever the script writes out

# 2. Refresh build/abi.json from the headers.
(cd ../.. && python3 tools/abi_dump.py --output build/abi.json)

# 3. Regenerate src/native/raw.cc and src/ts/*.ts.
npm install
npm run generate

# 4. Build WASM. This compiles the WASM bridge and requires wasm-compatible OCCT-Light objects.
emcmake cmake -S src/native -B build -DOCCTL_HEADERS_DIR=$(pwd)/../../include
cmake --build build -j

# 5. Run tests (requires dist/occtl.wasm).
npm test

# 6. Pack a distributable.
npm pack
```

## Usage

```ts
import { Occtl } from "@occtl/wasm";

const occtl = await Occtl.load();      // async — instantiates the WASM module

using graph = new occtl.Graph();        // explicit-resource-management; auto-frees
const v = graph.makeVertex({ x: 1, y: 2, z: 3 });
console.log("vertex id:", v);

const box = graph.makeBox({ dx: 10, dy: 10, dz: 5 });
const fused = graph.fuse([box], [box], { buildHistory: false });
const faces = [...graph.faces()];
console.log("faces:", faces.length);   // 6
```

Without `using` (older runtimes):

```ts
const graph = new occtl.Graph();
try {
  /* … */
} finally {
  graph.dispose();
}
```

## Error handling

Every non-OK C status maps to a typed JS exception. Catch the base class for everything, or a specific subclass for fine-grained handling:

```ts
import { OcctLError, NotDoneError } from "@occtl/wasm";

try {
  graph.makeBox({ dx: -1, dy: 1, dz: 1 });
} catch (e) {
  if (e instanceof NotDoneError) console.warn("OCCT bailed:", e.message);
  else if (e instanceof OcctLError) console.warn("Other failure:", e.status, e.message);
  else throw e;
}
```

Subclass names are aligned with the native JS/TS facade so user code can stay portable.

## Threading

Each Web Worker / process needs its own WASM Module instance. **Do not share a `Module` across workers.** The C ABI's thread-local error slot is per-instance.

## Where the C contract lives

- [`docs/design/BINDINGS.md`](../../docs/design/BINDINGS.md) §4 (uniform contract) and §5.4 (WASM specifics).
- [`docs/design/ABI_PATTERNS.md`](../../docs/design/ABI_PATTERNS.md) — handles, errors, options, spans, iterators.
- [`build/abi.json`](../../build/abi.json) — machine-readable ABI dump that drives `tools/generate_facade.ts`.

## Anti-patterns

- **Don't share `Module` across Web Workers.**
- **Don't cache `HeapView<T>` across calls.** Use `.copy()` immediately.
- **Don't forget `.delete()` / `using` on handles.** Embind handles do *not* GC; forgetting leaks WASM linear memory.
- **Don't hand-edit `src/native/raw.cc` or generated `src/ts/*.ts` blocks.** Re-run `npm run generate` after ABI changes.
- **Don't treat missing `dist/occtl.wasm` as an acceptable pass state.** Smoke/parity are fail-fast and should error until artifacts are built.
