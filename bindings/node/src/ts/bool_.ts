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

import { loadAddon } from './_raw.js';
import { NotFoundError } from './errors.js';
import { Graph } from './handles.js';
import { type NodeId, nodeId } from './ids.js';

/** Tunable parameters shared by all five boolean operations. Mirrors
 *  #occtl_bool_options_t. Defaults match OCCT (set by the runtime initialiser
 *  in the trampoline); pass only the fields you need to override. */
export interface BoolOptions {
  fuzzyValue?:                  number;
  runParallel?:                 boolean;
  simplifyResult?:              boolean;
  simplifyAngularTolerance?:    number;
  simplifyLinearTolerance?:     number;
  buildHistory?:                boolean;
}

function toOptions(opts: BoolOptions | undefined): Record<string, unknown> {
  const fuzzyValue = opts?.fuzzyValue;
  const runParallel = opts?.runParallel;
  const simplifyResult = opts?.simplifyResult;
  const simplifyAngularTolerance = opts?.simplifyAngularTolerance;
  const simplifyLinearTolerance = opts?.simplifyLinearTolerance;
  const buildHistory = opts?.buildHistory;

  const o: Record<string, unknown> = {};
  if (fuzzyValue !== undefined)                o.fuzzy_value = fuzzyValue;
  if (runParallel !== undefined)               o.run_parallel = runParallel ? 1 : 0;
  if (simplifyResult !== undefined)            o.simplify_result = simplifyResult ? 1 : 0;
  if (simplifyAngularTolerance !== undefined)  o.simplify_angular_tolerance = simplifyAngularTolerance;
  if (simplifyLinearTolerance !== undefined)   o.simplify_linear_tolerance = simplifyLinearTolerance;
  if (buildHistory !== undefined)              o.build_history = buildHistory ? 1 : 0;
  return o;
}

function invoke(
  fnKey: string,
  graph: Graph,
  objects: readonly NodeId[],
  tools: readonly NodeId[],
  opts?: BoolOptions,
): NodeId {
  const addon = loadAddon();
  const raw = addon.call(fnKey, [
    (graph as unknown as { _native(): unknown })._native(),
    objects, tools, toOptions(opts),
  ]) as bigint;
  return nodeId(raw);
}

export function isMissingHistoryInput(error: unknown): boolean {
  return error instanceof NotFoundError && error.message.includes('input_uid');
}

/** Boolean Fuse (union) of two argument groups. */
export function fuse(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
  return invoke('bool_fuse', graph, objects, tools, opts);
}

/** Boolean Cut (objects minus tools). */
export function cut(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
  return invoke('bool_cut', graph, objects, tools, opts);
}

/** Boolean Common (intersection) of two argument groups. */
export function common(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
  return invoke('bool_common', graph, objects, tools, opts);
}

/** Boolean Section: the intersection edges and vertices of all arguments. */
export function section(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
  return invoke('bool_section', graph, objects, tools, opts);
}

/** Boolean Split: split each object using the tools as cutters. */
export function split(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
  return invoke('bool_split', graph, objects, tools, opts);
}
