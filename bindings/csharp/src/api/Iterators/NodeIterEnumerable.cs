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
using System.Collections;
using System.Collections.Generic;
using OcctL.Native;
using OcctL.SafeHandles;

namespace OcctL.Iterators;

/// <summary>
/// Wraps a single-pass <c>occtl_node_iter_t*</c> as an <see cref="IEnumerable{T}"/> of
/// <see cref="NodeId"/>. The iterator handle is consumed by the first enumeration; the
/// enumerable holds a single-use handle and yields <see cref="NodeId"/> values until the
/// native side returns <c>OCCTL_NOT_FOUND</c>.
/// </summary>
public sealed class NodeIterEnumerable : IEnumerable<NodeId>, IDisposable
{
    private NodeIterHandle? _handle;
    private bool _enumerated;

    internal NodeIterEnumerable(NodeIterHandle handle)
    {
        _handle = handle;
    }

    /// <summary>Returns the single-pass enumerator over native node IDs.</summary>
    public IEnumerator<NodeId> GetEnumerator()
    {
        if (_handle is null) throw new ObjectDisposedException(nameof(NodeIterEnumerable));
        if (_enumerated) throw new InvalidOperationException(
            "NodeIterEnumerable is single-pass; iterate only once.");
        _enumerated = true;
        return new Enumerator(_handle);
    }

    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

    /// <summary>Releases the native iterator handle if it is still owned.</summary>
    public void Dispose()
    {
        _handle?.Dispose();
        _handle = null;
    }

    private sealed class Enumerator : IEnumerator<NodeId>
    {
        private readonly NodeIterHandle _handle;
        private NodeId _current;

        public Enumerator(NodeIterHandle handle) { _handle = handle; }

        public NodeId Current => _current;
        object IEnumerator.Current => _current;

        public bool MoveNext()
        {
            if (_handle.IsInvalid) return false;
            OcctlStatus s = NativeMethods.OcctlNodeIterNext(_handle.DangerousGetHandle(), out OcctlNodeId outId);
            if (s == OcctlStatus.NotFound) { _current = NodeId.Invalid; return false; }
            OcctlException.Check(s);
            _current = new NodeId(outId.Bits);
            return true;
        }

        public void Reset() => throw new NotSupportedException("OCCT-Light iterators are single-pass.");

        public void Dispose()
        {
            // The owning NodeIterEnumerable disposes the handle.
        }
    }
}
