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

import { loadAddon, type NativeGraph, type NativeNodeIter } from './_raw.js';
import { init } from './abi.js';
import { InvalidHandleError, NotFoundError, Status, UnsupportedError } from './errors.js';
import { type NodeId, nodeId, type RepId, repId, type RepUid, repUid, type Uid } from './ids.js';
import {
  type BoxInfo,
  type ConeInfo,
  type CylinderInfo,
  type SphereInfo,
  type TorusInfo,
  type WedgeInfo,
} from './generated/prim.js';

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

export interface StlWriteOptions {
  asciiMode?: boolean;
}

export interface VrmlWriteOptions {
  writerVersion?: number;
  representation?: number;
}

export interface BoolOptions {
  fuzzyValue?: number;
  runParallel?: boolean;
  simplifyResult?: boolean;
  simplifyAngularTolerance?: number;
  simplifyLinearTolerance?: number;
  buildHistory?: boolean;
}

export type BrepMemoryInput = Uint8Array | ArrayBuffer;

export type NodeKind =
  | 'solid'
  | 'shell'
  | 'face'
  | 'wire'
  | 'edge'
  | 'vertex'
  | 'compound'
  | 'compsolid'
  | 'coedge'
  | 'product'
  | 'occurrence'
  | 'unknown';

/** Common base for every Disposable handle. Holds the addon-side wrapper. */
abstract class HandleBase<T extends { close(): void; disposed: boolean }> {
  protected _h: T | null;
  protected constructor(handle: T) { this._h = handle; }

  /** Internal accessor for the bindings. Throws if disposed. */
  public _native(): T {
    if (!this._h || this._h.disposed) {
      throw new InvalidHandleError({
        status: Status.InvalidHandle,
        message: `${this.constructor.name} has been disposed`,
        source: 0n as never, extended: 0,
      });
    }
    return this._h;
  }

  public get disposed(): boolean { return !this._h || this._h.disposed; }

  public close(): void {
    if (this._h && !this._h.disposed) this._h.close();
    this._h = null;
  }

  public [Symbol.dispose](): void { this.close(); }
}

/** Topology graph — the central state for all topo/prim operations. */
export class Graph extends HandleBase<NativeGraph> {
  constructor(handle?: NativeGraph) {
    init();
    const addon = loadAddon();
    super(handle ?? new addon.Graph());
  }

  /** Hand-written module guard so reduced OCCT-Light builds fail with a typed status. */
  private static _requireExport(symbol: string, feature: string): void {
    const exported = loadAddon().EXPORTED_FUNCTIONS;
    if (!Array.isArray(exported) || exported.length === 0) return;
    const shortName = symbol.startsWith('occtl_') ? symbol.slice('occtl_'.length) : symbol;
    if (exported.includes(symbol) || exported.includes(shortName)) return;
    throw new UnsupportedError({
      status: Status.Unsupported,
      message: `Feature '${feature}' is not available in this OCCT-Light build (missing ${symbol})`,
      source: 0n as never,
      extended: 0,
    });
  }

  /** Create a new empty graph. Naming-aligned alias for `new Graph()`. */
  public static create(): Graph {
    return new Graph();
  }

  /** @internal Adopt a native graph handle returned by a C ABI call. */
  public static _adopt(handle: NativeGraph): Graph {
    return new Graph(handle);
  }

  /** Read a BRep file into a new graph and return its root node. */
  public static readBrep(path: string): GraphRootResult {
    const result = loadAddon().call('io_brep_read', [path]) as {
      out_graph: NativeGraph;
      out_root: bigint;
    };
    return { graph: Graph._adopt(result.out_graph), root: nodeId(result.out_root) };
  }

  /** Read an STL payload from memory into a new graph and return its root node. */
  public static readStlMemory(data: BrepMemoryInput): GraphRootResult {
    const result = loadAddon().call('io_stl_read_memory', [data]) as {
      out_graph: NativeGraph;
      out_root: bigint;
    };
    return { graph: Graph._adopt(result.out_graph), root: nodeId(result.out_root) };
  }

  /** Read a VRML payload from memory into a new graph and return its root node. */
  public static readVrmlMemory(data: BrepMemoryInput): GraphRootResult {
    const result = loadAddon().call('io_vrml_read_memory', [data]) as {
      out_graph: NativeGraph;
      out_root: bigint;
    };
    return { graph: Graph._adopt(result.out_graph), root: nodeId(result.out_root) };
  }

  /** Read a data-exchange payload from memory using an explicit format id. */
  public static readDeMemory(formatId: string, data: BrepMemoryInput): GraphRootResult {
    const result = loadAddon().call('de_read_memory', [formatId, data]) as {
      out_graph: NativeGraph;
      out_root: bigint;
    };
    return { graph: Graph._adopt(result.out_graph), root: nodeId(result.out_root) };
  }

  /** Build a fresh wrapper around a previously-created bigint pointer.
   *
   *  The pointer must satisfy the contract of `occtl_graph_t*` (non-NULL,
   *  owned by us). This is the marshal-in path for a graph that was allocated
   *  by another binding layer or by a parent addon that already called
   *  `occtl_graph_create`.
   *
   *  @throws UnsupportedError always in the default build — the addon
   *    does not ship a wrap-by-pointer factory because every graph created by
   *    the Node binding is already tracked through `new Graph()`. A custom
   *    addon that links OCCT-Light statically and vends pre-created graphs to
   *    Node must add an `occtl_graph_wrap` N-API binding and patch this
   *    method to call it.
   */
  public static fromPointerUnsafe(ptr: bigint): Graph {
    if (ptr <= 0n) {
      throw new InvalidHandleError({
        status: Status.InvalidHandle,
        message: 'Graph.fromPointerUnsafe received a non-positive pointer',
        source: 0n as never, extended: 0,
      });
    }
    // The addon must expose a wrap-by-pointer factory for round-tripping. If it
    // does not, surface Unsupported so callers can branch on configuration.
    throw new UnsupportedError({
      status: Status.Unsupported,
      message: 'Graph.fromPointerUnsafe is not implemented in this build',
      source: 0n as never, extended: 0,
    });
  }

  /** Iterator over face NodeIds. */
  public *faces(): Generator<NodeId> {
    const it = loadAddon().graphFaceIterCreate(this._native());
    try { yield* iterateNodes(it); } finally { it.close(); }
  }

  /** Iterator over edge NodeIds. */
  public *edges(): Generator<NodeId> {
    const it = loadAddon().graphEdgeIterCreate(this._native());
    try { yield* iterateNodes(it); } finally { it.close(); }
  }

  /** Iterator over vertex NodeIds. */
  public *vertices(): Generator<NodeId> {
    const it = loadAddon().graphVertexIterCreate(this._native());
    try { yield* iterateNodes(it); } finally { it.close(); }
  }

  /** Iterator over solid NodeIds. */
  public *solids(): Generator<NodeId> {
    const it = loadAddon().graphSolidIterCreate(this._native());
    try { yield* iterateNodes(it); } finally { it.close(); }
  }

  /** Number of active compound nodes. */
  public get nbCompounds(): number {
    return Number(loadAddon().call('graph_compound_count', [this._native()]));
  }

  /** Insert a vertex at the given point and return its NodeId. */
  public makeVertex(p: { x: number; y: number; z: number }): NodeId {
    const addon = loadAddon();
    const result = addon.call('topo_make_vertex', [this._native(), p.x, p.y, p.z]) as bigint;
    return nodeId(result);
  }

  /** Build an axis-aligned box solid in the graph. */
  public makeBox(info: BoxInfo): NodeId {
    Graph._requireExport('occtl_prim_make_box', 'prim');
    const addon = loadAddon();
    const result = addon.call('prim_make_box', [this._native(), info]) as bigint;
    return nodeId(result);
  }

  /** Build a sphere solid in the graph. */
  public makeSphere(info: SphereInfo | number): NodeId {
    Graph._requireExport('occtl_prim_make_sphere', 'prim');
    const options = typeof info === 'number' ? { radius: info } : info;
    return nodeId(loadAddon().call('prim_make_sphere', [this._native(), options]) as bigint);
  }

  /** Build a cylinder solid in the graph. */
  public makeCylinder(info: CylinderInfo): NodeId {
    Graph._requireExport('occtl_prim_make_cylinder', 'prim');
    return nodeId(loadAddon().call('prim_make_cylinder', [this._native(), info]) as bigint);
  }

  /** Build a cone or truncated-cone solid in the graph. */
  public makeCone(info: ConeInfo): NodeId {
    Graph._requireExport('occtl_prim_make_cone', 'prim');
    return nodeId(loadAddon().call('prim_make_cone', [this._native(), info]) as bigint);
  }

  /** Build a torus solid in the graph. */
  public makeTorus(info: TorusInfo): NodeId {
    Graph._requireExport('occtl_prim_make_torus', 'prim');
    return nodeId(loadAddon().call('prim_make_torus', [this._native(), info]) as bigint);
  }

  /** Build a right-angular wedge solid in the graph. */
  public makeWedge(info: WedgeInfo): NodeId {
    Graph._requireExport('occtl_prim_make_wedge', 'prim');
    return nodeId(loadAddon().call('prim_make_wedge', [this._native(), info]) as bigint);
  }

  private static _toBoolOptions(opts: BoolOptions | undefined): Record<string, unknown> {
    const fuzzyValue = opts?.fuzzyValue;
    const runParallel = opts?.runParallel;
    const simplifyResult = opts?.simplifyResult;
    const simplifyAngularTolerance = opts?.simplifyAngularTolerance;
    const simplifyLinearTolerance = opts?.simplifyLinearTolerance;
    const buildHistory = opts?.buildHistory;

    const out: Record<string, unknown> = {};
    if (fuzzyValue !== undefined) out.fuzzy_value = fuzzyValue;
    if (runParallel !== undefined) out.run_parallel = runParallel ? 1 : 0;
    if (simplifyResult !== undefined) out.simplify_result = simplifyResult ? 1 : 0;
    if (simplifyAngularTolerance !== undefined) out.simplify_angular_tolerance = simplifyAngularTolerance;
    if (simplifyLinearTolerance !== undefined) out.simplify_linear_tolerance = simplifyLinearTolerance;
    if (buildHistory !== undefined) out.build_history = buildHistory ? 1 : 0;
    return out;
  }

  private _runBool(
    op: 'bool_fuse' | 'bool_cut' | 'bool_common' | 'bool_section' | 'bool_split',
    objects: readonly NodeId[],
    tools: readonly NodeId[],
    opts?: BoolOptions,
  ): NodeId {
    const symbol = `occtl_${op}`;
    Graph._requireExport(symbol, 'bool');
    return nodeId(loadAddon().call(op, [
      this._native(),
      [...objects],
      [...tools],
      Graph._toBoolOptions(opts),
    ]) as bigint);
  }

  /** Boolean Fuse (union) of object/tool groups. */
  public fuse(objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
    return this._runBool('bool_fuse', objects, tools, opts);
  }

  /** Boolean Cut (objects minus tools). */
  public cut(objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
    return this._runBool('bool_cut', objects, tools, opts);
  }

  /** Boolean Common (intersection). */
  public common(objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
    return this._runBool('bool_common', objects, tools, opts);
  }

  /** Boolean Section. */
  public section(objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
    return this._runBool('bool_section', objects, tools, opts);
  }

  /** Boolean Split. */
  public split(objects: readonly NodeId[], tools: readonly NodeId[], opts?: BoolOptions): NodeId {
    return this._runBool('bool_split', objects, tools, opts);
  }

  /** Run topology validation and return reported issues. */
  public checkIssues(): CheckIssue[] {
    const sizing = loadAddon().call('topo_check', [this._native(), 0]) as { out_count: number };
    const count = Number(sizing.out_count);
    if (count === 0) return [];

    const result = loadAddon().call('topo_check', [this._native(), count]) as {
      out_issues?:
        | { node_id: bigint; context_node_id: bigint; status_bit: number; severity: number }
        | Array<{ node_id: bigint; context_node_id: bigint; status_bit: number; severity: number }>;
      out_count: number;
    };
    if (!result.out_issues) return [];
    const issues = Array.isArray(result.out_issues) ? result.out_issues : [result.out_issues];
    return issues.slice(0, count).map((issue) => ({
      nodeId: nodeId(issue.node_id),
      contextNodeId: nodeId(issue.context_node_id),
      statusBit: Number(issue.status_bit),
      severity: Number(issue.severity),
    }));
  }

  /** True when topology validation reports no issues. */
  public isValid(): boolean {
    return this.checkIssues().length === 0;
  }

  /** Write the topology rooted at `root` to a BRep file. */
  public writeBrep(root: NodeId, path: string, options: BrepWriteOptions = {}): void {
    loadAddon().call('io_brep_write', [this._native(), root, path, {
      write_triangulation: options.writeTriangulation === false ? 0 : 1,
    }]);
  }

  /** Write the topology rooted at `root` to an STL payload in memory. */
  public writeStlMemory(root: NodeId, options: StlWriteOptions = {}): Uint8Array {
    return loadAddon().call('io_stl_write_memory', [this._native(), root, {
      ascii_mode: options.asciiMode === true ? 1 : 0,
    }]) as Uint8Array;
  }

  /** Write the topology rooted at `root` to a VRML payload in memory. */
  public writeVrmlMemory(root: NodeId, options: VrmlWriteOptions = {}): Uint8Array {
    return loadAddon().call('io_vrml_write_memory', [this._native(), root, {
      writer_version: options.writerVersion ?? 2,
      representation: options.representation ?? 1,
    }]) as Uint8Array;
  }

  /** Write the topology rooted at `root` to a data-exchange memory payload. */
  public writeDeMemory(root: NodeId, formatId: string): Uint8Array {
    return loadAddon().call('de_write_memory', [this._native(), root, formatId]) as Uint8Array;
  }

  /** Return the persistent UID for a node. */
  public uidOf(node: NodeId): Uid {
    return loadAddon().call('graph_uid_from_node_id', [this._native(), node]) as Uid;
  }

  /** Return the persistent UID for a representation. */
  public repUidOf(rep: RepId): RepUid {
    return repUid(loadAddon().call('graph_rep_uid_from_rep_id', [this._native(), rep]) as bigint);
  }

  /** Resolve a persistent representation UID to the current RepId. */
  public repIdOf(rep: RepUid): RepId {
    return repId(loadAddon().call('graph_rep_id_from_rep_uid', [this._native(), rep]) as bigint);
  }

  /** Resolve a persistent UID to the current node id. */
  public nodeIdOf(uid: Uid): NodeId {
    return nodeId(loadAddon().call('graph_node_id_from_uid', [this._native(), uid]) as bigint);
  }

  /** Return the topology kind of a live node. */
  public nodeKind(node: NodeId): NodeKind {
    const raw = Number(loadAddon().call('graph_node_kind', [this._native(), node]));
    switch (raw) {
      case 1: return 'solid';
      case 2: return 'shell';
      case 3: return 'face';
      case 4: return 'wire';
      case 5: return 'edge';
      case 6: return 'vertex';
      case 7: return 'compound';
      case 8: return 'compsolid';
      case 9: return 'coedge';
      case 10: return 'product';
      case 11: return 'occurrence';
      default: return 'unknown';
    }
  }

  /** Return a transformed copy of the shape rooted at `root`. */
  public transformed(root: NodeId, transform: { m: number[] }): GraphRootResult {
    const result = loadAddon().call('topo_transformed', [this._native(), root, transform]) as {
      out_graph: NativeGraph;
      out_root: bigint;
    };
    return { graph: Graph._adopt(result.out_graph), root: nodeId(result.out_root) };
  }

  /** Return a translated copy of the shape rooted at `root`. */
  public translated(root: NodeId, delta: { x: number; y: number; z: number }): GraphRootResult {
    const transform = loadAddon().call('transform_translation', [delta]) as { m: number[] };
    return this.transformed(root, transform);
  }

  /** Return a rotated copy of the shape rooted at `root`. */
  public rotated(
    root: NodeId,
    axis: { location: { x: number; y: number; z: number }; direction: { x: number; y: number; z: number } },
    angle: number,
  ): GraphRootResult {
    const transform = loadAddon().call('transform_rotation', [axis, angle]) as { m: number[] };
    return this.transformed(root, transform);
  }

  /** Return a uniformly scaled copy of the shape rooted at `root`. */
  public scaled(root: NodeId, center: { x: number; y: number; z: number }, factor: number): GraphRootResult {
    const transform = loadAddon().call('transform_scale', [center, factor]) as { m: number[] };
    return this.transformed(root, transform);
  }

  /** Fillet selected edges of the shape rooted at `root`. */
  public filletEdges(root: NodeId, edges: readonly NodeId[], radius: number): GraphRootResult {
    const result = loadAddon().call('topo_blend_edges', [this._native(), {
      root,
      edges: [...edges],
      edge_count: edges.length,
      radius,
      chamfer_mode: 0,
    }]) as { out_graph: NativeGraph; out_root: bigint };
    return { graph: Graph._adopt(result.out_graph), root: nodeId(result.out_root) };
  }

  /** Chamfer selected edges of the shape rooted at `root`. */
  public chamferEdges(
    root: NodeId,
    edges: readonly NodeId[],
    distance1: number,
    distance2: number = distance1,
  ): GraphRootResult {
    const result = loadAddon().call('topo_blend_edges', [this._native(), {
      root,
      edges: [...edges],
      edge_count: edges.length,
      radius: 0,
      chamfer_mode: 1,
      chamfer_dist1: distance1,
      chamfer_dist2: distance2,
    }]) as { out_graph: NativeGraph; out_root: bigint };
    return { graph: Graph._adopt(result.out_graph), root: nodeId(result.out_root) };
  }
  /** UIDs modified from inputUid in this graph's recorded history. */
  public historyModified(inputUid: Uid): bigint[] {
    try {
      return loadAddon().call('graph_history_modified', [this._native(), inputUid]) as bigint[];
    } catch (error) {
      if ((error instanceof NotFoundError || (error as Error).name === 'NotFoundError')
        && (error as Error).message.includes('input_uid')) return [];
      throw error;
    }
  }

  /** UIDs generated from inputUid in this graph's recorded history. */
  public historyGenerated(inputUid: Uid): bigint[] {
    try {
      return loadAddon().call('graph_history_generated', [this._native(), inputUid]) as bigint[];
    } catch (error) {
      if ((error instanceof NotFoundError || (error as Error).name === 'NotFoundError')
        && (error as Error).message.includes('input_uid')) return [];
      throw error;
    }
  }

  /** All UIDs deleted in this graph's recorded history. */
  public historyDeletedAll(): bigint[] {
    return loadAddon().call('graph_history_deleted_all', [this._native()]) as bigint[];
  }

}

function* iterateNodes(it: NativeNodeIter): Generator<NodeId> {
  while (true) {
    const next = it.next();
    if (next === null) return;
    yield nodeId(next);
  }
}
