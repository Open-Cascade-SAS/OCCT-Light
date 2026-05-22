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

namespace OcctL.Spans;

/// <summary>
/// A read-only span over native memory whose lifetime is tied to a parent <see cref="SafeHandle"/>.
/// </summary>
/// <remarks>
/// BINDINGS.md §4.7 — zero-copy span/view. Wraps <c>ReadOnlySpan&lt;T&gt;</c> built via
/// <see cref="MemoryMarshal.CreateReadOnlySpan{T}"/>, plus a reference to the SafeHandle the
/// span borrows from. The handle's <c>DangerousAddRef</c> is held while the span is alive.
/// Dispose to release the ref-count.
/// <para>
/// Use the static factory <c>Create</c> rather than constructing directly —
/// it pins the handle before exposing the pointer.
/// </para>
/// </remarks>
public unsafe readonly ref struct BorrowedSpan<T> where T : unmanaged
{
    private readonly SafeHandle _parent;
    private readonly bool _refTaken;
    /// <summary>Borrowed read-only view over native memory.</summary>
    public ReadOnlySpan<T> Span { get; }

    internal BorrowedSpan(SafeHandle parent, ReadOnlySpan<T> span, bool refTaken)
    {
        _parent = parent;
        Span = span;
        _refTaken = refTaken;
    }

    /// <summary>Constructs a <see cref="BorrowedSpan{T}"/> from a native pointer and element count,
    /// holding a ref-count on <paramref name="parent"/> for the lifetime of the span.</summary>
    public static BorrowedSpan<T> Create(SafeHandle parent, IntPtr ptr, int count)
    {
        if (parent is null) throw new ArgumentNullException(nameof(parent));
        if (count < 0) throw new ArgumentOutOfRangeException(nameof(count));
        if (ptr == IntPtr.Zero || count == 0)
        {
            return new BorrowedSpan<T>(parent, ReadOnlySpan<T>.Empty, refTaken: false);
        }
        bool refTaken = false;
        parent.DangerousAddRef(ref refTaken);
        var span = new ReadOnlySpan<T>((void*)ptr, count);
        return new BorrowedSpan<T>(parent, span, refTaken);
    }

    /// <summary>Releases the ref-count taken on the parent SafeHandle.</summary>
    public void Dispose()
    {
        if (_refTaken) _parent.DangerousRelease();
    }
}
