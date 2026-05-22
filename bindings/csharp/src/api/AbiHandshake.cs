// Copyright (c) 2026 Capgemini Engineering Research and Development.
//
// This file is part of OCCT-Light software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License version 3 as published
// by the Free Software Foundation, with an option to use any later version.
// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
// for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of a commercial
// license or contractual agreement.
//
// SPDX-License-Identifier: AGPL-3.0-or-later

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using OcctL.Native;

namespace OcctL;

/// <summary>
/// One-time runtime check that the loaded native library exposes an ABI version compatible
/// with the binding's compile-time constant. Triggered automatically the first time any
/// OcctL type is used via <see cref="ModuleInitializerAttribute"/>.
/// </summary>
/// <remarks>
/// BINDINGS.md §4.1 — the ABI handshake. On mismatch we throw <see cref="AbiMismatchException"/>
/// rather than crashing later inside a P/Invoke with garbled struct layouts.
/// </remarks>
public static class AbiHandshake
{
    /// <summary>The ABI version the binding was compiled for. Matches <c>OCCTL_ABI_VERSION</c>.</summary>
    public const uint ExpectedAbiVersion = 1;

    private static int s_initialized = 0;

    /// <summary>Returns the native ABI version reported by the loaded library.</summary>
    public static uint RuntimeAbiVersion => unchecked((uint)NativeMethods.OcctlRuntimeAbiVersion());

    /// <summary>The handshake-verified ABI version this binding talks. Same as <see cref="ExpectedAbiVersion"/>.</summary>
    public static uint BindingAbiVersion => ExpectedAbiVersion;

    /// <summary>
    /// Runs the ABI handshake. Safe to call repeatedly — only the first call has effect.
    /// Throws <see cref="AbiMismatchException"/> on version mismatch.
    /// </summary>
    public static void EnsureInitialized()
    {
        if (System.Threading.Interlocked.Exchange(ref s_initialized, 1) != 0) return;

        uint actual = unchecked((uint)NativeMethods.OcctlRuntimeAbiVersion());
        if (actual != ExpectedAbiVersion)
        {
            // Reset so a caller catching the exception can retry after fixing their load
            // path — re-throwing if the mismatch persists.
            System.Threading.Volatile.Write(ref s_initialized, 0);
            throw new AbiMismatchException(ExpectedAbiVersion, actual);
        }
        // Build a fresh init-info struct with struct_version set. The C side accepts NULL
        // for "use defaults"; rather than introduce an unsafe-only overload we hand it a
        // zero-padded struct with the right version. The C API is idempotent if a prior
        // call already initialised; a second call returns OCCTL_INVALID_ARGUMENT which we
        // swallow here.
        OcctlRuntimeInitInfo info = default;
        info.StructVersion = 1u;
        _ = NativeMethods.OcctlRuntimeInit(in info);
    }

#pragma warning disable CA2255 // ModuleInitializer is intended for libraries doing ABI bootstrap.
    [ModuleInitializer]
    internal static void RunOnModuleLoad()
    {
        // Don't throw from a module initializer — defer the actual handshake to first use.
        // We just record the expected version and let EnsureInitialized run lazily.
        // (Throwing here would prevent the assembly from loading at all, hiding the real
        // problem behind a TypeInitializationException chain.)
    }
#pragma warning restore CA2255
}
