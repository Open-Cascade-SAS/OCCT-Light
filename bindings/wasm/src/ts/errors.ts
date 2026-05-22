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

import type { Uid } from "./ids.js";

/**
 * Numeric copy of occtl_status_t. Kept in lockstep with
 * `include/occtl/occtl_core.h`. Re-checked at load-time against
 * `_manifest.ts` / `occtl_runtime_abi_version()`.
 */
export const Status = Object.freeze({
  OK:                   0,
  ERROR:                1,
  INVALID_ARGUMENT:     2,
  INVALID_HANDLE:       3,
  NOT_FOUND:            4,
  OUT_OF_MEMORY:        5,
  OUT_OF_RANGE:         6,
  NOT_DONE:             7,
  GEOMETRY_INVALID:     8,
  TOPOLOGY_INVALID:     9,
  IO_ERROR:            10,
  FORMAT_ERROR:        11,
  UNSUPPORTED:         12,
  CANCELLED:           13,
  BUFFER_TOO_SMALL:    14,
  VERSION_MISMATCH:    15,
  INTERNAL:            16,
  WRONG_KIND:          17,
} as const);
export type StatusCode = typeof Status[keyof typeof Status];

/** Base class for every OCCT-Light failure. */
export class OcctLError extends Error {
  readonly status: StatusCode;
  readonly source: Uid;
  readonly extended: number;
  constructor(status: StatusCode, message: string, source: Uid, extended: number) {
    super(message);
    this.name = "OcctLError";
    this.status = status;
    this.source = source;
    this.extended = extended;
  }
}

// Per-status subclasses. Names match @occtl/node exactly.
export class GenericError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.ERROR, m, s, e); this.name = "GenericError"; } }
export class InvalidArgumentError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.INVALID_ARGUMENT, m, s, e); this.name = "InvalidArgumentError"; } }
export class InvalidHandleError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.INVALID_HANDLE, m, s, e); this.name = "InvalidHandleError"; } }
export class NotFoundError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.NOT_FOUND, m, s, e); this.name = "NotFoundError"; } }
export class OutOfMemoryError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.OUT_OF_MEMORY, m, s, e); this.name = "OutOfMemoryError"; } }
export class OutOfRangeError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.OUT_OF_RANGE, m, s, e); this.name = "OutOfRangeError"; } }
export class NotDoneError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.NOT_DONE, m, s, e); this.name = "NotDoneError"; } }
export class GeometryInvalidError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.GEOMETRY_INVALID, m, s, e); this.name = "GeometryInvalidError"; } }
export class TopologyInvalidError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.TOPOLOGY_INVALID, m, s, e); this.name = "TopologyInvalidError"; } }
export class IoError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.IO_ERROR, m, s, e); this.name = "IoError"; } }
export class FormatError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.FORMAT_ERROR, m, s, e); this.name = "FormatError"; } }
export class UnsupportedError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.UNSUPPORTED, m, s, e); this.name = "UnsupportedError"; } }
export class CancelledError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.CANCELLED, m, s, e); this.name = "CancelledError"; } }
export class BufferTooSmallError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.BUFFER_TOO_SMALL, m, s, e); this.name = "BufferTooSmallError"; } }
export class VersionMismatchError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.VERSION_MISMATCH, m, s, e); this.name = "VersionMismatchError"; } }
export class InternalError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.INTERNAL, m, s, e); this.name = "InternalError"; } }
export class WrongKindError extends OcctLError { constructor(m: string, s: Uid, e: number) { super(Status.WRONG_KIND, m, s, e); this.name = "WrongKindError"; } }

/** Raised by Occtl.load() when the runtime ABI doesn't match build-time. */
export class AbiMismatchError extends Error {
  readonly expected: number;
  readonly actual: number;
  constructor(expected: number, actual: number) {
    super(`OCCT-Light ABI mismatch: binding built for v${expected}, runtime reports v${actual}`);
    this.name = "AbiMismatchError";
    this.expected = expected;
    this.actual = actual;
  }
}

/**
 * Raised when a HeapView is accessed after the WASM heap has grown.
 * See lib/views.ts and README.md.
 */
export class WasmHeapInvalidatedError extends Error {
  constructor(operation = "read") {
    super(`Heap view invalidated by a prior WASM call; cannot ${operation}. ` +
          `Call .copy() to materialise an owned Float64Array before the next OCCT-Light call.`);
    this.name = "WasmHeapInvalidatedError";
  }
}

/**
 * Constructs the right subclass given a primary status code.
 */
export function makeErrorForStatus(status: number, message: string, source: Uid, extended: number): OcctLError {
  switch (status) {
    case Status.INVALID_ARGUMENT:  return new InvalidArgumentError(message, source, extended);
    case Status.INVALID_HANDLE:    return new InvalidHandleError(message, source, extended);
    case Status.NOT_FOUND:         return new NotFoundError(message, source, extended);
    case Status.OUT_OF_MEMORY:     return new OutOfMemoryError(message, source, extended);
    case Status.OUT_OF_RANGE:      return new OutOfRangeError(message, source, extended);
    case Status.NOT_DONE:          return new NotDoneError(message, source, extended);
    case Status.GEOMETRY_INVALID:  return new GeometryInvalidError(message, source, extended);
    case Status.TOPOLOGY_INVALID:  return new TopologyInvalidError(message, source, extended);
    case Status.IO_ERROR:          return new IoError(message, source, extended);
    case Status.FORMAT_ERROR:      return new FormatError(message, source, extended);
    case Status.UNSUPPORTED:       return new UnsupportedError(message, source, extended);
    case Status.CANCELLED:         return new CancelledError(message, source, extended);
    case Status.BUFFER_TOO_SMALL:  return new BufferTooSmallError(message, source, extended);
    case Status.VERSION_MISMATCH:  return new VersionMismatchError(message, source, extended);
    case Status.INTERNAL:          return new InternalError(message, source, extended);
    case Status.WRONG_KIND:        return new WrongKindError(message, source, extended);
    default:                       return new GenericError(message, source, extended);
  }
}
