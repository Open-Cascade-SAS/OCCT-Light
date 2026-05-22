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

namespace OcctL.SafeHandles;

/// <summary>RAII handle for <c>occtl_batch_t*</c>. Frees via <c>occtl_batch_free</c>.</summary>
public sealed class BatchHandle : SafeHandle
{
    /// <summary>Creates an empty batch handle.</summary>
    public BatchHandle() : base(IntPtr.Zero, ownsHandle: true) { }
    /// <summary>Adopts an existing native batch pointer.</summary>
    public BatchHandle(IntPtr handle) : base(IntPtr.Zero, ownsHandle: true) { SetHandle(handle); }
    /// <summary>True when the native handle is null.</summary>
    public override bool IsInvalid => handle == IntPtr.Zero;
    /// <summary>Aborts and releases the native batch.</summary>
    protected override bool ReleaseHandle()
    {
        // A batch is released by occtl_batch_commit (apply) or occtl_batch_abort (discard).
        // When the SafeHandle is finalized without an explicit decision, abort is the safe
        // default — drop staged mutations rather than apply half-built state.
        if (handle != IntPtr.Zero) _ = NativeMethods.OcctlBatchAbort(handle);
        return true;
    }
}
