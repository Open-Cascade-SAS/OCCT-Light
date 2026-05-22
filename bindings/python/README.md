# occtl — Python binding for OCCT-Light

**License: AGPL-3.0-or-later.** This package is licensed under the GNU Affero
General Public License v3.0 or later. Any software that links against it
must comply with the AGPL. If AGPL is not compatible with your project,
you cannot use this package.

`occtl` is a Pythonic veneer over OCCT-Light's public C ABI. Every public C
function in the ABI has a corresponding Python wrapper. Handles are RAII via
context managers and `__del__`; status codes are translated into typed
exceptions; iterators are Python generators; span-returning functions return
zero-copy NumPy arrays.

The binding uses [cffi](https://cffi.readthedocs.io/) in **ABI mode** — no
C compiler is needed at install time. The shared library
(`libocctl-<feature-set>.{so,dylib,dll}`) must be discoverable. The wheel bundles a
matching native library; for a source checkout, point `OCCTL_LIBRARY_PATH` at
the directory containing the built shared libs.

## Install

```sh
pip install occtl                       # from PyPI (when published)
pip install -e bindings/python/         # development install from a checkout
```

## Quickstart

```python
import occtl

# ABI version handshake runs automatically at import time.
print(occtl.runtime_occt_version())

with occtl.Graph() as g:
    v = g.make_vertex(x=1.0, y=2.0, z=3.0)
    print(v)                                   # NodeId(bits=...)
    for vid in g.vertex_iter():
        x, y, z = g.vertex_point(vid)
        print(x, y, z)
```

## Layout

- `occtl.core` — runtime lifecycle, errors, versions.
- `occtl.geom` — POD geometry types and helpers.
- `occtl.topo` — `Graph`, builders, iterators.
- `occtl.prim` — primitive solids (box, sphere, cone, …).
- `occtl.text` — text-to-faces.
- `occtl.curves`, `occtl.curves2d`, `occtl.surfaces` — parametric geometry.

`src/occtl` is the package root. `occtl._raw`, `occtl._abi`, `occtl._handles`, `occtl._ids`, `occtl._errors`,
`occtl._iters`, `occtl._spans`, `occtl._strings`, `occtl._options` are
implementation details. Do not import them from user code.

## Regenerating the raw layer

The raw cffi cdef and the auto-wrappers are produced from
`build/abi.json` by `tools/generate_facade.py`.

```sh
# from repo root
python3 tools/abi_dump.py --output build/abi.json
python3 bindings/python/tools/generate_facade.py
```

The generated files are committed so the package is installable without
libclang. Re-run when the C ABI changes.

## Testing

```sh
bash bindings/python/tools/setup_venv.sh
cd bindings/python
.venv/bin/pytest tests/ -v
```

## Versioning

The binding tracks the C ABI's `OCCTL_ABI_VERSION`. At import time, the runtime
ABI version is compared against the value the binding was generated with; on
mismatch, an `AbiMismatchError` is raised before any other call is allowed.
