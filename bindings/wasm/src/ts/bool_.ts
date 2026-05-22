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

import type { LoadedOcctl } from "./abi.js";
import { check } from "./abi.js";
import type { Graph } from "./topo.js";
import type { Uid } from "./ids.js";
import { NodeId } from "./ids.js";
import { UnsupportedError } from "./errors.js";
import { FUNCTION_NAMES_BOOL_ } from "./generated/bool_.js";

export { FUNCTION_NAMES_BOOL_ };

const OPTIONS_SIZE = 40;
const OFFSET_STRUCT_VERSION = 0;
const OFFSET_FUZZY_VALUE = 8;
const OFFSET_RUN_PARALLEL = 16;
const OFFSET_SIMPLIFY_RESULT = 20;
const OFFSET_SIMPLIFY_ANG = 24;
const OFFSET_BUILD_HISTORY = 32;

/** Tunable parameters shared by all five boolean operations. */
export interface BoolOptions
{
  /** Additional tolerance applied to all inputs; 0 keeps the default. */
  fuzzyValue?:                number;
  /** Enables parallel execution. */
  runParallel?:               boolean;
  /** Simplifies the result topology after the build. */
  simplifyResult?:            boolean;
  /** Angular tolerance for result simplification; used when @c simplifyResult is set. */
  simplifyAngularTolerance?:  number;
  /** Toggles graph-owned change-history collection. */
  buildHistory?:              boolean;
}

export class BoolModule
{
  private readonly myOcctl: LoadedOcctl;

  constructor(occtl: LoadedOcctl)
  {
    this.myOcctl = occtl;
  }

  /** Boolean Fuse (union) of two argument groups. */
  fuse(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId
  {
    return this.run("occtl_bool_fuse", graph, objects, tools, options);
  }

  /** Boolean Cut (objects minus tools). */
  cut(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId
  {
    return this.run("occtl_bool_cut", graph, objects, tools, options);
  }

  /** Boolean Common (intersection) of two argument groups. */
  common(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId
  {
    return this.run("occtl_bool_common", graph, objects, tools, options);
  }

  /** Boolean Section: intersection edges and vertices of all arguments. */
  section(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId
  {
    return this.run("occtl_bool_section", graph, objects, tools, options);
  }

  /** Boolean Split: split each object using the tools as cutters. */
  split(graph: Graph, objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId
  {
    return this.run("occtl_bool_split", graph, objects, tools, options);
  }

  private run(
    symbol: string,
    graph: Graph,
    objects: readonly NodeId[],
    tools: readonly NodeId[],
    options?: BoolOptions,
  ): NodeId
  {
    const mod = this.myOcctl.raw;
    const optsPtr = mod._malloc(OPTIONS_SIZE);
    const objsPtr = mod._malloc(Math.max(objects.length, 1) * 8);
    const toolsPtr = mod._malloc(Math.max(tools.length, 1) * 8);
    const rootPtr = mod._malloc(8);

    try
    {
      // Initialise options through the C ABI so defaults match exactly.
      const initFn = (mod as unknown as Record<string, (p: number) => void>)["occtl_bool_options_init"];
      if (initFn)
      {
        initFn(optsPtr);
      }
      else
      {
        for (let i = 0; i < OPTIONS_SIZE; ++i) mod.setValue(optsPtr + i, 0, "i8");
        mod.setValue(optsPtr + OFFSET_STRUCT_VERSION, 1, "i32");
        mod.setValue(optsPtr + OFFSET_BUILD_HISTORY,  1, "i32");
      }

      // Apply caller overrides.
      const fuzzyValue = options?.fuzzyValue;
      const runParallel = options?.runParallel;
      const simplifyResult = options?.simplifyResult;
      const simplifyAngularTolerance = options?.simplifyAngularTolerance;
      const buildHistory = options?.buildHistory ?? true;

      if (fuzzyValue !== undefined)
        mod.setValue(optsPtr + OFFSET_FUZZY_VALUE, fuzzyValue, "double");
      if (runParallel !== undefined)
        mod.setValue(optsPtr + OFFSET_RUN_PARALLEL, runParallel ? 1 : 0, "i32");
      if (simplifyResult !== undefined)
        mod.setValue(optsPtr + OFFSET_SIMPLIFY_RESULT, simplifyResult ? 1 : 0, "i32");
      if (simplifyAngularTolerance !== undefined)
        mod.setValue(optsPtr + OFFSET_SIMPLIFY_ANG, simplifyAngularTolerance, "double");
      mod.setValue(optsPtr + OFFSET_BUILD_HISTORY, buildHistory ? 1 : 0, "i32");

      // Pack node-id arrays (8 bytes each, little-endian uint64).
      for (let i = 0; i < objects.length; ++i) this.writeNodeIdAt(objsPtr + i * 8, objects[i]);
      for (let i = 0; i < tools.length;   ++i) this.writeNodeIdAt(toolsPtr + i * 8, tools[i]);

      const boundFn = (mod as unknown as Record<string, (graph: number, objects: number, nObjects: number, tools: number, nTools: number, opts: number, outRoot: number) => number>)[symbol];
      if (!boundFn) {
        throw new UnsupportedError(
          `Feature 'bool' is not available in this OCCT-Light build (missing ${symbol})`,
          0n as Uid,
          0,
        );
      }

      const status = boundFn(
        graph.pointer,
        objsPtr,  objects.length,
        toolsPtr, tools.length,
        optsPtr,
        rootPtr,
      );
      check(mod, status);

      const lo = BigInt.asUintN(32, BigInt(mod.getValue(rootPtr,     "i32") >>> 0));
      const hi = BigInt.asUintN(32, BigInt(mod.getValue(rootPtr + 4, "i32") >>> 0));
      return NodeId.fromBits((hi << 32n) | lo);
    }
    finally
    {
      mod._free(optsPtr);
      mod._free(objsPtr);
      mod._free(toolsPtr);
      mod._free(rootPtr);
    }
  }

  private writeNodeIdAt(ptr: number, id: NodeId): void
  {
    const mod = this.myOcctl.raw;
    const bits: bigint = id;
    mod.setValue(ptr,     Number(bits & 0xFFFFFFFFn),         "i32");
    mod.setValue(ptr + 4, Number((bits >> 32n) & 0xFFFFFFFFn), "i32");
  }
}
