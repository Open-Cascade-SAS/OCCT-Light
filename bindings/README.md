# Copyright (c) 2026 Capgemini Engineering Research and Development.
#
# This file is part of OCCT-Light software library.
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License version 3 as published
# by the Free Software Foundation, with an option to use any later version.
# Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
# for complete text of the license and disclaimer of any warranty.
#
# Alternatively, this file may be used under the terms of a commercial
# license or contractual agreement.
#
# SPDX-License-Identifier: AGPL-3.0-or-later

# Bindings Workspace Layout

This directory hosts all language facades over the same OCCT-Light C ABI.

## Logical Contract

Every binding is organized into the same logical zones, even when the
language-specific folders differ:

- `api` : idiomatic language-facing surface
- `ffi` : generated or raw native interop layer
- `tests` : smoke / parity / coverage checks
- `tools` : generation and maintenance scripts
- `generated` : generated files only (never hand-edited)

## Current Language Map

| Language | `api` | `ffi` | `tests` | `tools` | `generated` |
| --- | --- | --- | --- | --- | --- |
| Python | `python/src/occtl/` | `python/_abi.py`, `python/src/occtl/_generated/_raw.py` | `python/tests/` | `python/tools/` | `python/src/occtl/_generated/` |
| C# | `csharp/src/api/` | `csharp/src/ffi/` | `csharp/tests/unit/`, `csharp/tests/parity/` | `csharp/tools/` | `csharp/src/api/_Generated/`, `csharp/src/ffi/Generated/` |
| Node | `node/src/ts/` | `node/src/native/` | `node/tests/` | `node/tools/` | `node/src/ts/generated/`, `node/src/native/raw.cc` |
| WASM | `wasm/src/ts/` | `wasm/src/native/` | `wasm/tests/` | `wasm/tools/` | `wasm/src/ts/generated/`, `wasm/src/native/raw.cc` |
| Rust | `rust/src/occtl/` | `rust/src/occtl-sys/` | `rust/src/occtl/tests/`, `rust/tests/parity_runner/` | `rust/` | `rust/src/occtl-sys/generated/` |
| Go | `go/src/occtl/` | `go/src/occtl/raw.go` | `go/src/occtl/*_test.go`, `go/tests/parity_runner/` | `go/tools/` | `go/generated/` |
| Java | `java/src/main/` | `java/src/main/java/org/occtl/generated/` | `java/src/test/` | `java/tools/` | `java/src/main/java/org/occtl/generated/` |

## Unified Workspace Tooling

Use one command family for all languages:

```bash
python3 tools/scripts/bindings.py overview
python3 tools/scripts/bindings.py audit
python3 tools/scripts/bindings.py clean --dry-run
```

The tool checks contract paths and can remove transient local build artefacts
from binding folders when explicitly requested.
