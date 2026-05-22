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
using System.Runtime.InteropServices;
using OcctL.Native;

namespace OcctL;

/// <summary>
/// Idiomatic facade for the core OCCT-Light surface — version queries, error access,
/// runtime lifecycle. The ABI handshake is performed lazily on the first call.
/// </summary>
public static class Core
{
    /// <summary>The ABI version this binding talks. Matches <c>OCCTL_ABI_VERSION</c>.</summary>
    public const uint AbiVersion = AbiHandshake.ExpectedAbiVersion;

    /// <summary>The runtime's reported ABI version.</summary>
    public static uint RuntimeAbiVersion
    {
        get { AbiHandshake.EnsureInitialized(); return AbiHandshake.RuntimeAbiVersion; }
    }

    /// <summary>The library SemVer reported by the loaded native library.</summary>
    public static Version RuntimeVersion
    {
        get
        {
            AbiHandshake.EnsureInitialized();
            NativeMethods.OcctlRuntimeVersion(out uint major, out uint minor, out uint patch);
            return new Version((int)major, (int)minor, (int)patch);
        }
    }

    /// <summary>The OCCT version OCCT-Light was built against (diagnostic only).</summary>
    public static string OcctVersion
    {
        get
        {
            AbiHandshake.EnsureInitialized();
            IntPtr p = NativeMethods.OcctlRuntimeOcctVersion();
            return Marshal.PtrToStringUTF8(p) ?? string.Empty;
        }
    }

    /// <summary>Last error message recorded on this thread, or an empty string.</summary>
    public static string LastErrorMessage
    {
        get
        {
            IntPtr p = NativeMethods.OcctlErrorLast();
            if (p == IntPtr.Zero) return string.Empty;
            OcctlError e = Marshal.PtrToStructure<OcctlError>(p);
            if (e.Message == IntPtr.Zero) return string.Empty;
            return Marshal.PtrToStringUTF8(e.Message) ?? string.Empty;
        }
    }

    /// <summary>Clears the thread-local error slot.</summary>
    public static void ClearError() => NativeMethods.OcctlErrorClear();

    /// <summary>Returns the human-readable name for a status code.</summary>
    public static string StatusToString(OcctlStatus status)
    {
        IntPtr p = NativeMethods.OcctlStatusToString(status);
        return Marshal.PtrToStringUTF8(p) ?? "OCCTL_UNKNOWN";
    }
}
