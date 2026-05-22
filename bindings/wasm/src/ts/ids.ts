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

declare const __nodeId: unique symbol;
declare const __refId: unique symbol;
declare const __refUid: unique symbol;
declare const __uid: unique symbol;
declare const __repId: unique symbol;
declare const __repUid: unique symbol;

/** Session-local identity of a topology node. */
export type NodeId = bigint & { readonly [__nodeId]: void };
/** Session-local identity of a reference entry. */
export type RefId = bigint & { readonly [__refId]: void };
/** Persistent unique identity of a reference entry. */
export type RefUid = bigint & { readonly [__refUid]: void };
/** Persistent unique identity of a graph entity (stable across Compact). */
export type Uid = bigint & { readonly [__uid]: void };
/** Identity of a representation (geometry / mesh data). */
export type RepId = bigint & { readonly [__repId]: void };
/** Persistent identity of a representation. */
export type RepUid = bigint & { readonly [__repUid]: void };

/** Internal: construct a branded id from raw bits. */
export const NodeId = {
  fromBits: (bits: bigint): NodeId => bits as NodeId,
  invalid: (): NodeId => 0n as NodeId,
  isInvalid: (id: NodeId): boolean => id === 0n,
};
export const RefId = {
  fromBits: (bits: bigint): RefId => bits as RefId,
  invalid: (): RefId => 0n as RefId,
  isInvalid: (id: RefId): boolean => id === 0n,
};
export const RefUid = {
  fromBits: (bits: bigint): RefUid => bits as RefUid,
  invalid: (): RefUid => 0n as RefUid,
  isInvalid: (id: RefUid): boolean => id === 0n,
};
export const Uid = {
  fromBits: (bits: bigint): Uid => bits as Uid,
  invalid: (): Uid => 0n as Uid,
  isInvalid: (id: Uid): boolean => id === 0n,
};
export const RepId = {
  fromBits: (bits: bigint): RepId => bits as RepId,
  invalid: (): RepId => 0n as RepId,
  isInvalid: (id: RepId): boolean => id === 0n,
};
export const RepUid = {
  fromBits: (bits: bigint): RepUid => bits as RepUid,
  invalid: (): RepUid => 0n as RepUid,
  isInvalid: (id: RepUid): boolean => id === 0n,
};
