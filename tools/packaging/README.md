# Binding Packaging Scripts

Script-first packaging entrypoint for OCCT-Light bindings.

This toolchain is the canonical script-first packaging path for bindings.
CI/Actions are optional wrappers around these scripts, not a separate flow.
The scripts prepare artifacts only; they do not publish them.

Each target validates required tooling (`conda`, `dotnet`, `npm`, `mvn`,
`cargo`, `go`) before execution and writes an artifact manifest with SHA-256
checksums to `build/binding-packages/binding-packages-manifest.json`.

## Entrypoints

User-facing wrappers at repo root:
- macOS / Linux: `tools/scripts/build_binding_packages.sh`
- Windows (PowerShell): `tools/scripts/build_binding_packages.ps1`
- Direct Python: `tools/scripts/build_binding_packages.py`

Implementation entrypoint (this folder):
- Direct Python: `tools/packaging/build_binding_packages.py`

Wrapper behavior:
- `build_binding_packages.sh` uses `python3` when available, then falls back to `python`.
- `build_binding_packages.ps1` uses `python` when available, then falls back to `py -3`.

## Targets

- `python-wheel` : builds wheel + sdist for `bindings/python`
- `python-conda` : builds a conda package from `bindings/python/conda/recipe`
- `csharp-nuget` : builds a NuGet package from `bindings/csharp/src/api/OcctL.csproj`
- `node-npm` : builds npm tarball for `bindings/node`
- `wasm-npm` : builds npm tarball for `bindings/wasm`
- `java-maven` : builds Maven package for `bindings/java`
- `rust-crate` : creates `.crate` archives for `occtl-sys` and `occtl`
- `go-module` : creates a zip snapshot of `bindings/go` module sources

## Examples

Build everything:

```bash
tools/scripts/build_binding_packages.sh
```

Build selected targets:

```bash
tools/scripts/build_binding_packages.sh --targets python-wheel,python-conda,node-npm
```

Dry-run command plan:

```bash
tools/scripts/build_binding_packages.sh --dry-run
```

Custom output directory:

```bash
tools/scripts/build_binding_packages.sh --output-dir build/community-packages
```

On Windows PowerShell:

```powershell
.\tools\scripts\build_binding_packages.ps1 --targets python-wheel,csharp-nuget
```

## Output Layout

Default output root:

```text
build/binding-packages/
```

Subfolders are created per binding ecosystem.
