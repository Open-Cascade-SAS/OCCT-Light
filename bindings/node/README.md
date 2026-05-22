# occtl — Node.js binding for OCCT-Light

**AGPL-3.0-or-later.** Linking this package into published software makes that software AGPL-covered. Read [`LICENSE_AGPL_30.txt`](../../LICENSE_AGPL_30.txt) before you redistribute.

A Node.js N-API addon over the OCCT-Light C ABI — a modular wrapper around Open CASCADE Technology. The binding follows the uniform contract described in [`docs/design/BINDINGS.md`](../../docs/design/BINDINGS.md) §4 and the Node-specific plan in §5.3.

## Status

Tracks the current `occtl_*.h` ABI through generated raw N-API trampolines, generated typed TypeScript helpers, and a small hand-written ergonomic layer.

## Install

```sh
npm install occtl
```

Prebuilt binaries are shipped for `linux-x64`, `linux-arm64`, `macos-arm64`, `macos-x64`, `win-x64`. Falls back to a `node-gyp` source build if the prebuild is missing.

## Quick start

```ts
import { Graph } from "occtl";

using g = new Graph();
const solid = g.makeBox({ dx: 10, dy: 10, dz: 5 });
console.log([...g.faces()].length); // 6
```

The `using` declaration is the ECMAScript explicit-resource-management form (`Symbol.dispose`); a `.close()` method is also provided.

## Build from source

Requires:

- Node.js 18 or newer (LTS recommended).
- The host OCCT-Light feature-set shared library (`libocctl-<feature-set>.{so,dylib,dll}`) built with `-DOCCTL_SHARED_LIBS=ON`.
- A C++17 toolchain.

```sh
cd bindings/node
npm install
npm run generate     # regenerates src/native/raw.cc and generated src/ts/generated/<module>.ts files
npm run build        # node-gyp + tsc
npm test
```

The generator reads `../../build/abi.json` (produced by `tools/abi_dump.py`); regenerate it whenever a public header changes.

## Architecture

```
src/ts/ (idiomatic TS surface)
  index.ts        public re-exports
  abi.ts          ABI version handshake + runtimeInit
  errors.ts       OcctLError + per-status subclasses
  ids.ts          branded types: NodeId, Uid, RefId, RepId
  handles.ts      Disposable wrappers (Graph, iterators, ...)
  core.ts, geom.ts, topo.ts, prim.ts, text.ts (generated raw/typed modules)
  _raw.ts         private addon type declarations

src/native/ (N-API addon, C++17)
  addon.cc          Napi entry point
  raw.cc            generated: one Napi::Function per OCCTL_API
  handle_wrapper.{cc,h}  ObjectWrap<T> for each opaque type
  error.cc          status-code -> JS exception translation
  span.cc           Napi::External over §10.2 spans
  iter.cc           iterator-handle ObjectWrap
```

The `_raw` surface is **private**. Userland never sees `Napi`-typed handles.

## License

[AGPL-3.0-or-later](../../LICENSE_AGPL_30.txt).
