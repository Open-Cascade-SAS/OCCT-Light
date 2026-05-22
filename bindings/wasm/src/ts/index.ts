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

import { Occtl as OcctlLoader, fromRawModule } from "./abi.js";
import type { LoadedOcctl } from "./abi.js";
import { CoreModule } from "./core.js";
import { GeomModule } from "./geom.js";
import { TopoGraphFacade } from "./_topo_facade.js";
import { PrimModule } from "./prim.js";
import { TextModule } from "./text.js";
import { CurvesModule } from "./curves.js";
import { Curves2dModule } from "./curves2d.js";
import { SurfacesModule } from "./surfaces.js";
import { BoolModule } from "./bool_.js";
import { Graph } from "./topo.js";

// Re-export everything userland should see.
export * from "./errors.js";
export * from "./ids.js";
export * from "./views.js";
export { Disposable } from "./handles.js";
export { Graph } from "./topo.js";
export type { BoolOptions } from "./bool_.js";
export type { Point3, Vec3, Dir3, Point2, Vec2, Dir2, Axis1Placement, Axis2Placement } from "./geom.js";
export type { BoxInfo, SphereInfo, CylinderInfo, ConeInfo, TorusInfo, WedgeInfo } from "./prim.js";
export type { LoadedOcctl } from "./abi.js";
export type { BrepWriteOptions, CheckIssue, GraphRootResult } from "./topo.js";

/**
 * A fully-loaded OCCT-Light WASM facade. Returned by `Occtl.load()`.
 *
 * Each property is a per-module idiomatic helper. Raw bindings live
 * under `.core.raw`, `.geom.raw`, etc.
 */
export interface Occtl {
  readonly raw: LoadedOcctl["raw"];
  readonly ABI_VERSION: number;
  runtimeAbiVersion(): number;

  /** Construct a new topology graph (Disposable). */
  Graph: { new (): Graph; create(): Graph; readBrep(path: string): import("./topo.js").GraphRootResult };

  readonly core: CoreModule;
  readonly geom: GeomModule;
  readonly topo: TopoGraphFacade;
  readonly prim: PrimModule;
  readonly text: TextModule;
  readonly curves: CurvesModule;
  readonly curves2d: Curves2dModule;
  readonly surfaces: SurfacesModule;
  readonly bool_: BoolModule;
}

function buildFacade(loaded: LoadedOcctl): Occtl {
  const core = new CoreModule(loaded);
  const geom = new GeomModule(loaded);
  const topo = new TopoGraphFacade(loaded);
  const prim = new PrimModule(loaded);
  const text = new TextModule(loaded);
  const curves = new CurvesModule(loaded);
  const curves2d = new Curves2dModule(loaded);
  const surfaces = new SurfacesModule(loaded);
  const bool_ = new BoolModule(loaded);

  // Construct the user-visible `Graph` class with a bound `occtl` reference.
  // Users say `new occtl.Graph()` and get a graph tied to this load().
  function GraphCtor(): Graph {
    return Graph.create(loaded);
  }
  GraphCtor.create = (): Graph => Graph.create(loaded);
  GraphCtor.readBrep = (path: string): import("./topo.js").GraphRootResult => Graph.readBrep(loaded, path);
  // make `new occtl.Graph()` work
  (GraphCtor as unknown as { prototype: object }).prototype = Graph.prototype;

  return {
    raw: loaded.raw,
    ABI_VERSION: loaded.ABI_VERSION,
    runtimeAbiVersion: () => loaded.runtimeAbiVersion(),
    Graph: GraphCtor as unknown as Occtl["Graph"],
    core, geom, topo, prim, text, curves, curves2d, surfaces, bool_,
  };
}

/**
 * Public loader. Async — invokes the Emscripten factory.
 *
 *     const occtl = await Occtl.load();
 *     using g = new occtl.Graph();
 */
export const Occtl = {
  /** Load the WASM module and return the facade. Cached per-process. */
  async load(opts?: Parameters<typeof OcctlLoader.load>[0]): Promise<Occtl> {
    const loaded = await OcctlLoader.load(opts ?? {});
    return buildFacade(loaded);
  },

  /** Build a facade from an already-loaded raw module. Test seam. */
  fromRawModule(raw: LoadedOcctl["raw"]): Occtl {
    return buildFacade(fromRawModule(raw));
  },

  /** Reset the cached module — primarily for tests. */
  reset(): void {
    OcctlLoader.reset();
  },
};
