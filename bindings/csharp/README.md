# OcctL — .NET binding for OCCT-Light

**License: AGPL-3.0-or-later.** This binding is a thin managed layer over OCCT-Light. AGPL obligations attach to *any* program that links it — including hosted web services. Make sure your project's licensing model is compatible before depending on this package.

OcctL surfaces the OCCT-Light C ABI to .NET 8+ via source-generated `[LibraryImport]` marshalling.

## Install

```bash
dotnet add package OcctL
```

The package ships the managed `OcctL.dll` and platform-specific `libocctl-<feature-set>.{so,dylib,dll}` shared libraries under `runtimes/<rid>/native/`. Cross-platform consumers don't need to install OCCT separately — the C++ code is already linked into the selected OCCT-Light feature-set library.

## Use

```csharp
using OcctL;

// 1. ABI handshake runs lazily on first use; throws AbiMismatchException on skew.
Console.WriteLine($"OcctL {Core.RuntimeVersion} (ABI {Core.RuntimeAbiVersion})");

// 2. Build a topology graph.
using var graph = new Graph();
NodeId boxSolid = graph.MakeBox(dx: 10.0, dy: 10.0, dz: 5.0);
Console.WriteLine($"box={boxSolid} faces={graph.FaceCount} edges={graph.EdgeCount} vertices={graph.VertexCount}");

// 3. Iterate; the enumerator owns the underlying occtl_node_iter_t handle.
foreach (NodeId face in graph.Faces())
    Console.WriteLine($"  face {face}");
```

## What's in the box

| Layer                     | Where                                            | What                                                                 |
| ------------------------- | ------------------------------------------------ | -------------------------------------------------------------------- |
| Raw P/Invoke              | `src/ffi` assembly, `Generated/*.g.cs`            | One `[LibraryImport]` per `OCCTL_API` function.                      |
| Blittable types           | `OcctL.Native.Types.g.cs`                        | One `[StructLayout(Sequential)]` per public struct + value handle.   |
| Enums + constants         | `OcctL.Native.Enums.g.cs`, `Constants.g.cs`      | Mirrors of `occtl_*_t` enums and `OCCTL_*` `#define`s.               |
| Idiomatic raw wrappers    | `OcctL._Generated/<Module>.<Header>.Idiomatic.g.cs` | Per-function method that translates `OcctlStatus` → exception.   |
| Typed generated facade    | `OcctL._Generated/*.Typed.g.cs`                  | Option records and mechanically inferable helpers such as primitive builders. |
| Ergonomic facade          | `src/api/{Core,Geom,Topo,Prim,Text}.cs`           | Hand-written shaped API: `Graph` and extension methods. |
| Exceptions                | `OcctL.OcctLException` + 16 subclasses           | One subclass per `occtl_status_t` value.                             |
| RAII                      | `OcctL.SafeHandles.*`                            | `SafeHandle` per opaque type — auto-frees on disposal.               |
| Strong nominal IDs        | `OcctL.{Uid,NodeId,RefId,RepId}`                 | `readonly record struct` over `ulong`.                               |
| Zero-copy spans           | `OcctL.Spans.BorrowedSpan<T>`                    | Lifetime tied to parent `SafeHandle`.                                |
| Single-pass iterators     | `OcctL.Iterators.NodeIterEnumerable`             | `IEnumerable<NodeId>` over `occtl_node_iter_t`.                      |

## Threading

The OCCT-Light contract (BINDINGS.md §4.10) applies as-is:

- **Single-writer graph** — `Graph` mutators (`MakeVertex`, `MakeBox`, …) are not thread-safe for the same graph instance. Mutate from one thread at a time.
- **Read-mostly graph** — Multiple threads may read a graph concurrently with no synchronisation.
- **Thread-local error slot** — `occtl_error_last()` is per-thread. Calling `Core.LastErrorMessage` from a different thread than the one that triggered the failure returns an empty string.

The binding does not hold a process-wide mutex. If host code multiplexes graphs across threads, gate writes with a lock you own.

## Building from source

```bash
# 1. Refresh the ABI snapshot.
python3 ../../tools/abi_dump.py --output ../../build/abi.json

# 2. Regenerate the C# facade. (Idempotent.)
python3 tools/generate_facade.py --abi ../../build/abi.json

# 3. Build the solution.
dotnet build OcctL.sln

# 4. Run tests (tier-1 smoke + tier-3 symbol coverage).
dotnet test tests/unit/OcctL.Tests.csproj

# 5. Pack a NuGet.
dotnet pack src/api/OcctL.csproj -c Release -o dist/
```

Generated `.g.cs` files are build outputs. Don't edit them by hand — re-run `generate_facade.py` instead.
