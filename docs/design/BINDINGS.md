# OCCT-Light — Binding Strategy

> How OCCT-Light reaches Python, C#, JS/TS (Node and web), and every future language. The C ABI is the contract; this document is the **uniform recipe** every per-language facade follows so the cost of adding the seventh language is roughly the cost of the second.

Companion docs: [ARCHITECTURE.md](./ARCHITECTURE.md) · [ABI_PATTERNS.md](./ABI_PATTERNS.md) · [BREPGRAPH_AS_CANONICAL.md](./BREPGRAPH_AS_CANONICAL.md) · [MODULES.md](./MODULES.md) · [CODING_STYLE.md](./CODING_STYLE.md).

`ABI_PATTERNS.md §17` gives a one-line table; this document is the long-form. When in doubt about a single ABI shape, that doc wins; when a language-level question contradicts a binding shortcut, this doc wins.

---

## 1. Goals and non-goals

### Goals

1. **One uniform recipe.** Every language facade has the same shape: an auto-generated raw layer, an auto-generated typed facade for mechanically inferable wrappers, and a small hand-written ergonomic layer. Adding a language is a checklist (§8), not a research project.
2. **Zero translation drift.** The C ABI is the source of truth. The raw and typed layers are regenerated from `build/abi.json`; the ergonomic layer is hand-written but covered by parity and focused smoke tests so it cannot silently drift.
3. **Zero-copy on the hot paths.** §10.2 spans (mesh triangulation, B-spline poles) reach the binding's native array type (NumPy `ndarray`, .NET `Span<T>`, JS `TypedArray`) without an intermediate copy.
4. **No exceptions across the FFI.** Every status code becomes an idiomatic native error (Python exception, .NET exception, JS thrown error, Rust `Result`). Inverse on input — no native error type may leak into a callback that crosses back.
5. **Same vocabulary across languages.** Status codes, kind enums, and error messages are the same. A Python user reading a C# stack trace recognises the names.

### Non-goals

- Direct C++ binding surfaces. The C ABI is the binding interface.
- **Fully generated ergonomic APIs.** The raw FFI and complete typed facade are generated. The final user-facing layer still stays small and hand-written where language judgment matters: RAII/disposal, exceptions/errors, iterators, spans/views, builders, and names that match the host language.
- **Exposing the C++ veneer (`occtl-hpp`) to bindings.** The veneer is for C++ consumers. Bindings call the C ABI directly. The veneer's shape is a useful *reference* for what an idiomatic layer feels like, but it is never an FFI target.
- **Per-language extensions to the model.** A Python user does not get a Python-only `Graph.repair()` that has no analogue in C#. New behavior lands in the C ABI first, then propagates to every facade.

---

## 2. Practical principles

The binding strategy is intentionally simple:
- generate raw and typed layers from `build/abi.json`;
- keep the idiomatic layer small and hand-written;
- enforce parity and symbol coverage in tests;
- preserve one consistent shape across all languages.

---

## 3. Layering

Each language facade is three layers, named consistently:

```
┌──────────────────────────────────────────────┐
│  Idiomatic layer   (hand-written)            │
│  - Pythonic / .NETic / JS-native API         │
│  - RAII / disposable / closeable handles     │
│  - Native iterators / generators             │
│  - Native array views (NumPy / Span / TA)    │
│  - Native errors                             │
├──────────────────────────────────────────────┤
│  Typed facade     (auto-generated)           │
│  - option builders / dataclasses / records   │
│  - two-call strings and sequence helpers     │
│  - docs copied from C header metadata        │
├──────────────────────────────────────────────┤
│  Raw layer         (auto-generated)          │
│  - 1:1 with occtl_*.h                        │
│  - exposes only C primitive types            │
│  - thin generated FFI stubs                   │
├──────────────────────────────────────────────┤
│  Native feature-set library (libocctl-*.{so,dll}) │
│  - Public C ABI                              │
└──────────────────────────────────────────────┘
```

**The raw and generated typed layers are private.** Users `import occtl`, never `occtl._raw` or generated subpackages directly. Document generated layers as implementation details; do not stabilise them. This means a binding can switch its raw layer (e.g. cffi ABI mode → cffi API mode → cython → manylinux prebuilt) without a major-version bump as long as the idiomatic surface is unchanged.

**The idiomatic layer is the binding's stable surface.** It carries its own SemVer, independent from the wrapper SemVer. A new wrapper minor version may add a new C function; until the idiomatic layer surfaces it, the binding can ship its existing minor version unchanged.

### Layout in the repo

```
bindings/
├── python/             # cffi (ABI mode), pip / wheel
│   ├── src/occtl/      # package: ergonomic layer + generated typed/raw layers
│   │   ├── __init__.py
│   │   ├── _raw.py     # cffi cdef + dlopen — auto-generated
│   │   ├── _generated/ # generated 1:1 status-checking wrappers
│   │   ├── _typed/     # generated typed dataclasses/helpers
│   │   ├── _errors.py
│   │   ├── _handles.py
│   │   ├── core.py
│   │   ├── geom.py
│   │   └── topo.py
│   ├── tests/          # pytest; parity test imports the C-header symbol list
│   ├── stubs/          # PEP 561 .pyi (auto-generated when enabled)
│   └── pyproject.toml
├── csharp/             # P/Invoke, NuGet
│   ├── src/
│   │   ├── ffi/        # raw P/Invoke — auto-generated
│   │   └── api/        # ergonomic layer + generated typed facade
│   ├── tests/
│   │   ├── unit/       # xUnit
│   │   └── parity/     # parity runner
│   └── OcctL.sln
├── node/               # Node N-API addon
│   ├── src/
│   │   ├── native/     # C++ N-API glue
│   │   └── ts/         # TypeScript ergonomic + generated typed/raw modules
│   ├── tests/          # vitest
│   ├── binding.gyp
│   └── package.json
├── wasm/               # Emscripten WASM + Embind
│   ├── src/
│   │   ├── native/     # Embind glue + emcmake entrypoint
│   │   └── ts/         # TS facade (same shape as node/)
│   ├── tests/          # vitest, headless browser
│   └── package.json
├── rust/
│   ├── src/
│   │   ├── occtl-sys/  # bindgen, unsafe
│   │   └── occtl/      # safe
│   └── tests/parity_runner/
├── go/
│   ├── src/occtl/
│   ├── tests/parity_runner/
│   └── tools/
├── java/               # JNA raw + idiomatic facade
└── swift/
```

Node and WASM intentionally converge on the same public TypeScript shapes, but the shared type package is not split out yet. Keep new TS APIs portable between the two backends unless the runtime model makes that impossible.

---

## 4. The uniform binding contract

Every facade implements these ten responsibilities the same way. When implementing a new language facade, work down this list.

### 4.1 Load-time ABI version handshake

On first use, the facade calls:

```c
uint32_t v = occtl_runtime_abi_version();
if (v != OCCTL_ABI_VERSION) /* refuse */;
```

This must run **before** any other call. The expected ABI version is baked into the binding at build time (read from `occtl_core.h` during the raw-layer generation). On mismatch the binding raises a binding-specific `AbiMismatchError` carrying both versions; the binding does **not** attempt to proceed with a stale library — this is the failure mode that produces the worst, latest-bound crashes if ignored.

`occtl_runtime_init(NULL)` is then called once per process at first use. The binding stores a `bool _initialised` to keep it idempotent. `occtl_runtime_shutdown()` is **not** wired to module teardown by default — process exit handles it, and atexit hooks in dynamic loaders are a known source of crashes.

### 4.2 Error translation

Each non-OK status produced by a C call is converted at the **idiomatic-layer boundary**, never deeper. The pattern in every language:

```python
# Python
def _check(status):
    if status == _raw.OCCTL_OK:
        return
    err = _raw.occtl_error_last()
    raise Error(
        code=Status(status),
        message=ffi.string(err.message).decode('utf-8'),
        source=Uid(err.source.bits),
        extended=err.extended,
    )
```

```csharp
// C#
internal static void Check(OcctlStatus status) {
    if (status == OcctlStatus.Ok) return;
    var err = Marshal.PtrToStructure<OcctlError>(Native.occtl_error_last());
    throw new OcctLException(status, Marshal.PtrToStringUTF8(err.message), err.source, err.extended);
}
```

The error object carries **status, message, source UID, extended subcode** — the same four fields every language sees. The primary status is the language's `enum`; the extended code is an integer. Bindings never invent additional error categories — if the C ABI returns `OCCTL_NOT_DONE`, the Python exception is `NotDoneError`, not `RuntimeError`.

**Iterator end-of-iteration is not an error.** `OCCTL_NOT_FOUND` returned from `occtl_node_iter_next` is translated to native `StopIteration` (Python), `false` from `MoveNext` (C# `IEnumerator`), or end-of-iterator (JS), and the raw error slot is **not** read. See §4.6.

### 4.3 Opaque handles → RAII

Every `occtl_*_t*` becomes a handle wrapper that owns the pointer and calls the matching `_free` on disposal:

| Language | Mechanism |
| --- | --- |
| Python | `__del__` + explicit `close()`; `contextlib.AbstractContextManager`. Forbid resurrection. Optional `weakref.finalize` for the safety net. |
| C# | `IDisposable` + `SafeHandle` subclass per type. Finalizer as safety net only — users are expected to `using`. |
| Node | `Napi::ObjectWrap<T>` with finaliser registered for GC; manual `.close()` available. |
| WASM | Embind class with `delete()` required (Emscripten convention). Auto-finalisation via `FinalizationRegistry` where it survives the userland API. |
| Rust | `impl Drop`, paired with `unsafe` constructor in `occtl-sys` and safe wrapper in `occtl`. |
| Go | `runtime.SetFinalizer` + explicit `Close()`. |

Handles are **move-only** in languages with the concept; in others, copies are forbidden and clone goes through an explicit `clone()` (which calls a C-side clone when one exists, otherwise errors).

`occtl_*_free` is NULL-tolerant per [ABI_PATTERNS.md §4](./ABI_PATTERNS.md#4-opaque-handles); the binding may rely on this. Calling `free` twice is *not* defined to be safe at the binding layer — the wrapper sets its pointer to NULL after free and ignores subsequent disposes.

**Graph lifecycle naming parity.** Idiomatic facades should expose both the language-native constructor form and explicit factory/adoption aliases so cross-language examples stay mechanically translatable:

- Constructor/factory: `Graph()` / `Graph.create()` (or `Graph::new()` + `Graph::create()` in Rust, `NewGraph()` + `CreateGraph()` in Go).
- Owned-pointer adoption: use explicit danger markers (`from_pointer_unsafe` / `fromPointerUnsafe` / `FromPointerUnsafe`), with Rust remaining `unsafe` at this boundary.
- Explicit disposal alias: where the ecosystem expects it, provide `close`/`Close` in addition to finalizer/drop behavior.

### 4.4 Lightweight value IDs → strong nominal types

`occtl_node_id_t`, `occtl_uid_t`, `occtl_ref_id_t`, `occtl_rep_id_t` are 64-bit POD bundles in C. In bindings they become **distinct** nominal types so the type system catches "passed a face id where a vertex id was expected":

| Language | Mechanism |
| --- | --- |
| Python | `class NodeId(NamedTuple): bits: int` with `__class_getitem__`-typed factories; static type checkers (mypy / pyright) catch mixing |
| C# | `readonly record struct NodeId(ulong Bits)` |
| TS  | `type NodeId = { __brand: 'NodeId'; bits: bigint }` (branded type) |
| Rust | `#[derive(Copy)] struct NodeId(u64);` newtype |

Bindings do **not** unpack the bit layout. Kind queries go through the C function (`occtl_graph_node_kind`), exactly as the C++ veneer does.

### 4.5 Options / info structs → builder + auto `struct_version`

`occtl_*_info_t` structs are constructed by the binding's builder and the binding **always** sets `struct_version` to the value of `OCCTL_*_VERSION_N` the binding was compiled against. Users never set it. `p_next` is exposed as a typed-chain in languages that can express it (TS discriminated union, Rust enum) and as `None`/`null` in others — but for v1 it is always `None`, since no extension chains ship yet.

```python
# Python — natural construction; struct_version filled by the binding.
box = topo.make_box(prim.BoxInfo(dx=10.0, dy=10.0, dz=5.0))
```

```csharp
// C#
var box = topo.MakeBox(new BoxInfo { Dx = 10.0, Dy = 10.0, Dz = 5.0 });
```

```ts
// TS
const box = topo.makeBox({ dx: 10, dy: 10, dz: 5 });
```

The raw layer calls `occtl_*_info_init(&info)` to set version, then overrides fields. This is the [ABI_PATTERNS.md §8](./ABI_PATTERNS.md#8-options--info-structs) contract: **always go through the initializer**, never write `struct_version` from the binding-layer directly.

### 4.6 Iterators → native iteration

The §10.3 opaque iterator becomes the language-native shape:

```python
# Python — yields, frees on exhaustion or generator close
def faces(graph):
    it = _raw.ffi.new('occtl_node_iter_t**')
    _check(_raw.lib.occtl_graph_face_iter_create(graph._ptr, it))
    try:
        out = _raw.ffi.new('occtl_node_id_t*')
        while True:
            s = _raw.lib.occtl_node_iter_next(it[0], out)
            if s == _raw.OCCTL_NOT_FOUND:
                return
            _check(s)
            yield NodeId(out.bits)
    finally:
        _raw.lib.occtl_node_iter_free(it[0])
```

```csharp
// C# — IEnumerable<NodeId>; iterator handle held in IEnumerator and disposed
public IEnumerable<NodeId> Faces() {
    var it = IntPtr.Zero;
    Check(Native.occtl_graph_face_iter_create(_ptr, out it));
    try {
        while (true) {
            var id = default(OcctlNodeId);
            var s = Native.occtl_node_iter_next(it, ref id);
            if (s == OcctlStatus.NotFound) yield break;
            Check(s);
            yield return new NodeId(id.Bits);
        }
    } finally {
        Native.occtl_node_iter_free(it);
    }
}
```

```ts
// TS — Iterable<NodeId>; the binding's Generator owns the handle
function* faces(graph: Graph): Generator<NodeId> {
    const it = native.graphFaceIterCreate(graph._ptr);
    try {
        while (true) {
            const id = native.nodeIterNext(it);
            if (id === null) return;
            yield id;
        }
    } finally {
        native.nodeIterFree(it);
    }
}
```

Rules:

- The iterator handle is owned by the binding's iterator object, not exposed to users.
- Closing the generator early (`generator.close()` / `IDisposable.Dispose`) frees the iterator.
- `OCCTL_NOT_FOUND` is the end-of-iteration sentinel; do **not** read `occtl_error_last` on this path ([ABI_PATTERNS.md §10.3](./ABI_PATTERNS.md#103-opaque-iterator-sparse--lazy--unbounded)).
- Adding nodes while iterating is UB; document this on every iterator factory in the idiomatic layer.

### 4.7 Span / view → zero-copy native array

§10.2 spans (`occtl_triangulation_view_t`, B-spline pole arrays, etc.) map to the binding's native zero-copy array:

| Language | Type | Mechanism |
| --- | --- | --- |
| Python | `numpy.ndarray` | `np.frombuffer(ffi.buffer(ptr, count*sizeof(T)), dtype=T)` + `.reshape(count, 3)` for xyz |
| C# | `Span<T>` (or `ReadOnlySpan<T>`) | `new ReadOnlySpan<double>(ptr.ToPointer(), count)` (in `unsafe` block, hidden) |
| TS / Node | `Float64Array`, `Uint32Array` | `new Float64Array(napi_external_buffer)` over the native pointer |
| TS / WASM | `Float64Array` view over `HEAPF64` | `new Float64Array(Module.HEAPF64.buffer, ptr, count)` — must be re-acquired after any growth of the WASM heap |
| Rust | `&[T]` | `unsafe { std::slice::from_raw_parts(ptr, count) }` |

**Lifetime is the binding's responsibility to enforce.** §10.2 says "valid until the graph is mutated or freed"; the idiomatic layer either:

(a) **Materialises a copy** on access when the span is small or the lifetime is awkward. Default for ergonomic accessors.

(b) **Hands back a view tied to the parent**: the parent handle is held by the view, so the view object's lifetime extends the parent. Default for hot paths (mesh data, big pole arrays). Document `view.copy()` as the escape hatch when the user wants to outlive the parent.

WASM is the special case: any `growMemory` call invalidates every TypedArray view into `HEAPF64`. The WASM facade re-acquires views on every accessor and forbids users from caching them across calls that may allocate. This is enforced by a `view.invalidate()` hook the Module wires up.

### 4.8 Strings — UTF-8 always

The C ABI is UTF-8 everywhere ([ABI_PATTERNS.md §9](./ABI_PATTERNS.md#9-strings)). Bindings:

- **Input.** Convert the language's native string to UTF-8 bytes before crossing. Python `str.encode('utf-8')`; C# `Encoding.UTF8.GetBytes` (or `[MarshalAs(UnmanagedType.LPUTF8Str)]` on .NET 5+); JS `TextEncoder`; Rust `&CStr` constructed from a `String`.
- **Output (NUL-terminated, borrowed).** Decode lazily. Always copy across the boundary into a native string — the C ABI promises only "valid until next call on this thread," and async / multi-threaded callers can't rely on that.
- **Output (two-call buffer).** The binding hides the two-call pattern: sizes once, allocates, refills, returns the decoded native string.

Length-counted strings (`name`, `name_len`) take a bytes-like input. Both NUL-terminated and length-counted forms exist in the C ABI; the binding picks the safer length-counted variant when both exist.

### 4.9 Callbacks — pinning across the GC

Bindings hold a **strong reference** to the wrapper containing `user_data` for the registration's lifetime. Forgetting to do this is the #1 binding bug.

```python
# Python — closure-bound user_data wrapped in a CFFI callback handle
class _CbHolder:
    def __init__(self, py_cb): self.cb = py_cb
self._cb_holder = _CbHolder(my_progress_fn)
self._ffi_cb = ffi.callback('occtl_status_t (double, const char*, void*)',
                             lambda f, s, ud: self._cb_holder.cb(f, ffi.string(s).decode()))
```

```csharp
// C# — GCHandle.Alloc keeps the delegate from being collected
_progressDelegate = (fraction, stage, user_data) => userCb(fraction, Marshal.PtrToStringUTF8(stage));
_progressDelegateHandle = GCHandle.Alloc(_progressDelegate);
```

Rules:

- Callbacks are `noexcept` ([ABI_PATTERNS.md §12](./ABI_PATTERNS.md#12-callbacks)). The binding wraps the user's callback to catch native exceptions and translate them into `OCCTL_CANCELLED` or `OCCTL_ERROR` plus an opaque `extended` code identifying the binding origin.
- `user_data` is always the **last** parameter. Bindings do not surface `user_data` to userland — they bind it via closure.
- The cancellation channel is the cooperative one — `OCCTL_CANCELLED` returned from the callback short-circuits the host operation per [ABI_PATTERNS.md §6.1](./ABI_PATTERNS.md#61-status-codes).

### 4.10 Threading

Same model as the C ABI ([ARCHITECTURE.md §8](./ARCHITECTURE.md#8-threading-model)):

- Concurrent reads of distinct graphs: OK.
- Concurrent reads of the same graph: OK if no writer.
- Concurrent writes: the binding does **not** add a mutex. Userland concurrency is userland's problem; if the binding's stdlib idiom expects a mutex (e.g. .NET `lock`), document the user-visible recommendation, do not bake it into the binding silently.
- Thread-local last-error storage is per OS-thread; bindings must not pool requests across threads or the error slot will read the wrong thread's failure. async/await runtimes that hop threads need to capture the error *immediately* on the calling thread (before the next `await`).

---

## 5. Per-language plans

> **Regenerating the raw layer requires libclang.** `tools/abi_dump.py` walks the enabled `include/occtl/*.h` headers via `clang.cindex` and emits `build/abi.json`, which every facade generator consumes. Install once with `pip install libclang` (the PyPI package bundles the native dylib; no system `libclang-dev` is required). The CMake target `occtl-abi-dump` runs the dumper for you; per-binding `occtl-<lang>-regenerate` / `occtl-<lang>-generate` targets depend on it. The configured build also emits `OCCTLFeatures.json`; generators use it with `abi.json` to pick the feature-set library, enabled components, and header set.

`abi.json` carries the Doxygen summaries, parameter docs, return/status docs, and threadsafety tags extracted from the C headers. Binding generators must preserve that documentation where the target language has a native documentation surface: Python `.pyi`/docstrings, C# XML docs, TypeScript JSDoc, Rust doc comments, and Go exported comments. Hand-written ergonomic layers may add idiomatic examples, but they must not contradict the C header text.

### 5.1 Python — cffi (ABI mode)

**Why cffi ABI mode.**

- **ABI mode = no C compiler at install time.** `pip install occtl` only fetches the platform-appropriate wheel for `libocctl-*.so/.dll` plus the pure-Python facade. No `setup.py build_ext` against the user's headers.
- **No re-translation of C++ types** — the whole reason for the C ABI.
- **No C compiler required at install time** for the idiomatic Python package.
- **Typed C declarations** in `cdef` provide strong signature validation.

**Generated layers.**

`occtl/_raw.py`, `occtl/_generated/*.py`, and `occtl/_typed/*.py` are generated from `build/abi.json`, which is produced from `include/occtl/*.h` by the libclang-backed `tools/abi_dump.py`. `_raw.py` owns the cffi declarations and `ffi.dlopen(...)`; `_generated` is the 1:1 function layer; `_typed` promotes mechanically inferable options and helper functions into documented dataclasses and typed wrappers.

The cdef does **not** preprocess `#define` macros; constants are mirrored in a generated `occtl/_abi.py` from the same ABI catalog.

**Idiomatic layer.**

- `occtl.Graph` — `__enter__` / `__exit__`, `close()`, `faces()`, `edges()`, …
- `occtl.geom` — POD types as `@dataclass(frozen=True, slots=True)`. `Point3(x, y, z)` is a plain Python dataclass with `__array__` returning a NumPy view (zero-copy when the dataclass-of-doubles ends up packed; copy otherwise).
- `occtl.geom.BSpline.poles` — returns a NumPy `ndarray` view over §10.2 span memory, lifetime-tied to the parent curve handle by a `weakref`.
- `occtl.viz` — optional `full-with-viz` facade for offscreen/native visualization; the AI/notebook path is `Driver → Viewer → View → Presentable`, then `read_pixels_rgba()` or `dump_image()`.
- `occtl.Error`, `occtl.NotDoneError`, `occtl.GeometryInvalidError`, etc. — `class NotDoneError(Error)` per status code, parented at `Error`.

**Type stubs.**

`occtl/*.pyi` are generated from the C headers (with hand-written overlays for the ergonomic surface). Type-checkers (mypy / pyright) see the binding as fully typed.

**NumPy interop.**

The idiomatic layer's policy is **zero-copy for the spans, copy for small POD reads**. `triangulation.nodes` is a `(N, 3) float64 ndarray` view; `vertex.point()` returns a fresh `Point3` (copy, three doubles).

**Async.**

asyncio support is opt-in via `await occtl.aio.read_step(path)` — the binding offloads to a thread pool and awaits the result. No native async at the C ABI; doing real async would mean coroutine-driven progress callbacks, which is a v2 conversation.

**Packaging.**

- Wheels per platform: `manylinux_2_28_x86_64`, `manylinux_2_28_aarch64`, `macosx_11_0_arm64`, `macosx_11_0_x86_64`, `win_amd64`, `win_arm64`. Source dist excluded (no C compile in install path).
- `auditwheel` bundles `libocctl-*.so` into the wheel; OCCT toolkits are also bundled (vendored), since users are unlikely to have an OCCT install on their PATH.
- License: AGPL is preserved in the wheel; downstream apps must comply. A note in the README spells this out.

### 5.2 C# — P/Invoke + source-generator raw layer

**Why P/Invoke + source generator.**

- P/Invoke is the cross-platform path for this binding.
- Modern .NET source generators emit the raw `[LibraryImport]` declarations from the C headers at compile time. This catches signature drift at build, not at first call.
- Targets **.NET 8+** to use `[LibraryImport]` and `Span<T>` / `Memory<T>` throughout the surface.

**Raw layer.**

`OcctL.Native` — generated by a Roslyn source generator that reads `include/occtl/*.h` and emits one `partial class` per header. The generator runs as part of the build, not committed; the developer ergonomics match the binding shipping as a regular NuGet package.

**Idiomatic layer.**

- `OcctL.Graph : SafeHandle, IDisposable` — `using` lifetime.
- `OcctL.NodeId : IEquatable<NodeId>` — `readonly record struct NodeId(ulong Bits)`.
- `OcctL.Geom.Point3 : IEquatable<Point3>` — `readonly record struct Point3(double X, double Y, double Z)`, laid out compatibly with `occtl_point3_t` so `MemoryMarshal.Cast` lets us reinterpret a `Span<double>` as `Span<Point3>` without copy.
- `OcctL.OcctLException : Exception` — carries `Status`, `Uid`, `ExtendedCode`. Subclasses per status code.

**`Span<T>` interop.**

Span views over §10.2 spans use `MemoryMarshal.CreateReadOnlySpan` over the native pointer. The wrapping struct (`MeshView`) holds a `GCHandle` on the parent graph wrapper to extend the parent's lifetime past the view's last use; `Dispose()` releases.

**NuGet packaging.**

- `OcctL.Core` package — pure managed assembly with the idiomatic layer + the generated raw layer's source.
- `OcctL.Native.{rid}` packages — runtime-specific native libraries (`runtimes/{rid}/native/libocctl-*.{so,dll,dylib}`). The `.NET` build system picks the right one per RID at publish time.
- Meta-package `OcctL` references `OcctL.Core` + all `OcctL.Native.{rid}`.
- AOT-friendly: `[LibraryImport]` is AOT-compatible; the binding compiles with `IsAotCompatible=true`.

### 5.3 JS/TS native — Node N-API

**Why N-API (not nan, not direct V8).**

- N-API is ABI-stable across Node versions. Bind once, run on Node 18 / 20 / 22 / 24 without rebuilding.
- `node-addon-api` is the C++ helper; the addon links the OCCT-Light C ABI directly.

**Raw layer.**

`bindings/node/src/native/raw.cc` — C++ glue exposing C functions as `Napi::Function`. Auto-generated by a TS script that reads `include/occtl/*.h` and emits the addon source.

**Idiomatic layer (TypeScript).**

- `class Graph implements Disposable` — `[Symbol.dispose]()` per the explicit-resource-management proposal; `.close()` fallback.
- `type NodeId = { __brand: 'NodeId'; bits: bigint }` — branded type; constructed by the binding only.
- `Point3 = { x: number; y: number; z: number }`.
- `viz` wrappers expose generated raw calls for driver/viewer/view/presentable handles; higher-level frontends can layer native-window ownership on top.
- `class OcctLError extends Error` — carries `status`, `uid`, `extended`.
- Runtime feature probing exports `AVAILABLE_MODULES` (derived from `OCCTLFeatures.json` when present, otherwise inferred from the native export table). Hand-written facades throw `Unsupported` deterministically when a disabled symbol is requested.

**`TypedArray` interop.**

§10.2 spans use `napi_create_external_buffer` to wrap the native pointer in a `Buffer`, then create a `Float64Array` view over it. The buffer's `finalize` callback releases the GC-handle on the parent graph wrapper, keeping the parent alive while the view is reachable.

**Packaging.**

- `npm install occtl` fetches prebuilt binaries via `prebuildify` for the four common Node ABI × platform combinations (linux-x64, linux-arm64, macos-arm64, macos-x64, win-x64). Falls back to source build with `node-gyp` if a prebuild is missing.
- TS declarations are shipped (`occtl.d.ts`) — generated from the C headers + hand-written overlays.

### 5.4 JS/TS web — Emscripten + Embind

**Why Emscripten + Embind, not WebAssembly Component Model (yet).**

- Component Model is the right long-term answer; today Emscripten + Embind is mature, debuggable, and ships across every browser.
- Embind exposes JS classes that mirror C++ classes; we use the C ABI directly (no C++) and Embind purely for the JS-class wiring.

**Build.**

The wasm build links the selected single OCCT-Light feature-set target statically (Emscripten compiles OCCT itself + OCCT-Light into one `.wasm`). Output: `occtl.wasm` + `occtl.js` (the JS glue that loads the wasm and exposes the Embind types).

The wasm bundle is large (OCCT is ~30 MB compressed). Code-splitting per module (loading `prim` and `mesh` lazily after `core`/`geom`/`topo`) is a v2 conversation; v1 ships the unified bundle.

**Raw layer.**

`bindings/wasm/src/native/raw.cc` — Embind glue exposing each public C function as a `function`. Auto-generated.

**Idiomatic layer.**

Identical surface to the Node TS facade. The two facades export the same types from `@occtl/types` (a third package shared between them). User code can move between Node and web without touching the API surface.

**Memory model — the WASM gotcha.**

- All zero-copy views over §10.2 spans go through `Module.HEAPF64` etc. and **may be invalidated** if any subsequent call causes the heap to grow.
- Idiomatic-layer policy: views are short-lived. Long-lived data → `.copy()` to a JS array on the JS heap. The TS API surface marks "view" return types vs "owned array" return types in the type signature (`Float64ArrayView` vs `Float64Array`).

**Packaging.**

- `npm install @occtl/wasm` ships `occtl.wasm` + `occtl.js` + types.
- The wasm is loaded lazily on first `await Occtl.load()`.
- Browser-only: pure ESM. No Node.js support in this package; Node uses `@occtl/node`.
- Smoke/parity runners are fail-fast: missing `dist/occtl.wasm` is treated as a setup error, not an optional scenario.

### 5.5 Additional languages

The model scales by the §8 checklist. Three explicit pre-staked plans:

- **Rust.** `occtl-sys` crate — `bindgen` over `include/occtl/occtl.h`, raw `unsafe extern "C"` declarations. `occtl` crate — safe wrapper, `Result<T, occtl::Error>` everywhere, `impl Drop` on handle types, iterator types implementing `Iterator`. Optional `serde` derives for the POD types behind a feature flag.

- **Go.** `cgo` raw layer + `occtl` package with idiomatic Go: `error` return values, close-on-owner handles, and range-friendly helpers. The current implementation reads installed feature metadata (`OCCTLFeatures.json`) to derive `CGO_*` flags and Go build tags, so reduced feature sets compile without unresolved symbols.

- **Java.** Generated JNA raw layer from `build/abi.json` + thin idiomatic facade (`Graph`, primitive builders, booleans, history helpers) + JUnit smoke/parity/coverage checks. Promotion requires CI matrix hardening and packaging into publishable Maven artifacts.

- **Swift.** Direct C-header import (Swift speaks C ABI natively). Thin `OcctL` Swift Package with `class Graph`, `struct NodeId`, error translation, `Sequence` conformance for iterators. Same SafeHandle pattern as C# (`deinit { occtl_graph_free(ptr) }`).

Additional language facades can be added in v2+ using the same checklist.

---

## 6. Packaging and distribution

| Language | Format | Vendoring policy | Versioning |
| --- | --- | --- | --- |
| Python | wheel per platform/arch | Wheel bundles `libocctl-*.so/.dylib/.dll` and OCCT toolkits | binding SemVer; pins min wrapper version `>=X.Y` |
| C# | NuGet (managed + RID-native packages) | Native libraries shipped per RID under `runtimes/{rid}/native/` | binding SemVer; metapackage pins exact RID-native versions |
| Node | npm (+ prebuildify binaries) | Native `.node` files per ABI/platform | binding SemVer; engines.node sets the supported ABI |
| WASM | npm (with `.wasm` + JS glue) | Self-contained (the wasm has OCCT inlined) | binding SemVer; ships exactly one wasm version per package version |
| Rust | crates.io (`occtl-sys` static-links `libocctl-*.a`) | `build.rs` reads `OCCTL_DIR`, links static | binding SemVer; pins compatible wrapper via `links =` |
| Go | go modules (links selected OCCT-Light shared lib) | env from `bindings/go/tools/occtl_features_env.py` or one-shot `go_with_features.py` | binding SemVer; module pins min wrapper |
| Java | Maven artifact(s) | JNA loads selected feature-set shared library from `OCCTL_LIBRARY_PATH` / `OCCTL_LIBRARY_NAME` | binding SemVer; artifact declares minimum ABI version |
| Swift | Swift Package + binary target | Binary target ships `.xcframework` with the lib | binding SemVer |

Static vs dynamic linking of the native library is a per-language ergonomics call, not an ABI question — the C ABI is the same either way.

### Script-first package preparation

Package preparation is script-first. User-facing entrypoints live under
`tools/scripts/`, while implementation lives under `tools/packaging/`. Any later
CI/Actions integration should wrap these scripts, not replace them:

- macOS/Linux: `tools/scripts/build_binding_packages.sh`
- Windows PowerShell: `tools/scripts/build_binding_packages.ps1`
- Direct Python entrypoint: `tools/scripts/build_binding_packages.py`

Supported targets:

- `python-wheel` and `python-conda`
- `csharp-nuget`
- `node-npm` and `wasm-npm`
- `java-maven`
- `rust-crate`
- `go-module`

The packaging script emits `binding-packages-manifest.json` under the selected
output directory with artifact paths and SHA-256 checksums for release handoff.

Build-system wrapper targets:
- `occtl-bindings-packages-dry-run`
- `occtl-bindings-packages`

Workspace unification helpers:
- Shared layout map: `bindings/README.md`
- Cross-language workspace tool: `tools/scripts/bindings.py`
  - `overview` (logical zone map)
  - `audit` (required path checks)
  - `clean --dry-run` / `clean` (transient artefact cleanup)

### License

All bindings inherit AGPL-3.0-or-later from OCCT-Light. The Python `occtl` wheel, NuGet `OcctL` package, npm `occtl` and `@occtl/wasm` packages all declare AGPL. Downstream binaries must comply; this is documented in every binding's README first paragraph. OCCT itself is LGPL-2.1; the wrapper does not change OCCT's license; bundling OCCT in a wheel is a redistribution and is honoured as LGPL distribution.

---

## 7. Test strategy

Every binding ships three tiers.

**Tier 1 — smoke.** A single test per binding that:

1. Loads the library, verifies `occtl_runtime_abi_version()` matches.
2. Creates a graph, adds a box via `occtl_prim_make_box` (when `prim` lands; until then via topo builders), counts faces, frees the graph.
3. Verifies the error path: forces an `OCCTL_INVALID_ARGUMENT`, reads the translated native exception, confirms `.message` is non-empty.

Tier 1 is the required contract for every binding. Current CI enforces Tier-1 smoke for Python, C#, Node, and Java; WASM, Rust, and Go bindings currently run focused coverage/parity checks in CI while their full Tier-1 runtime matrix is being finalized.
Smoke/parity runners must be fail-fast: missing native artifacts or unresolved loader paths are setup errors, not bypassed tests.

**Tier 2 — parity.** A binding-agnostic test grid lives at `tests/binding_parity/`. Each row is a small scenario expressed in JSON (or as a generated C++ executable that prints expected output). Every binding's parity test runs the same scenario through its facade and compares output. This catches "Python returns 3.0 where C# returns 3" types of drift.

**Tier 3 — symbol coverage.** For each binding, a build-time check consumes `build/abi.json`, lists every `OCCTL_API` function, and confirms the generated raw/typed layers expose it. The ergonomic layer has focused presence tests for the commands users naturally call directly (`Graph` builders, booleans, I/O helpers, history, selectors, spans, viz screenshot helpers). Bindings may start with raw coverage and parity-smoke checks, but promotion to full Tier-1 requires generated coverage over the current ABI plus ergonomic presence tests for every promoted command family.

The C++ veneer's symbol-coverage check ([ABI_PATTERNS.md §16](./ABI_PATTERNS.md#16-the-c-veneer-occtl-hpp)) is the model — bindings adopt the same pattern.

---

## 8. Adding a new language — the checklist

When a contributor wants to add language `<L>`:

1. **Read.** ARCHITECTURE.md, ABI_PATTERNS.md, this doc.
2. **Carve directory.** `bindings/<L>/`. Sub-structure mirrors §3 layout.
3. **Raw layer.** Generate from `build/abi.json` or the public C headers. Whatever tool the language community settles on (cffi, bindgen, source generators, N-API glue, cgo, Swift ClangImporter). The raw layer is private; document it as such.
4. **Typed facade.** Generate complete status-checking wrappers, typed option structs/builders, and doc comments from `abi.json`. This layer is also private unless a language has no better packaging boundary.
5. **Ergonomic layer.** Hand-write the small language-native surface that users should reach for: RAII/disposal, errors, graph methods, iterators, spans/views, options builders, and idiomatic names.
6. **Implement the ten §4 responsibilities**, in order. ABI handshake → error translation → handles → IDs → options → iterators → spans → strings → callbacks → threading.
7. **Tier-1 smoke test.** Same shape as every other binding; same expected output.
8. **Symbol-coverage check** in the binding's build pipeline.
9. **Tier-2 parity tests** — wire the binding into `tests/binding_parity/`.
10. **Packaging.** Per §6: how does this language's package manager pull binaries; where do native libs live; what does `install` look like for the user.
11. **README + AGENTS.md update.** Add the binding to the "Bindings" tables in `ABI_PATTERNS.md §17`, this doc's §5, and the project root `README.md`.

A new binding lands in v0.x while the API surface is still evolving; promotion to v1 stable requires Tier 1–3 fully green over two release cycles.

---

## 9. Anti-patterns — do not do these in any binding

- Keep bindings on the C ABI only.
- **Do not re-export C++ types** (`occtl-hpp` is for C++ users; bindings don't see it).
- **Do not invent additional error types** beyond the C status codes. Bindings translate, they do not classify.
- **Do not silently retry** on `OCCTL_NOT_DONE`, `OCCTL_GEOMETRY_INVALID`, or any error code. Retry policy is userland's decision.
- **Do not hold raw pointers across `await` / Promise boundaries** without an explicit lifetime extension on the parent handle. Doubly do not do this on WASM, where the pointer may dangle after a `growMemory`.
- **Do not surface `user_data` / `void*`** to userland. Callbacks bind context via closure; the C-side `user_data` is the binding's bookkeeping.
- **Do not omit the ABI version check** at load time. Mismatch crashes are the worst kind of crash to debug.
- **Do not add a Python / .NET / JS-only function** that has no C-ABI analogue. New behavior goes through the C ABI first.
- **Do not expose the raw layer.** Users `import occtl`, never `occtl._raw`.
- **Do not bypass the symbol-coverage check.** The generated raw/typed layers must match `abi.json`, and promoted ergonomic command families need explicit presence tests.


