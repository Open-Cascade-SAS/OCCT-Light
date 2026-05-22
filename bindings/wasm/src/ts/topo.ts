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
import { Disposable } from "./handles.js";
import { NodeId, RefId, RefUid, RepId, RepUid, Uid } from "./ids.js";
import { Status, makeErrorForStatus } from "./errors.js";
import { FUNCTION_NAMES_TOPO, bindRawTopo } from "./generated/topo.js";
import { occtl_node_kind } from "./generated/core.js";
import { STRUCT_LAYOUTS } from "./generated/_layouts.js";
import type { Point3 } from "./geom.js";
import { PrimModule } from "./prim.js";
import type { BoxInfo, ConeInfo, CylinderInfo, SphereInfo, TorusInfo, WedgeInfo } from "./prim.js";
import { BoolModule } from "./bool_.js";
import type { BoolOptions } from "./bool_.js";

export { FUNCTION_NAMES_TOPO };
export const NodeKind = occtl_node_kind;
export type NodeKind = typeof NodeKind[keyof typeof NodeKind];
export type NodeKindName =
  | "solid"
  | "shell"
  | "face"
  | "wire"
  | "edge"
  | "vertex"
  | "compound"
  | "compsolid"
  | "coedge"
  | "product"
  | "occurrence"
  | "unknown";

export interface GraphRootResult {
  graph: Graph;
  root: NodeId;
}

export interface CheckIssue {
  nodeId: NodeId;
  contextNodeId: NodeId;
  statusBit: number;
  severity: number;
}

export interface BrepWriteOptions {
  writeTriangulation?: boolean;
}

/**
 * A topology graph (BRepGraph). Owns a heap-allocated `occtl_graph_t*`.
 * Dispose with .dispose() or `using`.
 */
export class Graph extends Disposable {
  private readonly _raw: ReturnType<typeof bindRawTopo>;
  private readonly _occtl: LoadedOcctl;
  private readonly _prim: PrimModule;
  private readonly _bool: BoolModule;

  private constructor(occtl: LoadedOcctl, ptr: number) {
    super(occtl.raw, ptr, "occtl_graph_t", (mod, p) => bindRawTopo(mod).occtlGraphFree(p));
    this._occtl = occtl;
    this._raw = bindRawTopo(occtl.raw);
    this._prim = new PrimModule(occtl);
    this._bool = new BoolModule(occtl);
  }

  /** Create a new empty topology graph. */
  static create(occtl: LoadedOcctl): Graph {
    const mod = occtl.raw;
    const raw = bindRawTopo(mod);
    const outPtr = mod._malloc(4);
    try {
      const status = raw.occtlGraphCreate(outPtr);
      check(mod, status);
      const ptr = mod.getValue(outPtr, "i32");
      return new Graph(occtl, ptr);
    } finally {
      mod._free(outPtr);
    }
  }

  /** Read a BRep file into a new graph and return its root node. */
  static readBrep(occtl: LoadedOcctl, path: string): GraphRootResult {
    const mod = occtl.raw;
    const pathPtr = Graph._writeCString(mod, path);
    const graphPtr = mod._malloc(4);
    const rootPtr = mod._malloc(8);
    try {
      const status = mod.occtl_io_brep_read(pathPtr, graphPtr, rootPtr);
      check(mod, status);
      const ptr = mod.getValue(graphPtr, "i32");
      return { graph: new Graph(occtl, ptr), root: Graph._readNodeId(mod, rootPtr) };
    } finally {
      mod._free(pathPtr);
      mod._free(graphPtr);
      mod._free(rootPtr);
    }
  }

  private static _writeCString(mod: LoadedOcctl["raw"], value: string): number {
    const bytes = mod.lengthBytesUTF8(value) + 1;
    const ptr = mod._malloc(bytes);
    mod.stringToUTF8(value, ptr, bytes);
    return ptr;
  }

  private static _readNodeId(mod: LoadedOcctl["raw"], ptr: number): NodeId {
    const lo = BigInt.asUintN(32, BigInt(mod.getValue(ptr + 0, "i32") >>> 0));
    const hi = BigInt.asUintN(32, BigInt(mod.getValue(ptr + 4, "i32") >>> 0));
    return NodeId.fromBits((hi << 32n) | lo);
  }

  /**
   * Allocate a node-id-sized output slot, run `fn(slotPtr)`, read out the
   * resulting NodeId, free the slot. Helper for OUT parameters.
   */
  private _withNodeIdOut(fn: (ptr: number) => number): NodeId {
    const mod = this._occtl.raw;
    const slot = mod._malloc(8); // occtl_node_id_t = struct { uint64_t bits; }
    try {
      const status = fn(slot);
      check(mod, status);
      const lo = BigInt.asUintN(32, BigInt(mod.getValue(slot + 0, "i32") >>> 0));
      const hi = BigInt.asUintN(32, BigInt(mod.getValue(slot + 4, "i32") >>> 0));
      return NodeId.fromBits((hi << 32n) | lo);
    } finally {
      mod._free(slot);
    }
  }

  private _count(fn: (graph: number, outCount: number) => number): number {
    const mod = this._occtl.raw;
    const slot = mod._malloc(4); // size_t on wasm32
    try {
      check(mod, fn(this.pointer, slot));
      return mod.getValue(slot, "i32") >>> 0;
    } finally {
      mod._free(slot);
    }
  }

  /** Append a vertex at the given 3D point and return its NodeId. */
  makeVertex(point: Point3): NodeId {
    const mod = this._occtl.raw;
    const infoLayout = STRUCT_LAYOUTS.occtl_topo_make_vertex_info_t;
    const infoPtr = mod._malloc(infoLayout.size);
    try {
      this._raw.occtlTopoMakeVertexInfoInit(infoPtr);
      const pointOffset = infoLayout.fields.point.offset;
      mod.setValue(infoPtr + pointOffset + 0, point.x, "double");
      mod.setValue(infoPtr + pointOffset + 8, point.y, "double");
      mod.setValue(infoPtr + pointOffset + 16, point.z, "double");
      return this._withNodeIdOut((idPtr) => this._raw.occtlTopoMakeVertex(this.pointer, infoPtr, idPtr));
    } finally {
      mod._free(infoPtr);
    }
  }

  /** Build an axis-aligned box solid in the graph. */
  makeBox(info: BoxInfo): NodeId {
    return this._prim.makeBox(this, info);
  }

  /** Build a sphere solid in the graph. */
  makeSphere(info: SphereInfo | number): NodeId {
    return this._prim.makeSphere(this, info);
  }

  /** Build a cylinder solid in the graph. */
  makeCylinder(info: CylinderInfo): NodeId {
    return this._prim.makeCylinder(this, info);
  }

  /** Build a cone or truncated-cone solid in the graph. */
  makeCone(info: ConeInfo): NodeId {
    return this._prim.makeCone(this, info);
  }

  /** Build a torus solid in the graph. */
  makeTorus(info: TorusInfo): NodeId {
    return this._prim.makeTorus(this, info);
  }

  /** Build a right-angular wedge solid in the graph. */
  makeWedge(info: WedgeInfo): NodeId {
    return this._prim.makeWedge(this, info);
  }

  /** Boolean Fuse (union) of object/tool groups. */
  fuse(objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId {
    return this._bool.fuse(this, objects, tools, options);
  }

  /** Boolean Cut (objects minus tools). */
  cut(objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId {
    return this._bool.cut(this, objects, tools, options);
  }

  /** Boolean Common (intersection). */
  common(objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId {
    return this._bool.common(this, objects, tools, options);
  }

  /** Boolean Section. */
  section(objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId {
    return this._bool.section(this, objects, tools, options);
  }

  /** Boolean Split. */
  split(objects: readonly NodeId[], tools: readonly NodeId[], options?: BoolOptions): NodeId {
    return this._bool.split(this, objects, tools, options);
  }

  /** Run topology validation and return reported issues. */
  checkIssues(): CheckIssue[] {
    const mod = this._occtl.raw;
    const countPtr = mod._malloc(4);
    try {
      let status = mod.occtl_topo_check(this.pointer, 0, 0, countPtr);
      check(mod, status);
      const count = mod.getValue(countPtr, "i32");
      if (count === 0) return [];

      const issueSize = 24;
      const issuesPtr = mod._malloc(count * issueSize);
      try {
        status = mod.occtl_topo_check(this.pointer, issuesPtr, count, countPtr);
        check(mod, status);
        const out: CheckIssue[] = [];
        const actual = mod.getValue(countPtr, "i32");
        for (let i = 0; i < actual; ++i) {
          const ptr = issuesPtr + i * issueSize;
          out.push({
            nodeId: Graph._readNodeId(mod, ptr + 0),
            contextNodeId: Graph._readNodeId(mod, ptr + 8),
            statusBit: mod.getValue(ptr + 16, "i32") >>> 0,
            severity: mod.getValue(ptr + 20, "i32"),
          });
        }
        return out;
      } finally {
        mod._free(issuesPtr);
      }
    } finally {
      mod._free(countPtr);
    }
  }

  /** True when topology validation reports no issues. */
  isValid(): boolean {
    return this.checkIssues().length === 0;
  }

  /** Write the topology rooted at `root` to a BRep file. */
  writeBrep(root: NodeId, path: string, options: BrepWriteOptions = {}): void {
    const mod = this._occtl.raw;
    const pathPtr = Graph._writeCString(mod, path);
    const optionsPtr = mod._malloc(24);
    try {
      mod.occtl_io_brep_write_options_init(optionsPtr);
      mod.setValue(optionsPtr + 8, options.writeTriangulation === false ? 0 : 1, "i32");
      const status = mod.occtl_io_brep_write(this.pointer, root as unknown as number, pathPtr, optionsPtr);
      check(mod, status);
    } finally {
      mod._free(pathPtr);
      mod._free(optionsPtr);
    }
  }

  /**
   * Iterate every vertex node in the graph.
   *
   * Owns the underlying `occtl_node_iter_t*` for the lifetime of the
   * generator. Closing the generator early (`.return()`) frees it.
   */
  *vertices(): Generator<NodeId> {
    yield* this._iterByKind("occtl_graph_vertices");
  }
  *edges(): Generator<NodeId> {
    yield* this._iterByKind("occtl_graph_edges");
  }
  *faces(): Generator<NodeId> {
    yield* this._iterByKind("occtl_graph_faces");
  }
  *wires(): Generator<NodeId> {
    yield* this._iterByKind("occtl_graph_wires");
  }
  *shells(): Generator<NodeId> {
    yield* this._iterByKind("occtl_graph_shells");
  }
  *solids(): Generator<NodeId> {
    yield* this._iterByKind("occtl_graph_solids");
  }

  /** Number of active compound nodes. */
  get nbCompounds(): number {
    return this._count(this._raw.occtlGraphCompoundCount);
  }

  /** Return the persistent UID for a node. */
  uidOf(node: NodeId): Uid {
    const mod = this._occtl.raw;
    const uidPtr = mod._malloc(8);
    try {
      const status = this._raw.occtlGraphUidFromNodeId(this.pointer, node as unknown as number, uidPtr);
      check(mod, status);
      const lo = BigInt.asUintN(32, BigInt(mod.getValue(uidPtr + 0, "i32") >>> 0));
      const hi = BigInt.asUintN(32, BigInt(mod.getValue(uidPtr + 4, "i32") >>> 0));
      return Uid.fromBits((hi << 32n) | lo);
    } finally {
      mod._free(uidPtr);
    }
  }

  /** Return the persistent UID for a live representation. */
  repUidOf(rep: RepId): RepUid {
    const mod = this._occtl.raw;
    const uidPtr = mod._malloc(8);
    try {
      const status = this._raw.occtlGraphRepUidFromRepId(this.pointer, rep as unknown as number, uidPtr);
      check(mod, status);
      return RepUid.fromBits(Graph._readU64(mod, uidPtr));
    } finally {
      mod._free(uidPtr);
    }
  }

  /** Resolve a persistent representation UID to the current representation id. */
  repIdOf(uid: RepUid): RepId {
    const mod = this._occtl.raw;
    const idPtr = mod._malloc(8);
    try {
      const status = this._raw.occtlGraphRepIdFromRepUid(this.pointer, uid as unknown as number, idPtr);
      check(mod, status);
      return RepId.fromBits(Graph._readU64(mod, idPtr));
    } finally {
      mod._free(idPtr);
    }
  }

  /** UIDs modified from inputUid in this graph's recorded history. */
  historyModified(inputUid: Uid): Uid[] {
    return this._fetchHistory(inputUid, this._raw.occtlGraphHistoryModified);
  }

  /** UIDs generated from inputUid in this graph's recorded history. */
  historyGenerated(inputUid: Uid): Uid[] {
    return this._fetchHistory(inputUid, this._raw.occtlGraphHistoryGenerated);
  }

  /** All UIDs deleted in this graph's recorded history. */
  historyDeletedAll(): Uid[] {
    const mod = this._occtl.raw;
    const countPtr = mod._malloc(4);
    try {
      let status = this._raw.occtlGraphHistoryDeletedAll(this.pointer, 0, 0, countPtr);
      if (status === Status.NOT_FOUND) return [];
      check(mod, status);
      const count = mod.getValue(countPtr, "i32");
      if (count === 0) return [];
      const bufPtr = mod._malloc(count * 8);
      try {
        status = this._raw.occtlGraphHistoryDeletedAll(this.pointer, bufPtr, count, countPtr);
        check(mod, status);
        return this._readUidArray(bufPtr, mod.getValue(countPtr, "i32"));
      } finally {
        mod._free(bufPtr);
      }
    } finally {
      mod._free(countPtr);
    }
  }

  private _fetchHistory(
    inputUid: Uid,
    fn: (graph: number, inputUid: number, outBuf: number, cap: number, outCount: number) => number,
  ): Uid[] {
    const mod = this._occtl.raw;
    const countPtr = mod._malloc(4);
    try {
      let status = fn(this.pointer, inputUid as unknown as number, 0, 0, countPtr);
      if (status === Status.NOT_FOUND) return [];
      check(mod, status);
      const count = mod.getValue(countPtr, "i32");
      if (count === 0) return [];
      const bufPtr = mod._malloc(count * 8);
      try {
        status = fn(this.pointer, inputUid as unknown as number, bufPtr, count, countPtr);
        check(mod, status);
        return this._readUidArray(bufPtr, mod.getValue(countPtr, "i32"));
      } finally {
        mod._free(bufPtr);
      }
    } finally {
      mod._free(countPtr);
    }
  }

  private _readUidArray(ptr: number, count: number): Uid[] {
    const mod = this._occtl.raw;
    const out: Uid[] = [];
    for (let i = 0; i < count; ++i) {
      const itemPtr = ptr + i * 8;
      const lo = BigInt.asUintN(32, BigInt(mod.getValue(itemPtr, "i32") >>> 0));
      const hi = BigInt.asUintN(32, BigInt(mod.getValue(itemPtr + 4, "i32") >>> 0));
      out.push(Uid.fromBits((hi << 32n) | lo));
    }
    return out;
  }

  private static _readU64(mod: LoadedOcctl["raw"], ptr: number): bigint {
    const lo = BigInt.asUintN(32, BigInt(mod.getValue(ptr + 0, "i32") >>> 0));
    const hi = BigInt.asUintN(32, BigInt(mod.getValue(ptr + 4, "i32") >>> 0));
    return (hi << 32n) | lo;
  }

  private static _writeU64(mod: LoadedOcctl["raw"], ptr: number, value: bigint): void {
    mod.setValue(ptr + 0, Number(value & 0xffffffffn), "i32");
    mod.setValue(ptr + 4, Number((value >> 32n) & 0xffffffffn), "i32");
  }

  /** Return the topology kind of a live node. */
  nodeKind(node: NodeId): NodeKindName {
    const mod = this._occtl.raw;
    const kindPtr = mod._malloc(4);
    try {
      const status = this._raw.occtlGraphNodeKind(this.pointer, node as unknown as number, kindPtr);
      check(mod, status);
      switch (mod.getValue(kindPtr, "i32")) {
        case NodeKind.OCCTL_KIND_SOLID: return "solid";
        case NodeKind.OCCTL_KIND_SHELL: return "shell";
        case NodeKind.OCCTL_KIND_FACE: return "face";
        case NodeKind.OCCTL_KIND_WIRE: return "wire";
        case NodeKind.OCCTL_KIND_EDGE: return "edge";
        case NodeKind.OCCTL_KIND_VERTEX: return "vertex";
        case NodeKind.OCCTL_KIND_COMPOUND: return "compound";
        case NodeKind.OCCTL_KIND_COMPSOLID: return "compsolid";
        case NodeKind.OCCTL_KIND_COEDGE: return "coedge";
        case NodeKind.OCCTL_KIND_PRODUCT: return "product";
        case NodeKind.OCCTL_KIND_OCCURRENCE: return "occurrence";
        default: return "unknown";
      }
    } finally {
      mod._free(kindPtr);
    }
  }

  private *_iterByKind(factoryName: string): Generator<NodeId> {
    const mod = this._occtl.raw;
    const factory = (mod as unknown as Record<string, (...a: number[]) => number>)[factoryName];
    if (!factory) {
      throw new Error(`${factoryName} not exported by this build`);
    }
    const itOut = mod._malloc(4);
    let iter = 0;
    try {
      const st = factory(this.pointer, itOut);
      check(mod, st);
      iter = mod.getValue(itOut, "i32");
      const idSlot = mod._malloc(8);
      try {
        while (true) {
          const step = mod.occtl_node_iter_next(iter, idSlot);
          if (step === Status.NOT_FOUND) return;
          if (step !== Status.OK) {
            // Surface as a typed exception; readLastError populated by call.
            check(mod, step);
          }
          const lo = BigInt.asUintN(32, BigInt(mod.getValue(idSlot + 0, "i32") >>> 0));
          const hi = BigInt.asUintN(32, BigInt(mod.getValue(idSlot + 4, "i32") >>> 0));
          yield NodeId.fromBits((hi << 32n) | lo);
        }
      } finally {
        mod._free(idSlot);
      }
    } finally {
      if (iter !== 0) mod.occtl_node_iter_free(iter);
      mod._free(itOut);
    }
  }

  /** Raw bindings escape hatch — same shape as in core.ts. */
  get raw(): ReturnType<typeof bindRawTopo> {
    return this._raw;
  }
}

// Re-export so userland import { NodeId, RefId, RepId, Graph } from "@occtl/wasm" works.
export { NodeId, RefId, RefUid, RepId, RepUid, Uid };
