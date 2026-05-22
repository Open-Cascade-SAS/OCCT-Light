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

declare const __nodeIdBrand: unique symbol;
declare const __uidBrand: unique symbol;
declare const __refIdBrand: unique symbol;
declare const __refUidBrand: unique symbol;
declare const __repIdBrand: unique symbol;
declare const __repUidBrand: unique symbol;
declare const __jointIdBrand: unique symbol;

export type NodeId = bigint & { readonly [__nodeIdBrand]: 'NodeId' };
export type Uid    = bigint & { readonly [__uidBrand]:    'Uid' };
export type RefId  = bigint & { readonly [__refIdBrand]:  'RefId' };
export type RefUid = bigint & { readonly [__refUidBrand]: 'RefUid' };
export type RepId  = bigint & { readonly [__repIdBrand]:  'RepId' };
export type RepUid = bigint & { readonly [__repUidBrand]: 'RepUid' };
export type JointId = bigint & { readonly [__jointIdBrand]: 'JointId' };

/** Build a NodeId from raw bits. The binding uses this internally; userland code
 *  should not fabricate IDs — receive them from the C ABI. */
export function nodeId(bits: bigint): NodeId { return bits as NodeId; }
export function uid(bits: bigint): Uid       { return bits as Uid; }
export function refId(bits: bigint): RefId   { return bits as RefId; }
export function refUid(bits: bigint): RefUid { return bits as RefUid; }
export function repId(bits: bigint): RepId   { return bits as RepId; }
export function repUid(bits: bigint): RepUid { return bits as RepUid; }
export function jointId(bits: bigint): JointId { return bits as JointId; }

export const NODE_ID_INVALID: NodeId = nodeId(0n);
export const UID_INVALID:     Uid    = uid(0n);
export const REF_ID_INVALID:  RefId  = refId(0n);
export const REF_UID_INVALID: RefUid = refUid(0n);
export const REP_ID_INVALID:  RepId  = repId(0n);
export const REP_UID_INVALID: RepUid = repUid(0n);
export const JOINT_ID_INVALID: JointId = jointId(0n);

/** Equality on IDs — bigint comparison; exported for ergonomics so users don't
 *  have to know the runtime is bigint. */
export function nodeIdEq(a: NodeId, b: NodeId): boolean { return a === b; }
export function uidEq(a: Uid, b: Uid): boolean { return a === b; }
export function refUidEq(a: RefUid, b: RefUid): boolean { return a === b; }
export function repUidEq(a: RepUid, b: RepUid): boolean { return a === b; }
export function jointIdEq(a: JointId, b: JointId): boolean { return a === b; }
