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
/// Base exception type for every translated OCCT-Light failure.
/// </summary>
/// <remarks>
/// One subclass per concrete <see cref="OcctlStatus"/> value (see below). Translated
/// from the thread-local error slot populated by every native entry point — read via
/// <c>occtl_error_last</c>.
/// </remarks>
public class OcctlException : Exception
{
    /// <summary>Status code returned by the native OCCT-Light entry point.</summary>
    public OcctlStatus Status { get; }
    /// <summary>UID of the offending entity, or <see cref="OcctlUid"/>'s default if not applicable.
    /// Distinct from <see cref="Exception.Source"/> (the string-typed source assembly name).</summary>
    public new OcctlUid Source { get; }
    /// <summary>Module-specific extended error code.</summary>
    public uint Extended { get; }

    /// <summary>Creates an exception from a native status and last-error payload.</summary>
    public OcctlException(OcctlStatus status, string message, OcctlUid source, uint extended)
        : base(message)
    {
        Status = status;
        Source = source;
        Extended = extended;
    }

    /// <summary>
    /// Throws if <paramref name="status"/> is not <see cref="OcctlStatus.Ok"/>.
    /// Pulls the thread-local error slot to build the right subclass.
    /// </summary>
    public static void Check(OcctlStatus status)
    {
        if (status == OcctlStatus.Ok) return;
        throw FromLastError(status);
    }

    internal static OcctlException FromLastError(OcctlStatus status)
    {
        IntPtr errPtr = NativeMethods.OcctlErrorLast();
        string message = $"OCCT-Light failure: {status}";
        OcctlUid source = default;
        uint extended = 0;
        if (errPtr != IntPtr.Zero)
        {
            OcctlError err = Marshal.PtrToStructure<OcctlError>(errPtr);
            if (err.Message != IntPtr.Zero)
            {
                string? m = Marshal.PtrToStringUTF8(err.Message);
                if (!string.IsNullOrEmpty(m)) message = m;
            }
            source = err.Source;
            extended = err.Extended;
        }
        return status switch
        {
            OcctlStatus.Error             => new GenericFailureException(status, message, source, extended),
            OcctlStatus.InvalidArgument   => new InvalidArgumentException(status, message, source, extended),
            OcctlStatus.InvalidHandle     => new InvalidHandleException(status, message, source, extended),
            OcctlStatus.NotFound          => new NotFoundException(status, message, source, extended),
            OcctlStatus.OutOfMemory       => new OutOfMemoryStatusException(status, message, source, extended),
            OcctlStatus.OutOfRange        => new OutOfRangeException(status, message, source, extended),
            OcctlStatus.NotDone           => new NotDoneException(status, message, source, extended),
            OcctlStatus.GeometryInvalid   => new GeometryInvalidException(status, message, source, extended),
            OcctlStatus.TopologyInvalid   => new TopologyInvalidException(status, message, source, extended),
            OcctlStatus.IoError           => new IoErrorException(status, message, source, extended),
            OcctlStatus.FormatError       => new FormatErrorException(status, message, source, extended),
            OcctlStatus.Unsupported       => new UnsupportedException(status, message, source, extended),
            OcctlStatus.Cancelled         => new CancelledException(status, message, source, extended),
            OcctlStatus.BufferTooSmall    => new BufferTooSmallException(status, message, source, extended),
            OcctlStatus.VersionMismatch   => new VersionMismatchException(status, message, source, extended),
            OcctlStatus.Internal          => new InternalException(status, message, source, extended),
            OcctlStatus.WrongKind         => new WrongKindException(status, message, source, extended),
            _                             => new OcctlException(status, message, source, extended),
        };
    }
}

/// <summary>Thrown on <see cref="OcctlStatus.Error"/> — a generic, unrecoverable failure.</summary>
public sealed class GenericFailureException   : OcctlException { internal GenericFailureException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.InvalidArgument"/> — a parameter was NULL, out-of-range, or of the wrong type.</summary>
public sealed class InvalidArgumentException  : OcctlException { internal InvalidArgumentException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.InvalidHandle"/> — an opaque handle was NULL or already freed.</summary>
public sealed class InvalidHandleException    : OcctlException { internal InvalidHandleException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.NotFound"/> — a requested entity was not found.</summary>
public sealed class NotFoundException         : OcctlException { internal NotFoundException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.OutOfMemory"/> — a memory allocation failed.</summary>
public sealed class OutOfMemoryStatusException: OcctlException { internal OutOfMemoryStatusException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.OutOfRange"/> — a numeric value is outside its valid range.</summary>
public sealed class OutOfRangeException       : OcctlException { internal OutOfRangeException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.NotDone"/> — a multi-step algorithm was not built.</summary>
public sealed class NotDoneException          : OcctlException { internal NotDoneException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.GeometryInvalid"/> — input geometry is degenerate or self-contradictory.</summary>
public sealed class GeometryInvalidException  : OcctlException { internal GeometryInvalidException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.TopologyInvalid"/> — input topology is invalid.</summary>
public sealed class TopologyInvalidException  : OcctlException { internal TopologyInvalidException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.IoError"/> — a disk or I/O operation failed.</summary>
public sealed class IoErrorException          : OcctlException { internal IoErrorException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.FormatError"/> — a file format could not be parsed.</summary>
public sealed class FormatErrorException      : OcctlException { internal FormatErrorException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.Unsupported"/> — a feature or format is not supported by this build.</summary>
public sealed class UnsupportedException      : OcctlException { internal UnsupportedException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.Cancelled"/> — an operation was cancelled.</summary>
public sealed class CancelledException        : OcctlException { internal CancelledException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.BufferTooSmall"/> — a caller-supplied buffer is too small.</summary>
public sealed class BufferTooSmallException   : OcctlException { internal BufferTooSmallException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.VersionMismatch"/> — the options struct's struct_version is unrecognised.</summary>
public sealed class VersionMismatchException  : OcctlException { internal VersionMismatchException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.Internal"/> — an unexpected internal error occurred.</summary>
public sealed class InternalException         : OcctlException { internal InternalException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }
/// <summary>Thrown on <see cref="OcctlStatus.WrongKind"/> — the handle does not support the requested operation.</summary>
public sealed class WrongKindException        : OcctlException { internal WrongKindException(OcctlStatus s, string m, OcctlUid src, uint ext) : base(s, m, src, ext) {} }

/// <summary>Thrown when the runtime ABI version differs from the binding's compile-time constant.</summary>
public sealed class AbiMismatchException : Exception
{
    /// <summary>Creates an ABI mismatch exception.</summary>
    public AbiMismatchException(uint expected, uint actual)
        : base($"OCCT-Light ABI version mismatch: binding expected {expected}, native reports {actual}.")
    {
        Expected = expected;
        Actual = actual;
    }
    /// <summary>ABI version expected by this binding.</summary>
    public uint Expected { get; }
    /// <summary>ABI version reported by the loaded native library.</summary>
    public uint Actual { get; }
}
