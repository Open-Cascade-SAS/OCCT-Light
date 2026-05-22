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

import type { Uid } from './ids.js';

export enum Status {
  Ok                = 0,
  Error             = 1,
  InvalidArgument   = 2,
  InvalidHandle     = 3,
  NotFound          = 4,
  OutOfMemory       = 5,
  OutOfRange        = 6,
  NotDone           = 7,
  GeometryInvalid   = 8,
  TopologyInvalid   = 9,
  IoError           = 10,
  FormatError       = 11,
  Unsupported       = 12,
  Cancelled         = 13,
  BufferTooSmall    = 14,
  VersionMismatch   = 15,
  Internal          = 16,
  WrongKind         = 17,
}

export interface OcctLErrorInit {
  readonly status:   Status;
  readonly message:  string;
  readonly source:   Uid;
  readonly extended: number;
}

export class OcctLError extends Error {
  public readonly status:   Status;
  public readonly source:   Uid;
  public readonly extended: number;
  constructor(init: OcctLErrorInit) {
    super(init.message || `OCCT-Light status ${Status[init.status] ?? init.status}`);
    this.name     = 'OcctLError';
    this.status   = init.status;
    this.source   = init.source;
    this.extended = init.extended;
  }
}

// One subclass per status code. The name field matches the C enum suffix so
// stack-trace inspection in any language is recognisable.
/** Status 1 — a generic, unrecoverable failure with no more-specific code. */
export class GenericError          extends OcctLError { public override readonly name = 'GenericError'; }
/** Status 2 — an argument to the call was invalid (NULL, out-of-range, wrong type). */
export class InvalidArgumentError  extends OcctLError { public override readonly name = 'InvalidArgumentError'; }
/** Status 3 — a handle (`occtl_graph_t*`, etc.) was NULL or already freed. */
export class InvalidHandleError    extends OcctLError { public override readonly name = 'InvalidHandleError'; }
/** Status 4 — a requested entity (NodeId, file, font) was not found. */
export class NotFoundError         extends OcctLError { public override readonly name = 'NotFoundError'; }
/** Status 5 — a memory allocation failed inside OCCT or the wrapper. */
export class OutOfMemoryError      extends OcctLError { public override readonly name = 'OutOfMemoryError'; }
/** Status 6 — a numeric value is outside its valid range. */
export class OutOfRangeError       extends OcctLError { public override readonly name = 'OutOfRangeError'; }
/** Status 7 — a multi-step algorithm has not been run to completion (call `Build()` first). */
export class NotDoneError          extends OcctLError { public override readonly name = 'NotDoneError'; }
/** Status 8 — input geometry is degenerate or self-contradictory. */
export class GeometryInvalidError  extends OcctLError { public override readonly name = 'GeometryInvalidError'; }
/** Status 9 — input topology is invalid (non-manifold, non-orientable, etc.). */
export class TopologyInvalidError  extends OcctLError { public override readonly name = 'TopologyInvalidError'; }
/** Status 10 — a disk or I/O operation failed. */
export class IoError               extends OcctLError { public override readonly name = 'IoError'; }
/** Status 11 — a file format could not be parsed. */
export class FormatError           extends OcctLError { public override readonly name = 'FormatError'; }
/** Status 12 — a feature or format is not supported by this build of the library. */
export class UnsupportedError      extends OcctLError { public override readonly name = 'UnsupportedError'; }
/** Status 13 — an asynchronous or long-running operation was cancelled. */
export class CancelledError        extends OcctLError { public override readonly name = 'CancelledError'; }
/** Status 14 — a caller-supplied buffer is too small (re-allocate and retry). */
export class BufferTooSmallError   extends OcctLError { public override readonly name = 'BufferTooSmallError'; }
/** Status 15 — the options struct's `struct_version` is unrecognised. */
export class VersionMismatchError  extends OcctLError { public override readonly name = 'VersionMismatchError'; }
/** Status 16 — an unexpected internal error occurred inside OCCT. */
export class InternalError         extends OcctLError { public override readonly name = 'InternalError'; }
/** Status 17 — an opaque handle is of a kind that does not support the requested operation. */
export class WrongKindError        extends OcctLError { public override readonly name = 'WrongKindError'; }
/** Thrown at module load when compile-time and runtime ABI versions differ. */
export class AbiMismatchError      extends OcctLError { public override readonly name = 'AbiMismatchError'; }

const STATUS_TO_CLASS: Record<number, new (init: OcctLErrorInit) => OcctLError> = {
  [Status.Error]:            GenericError,
  [Status.InvalidArgument]:  InvalidArgumentError,
  [Status.InvalidHandle]:    InvalidHandleError,
  [Status.NotFound]:         NotFoundError,
  [Status.OutOfMemory]:      OutOfMemoryError,
  [Status.OutOfRange]:       OutOfRangeError,
  [Status.NotDone]:          NotDoneError,
  [Status.GeometryInvalid]:  GeometryInvalidError,
  [Status.TopologyInvalid]:  TopologyInvalidError,
  [Status.IoError]:          IoError,
  [Status.FormatError]:      FormatError,
  [Status.Unsupported]:      UnsupportedError,
  [Status.Cancelled]:        CancelledError,
  [Status.BufferTooSmall]:   BufferTooSmallError,
  [Status.VersionMismatch]:  VersionMismatchError,
  [Status.Internal]:         InternalError,
  [Status.WrongKind]:        WrongKindError,
};

export function errorForStatus(init: OcctLErrorInit): OcctLError {
  const Klass = STATUS_TO_CLASS[init.status] ?? OcctLError;
  return new Klass(init);
}

/** Throws the appropriate typed error for a non-OK status. */
export function check(status: number, info?: Partial<OcctLErrorInit>): void {
  if (status === Status.Ok) return;
  throw errorForStatus({
    status:   status as Status,
    message:  info?.message  ?? '',
    source:   info?.source   ?? (0n as Uid),
    extended: info?.extended ?? 0,
  });
}
