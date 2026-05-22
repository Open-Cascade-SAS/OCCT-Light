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
import { FUNCTION_NAMES_PRIM, bindRawPrim } from "./generated/prim.js";
import { STRUCT_LAYOUTS, mallocStruct, setField } from "./generated/_layouts.js";

export { FUNCTION_NAMES_PRIM };

/** Options for makeBox. The binding sets struct_version itself. */
export interface BoxInfo {
  dx: number;
  dy: number;
  dz: number;
  /** Optional local frame. Defaults to XOY. */
  placement?: {
    location?: { x: number; y: number; z: number };
    x_dir?: { x: number; y: number; z: number };
    x_dir_ref?: { x: number; y: number; z: number };
    normal?: { x: number; y: number; z: number };
    xDirection?: { x: number; y: number; z: number };
  };
}

/** Options for makeSphere. The binding sets struct_version itself. */
export interface SphereInfo {
  radius: number;
  angle1?: number;
  angle2?: number;
  angle?: number;
}

/** Options for makeCylinder. The binding sets struct_version itself. */
export interface CylinderInfo {
  radius: number;
  height: number;
  angle?: number;
}

/** Options for makeCone. The binding sets struct_version itself. */
export interface ConeInfo {
  r1: number;
  r2: number;
  height: number;
  angle?: number;
}

/** Options for makeTorus. The binding sets struct_version itself. */
export interface TorusInfo {
  r1: number;
  r2: number;
  angle1?: number;
  angle2?: number;
  angle?: number;
}

/** Options for makeWedge. The binding sets struct_version itself. */
export interface WedgeInfo {
  dx: number;
  dy: number;
  dz: number;
  ltx: number;
}

export class PrimModule {
  private readonly _raw: ReturnType<typeof bindRawPrim>;
  private readonly _occtl: LoadedOcctl;
  constructor(occtl: LoadedOcctl) {
    this._occtl = occtl;
    this._raw = bindRawPrim(occtl.raw);
  }
  get raw(): ReturnType<typeof bindRawPrim> { return this._raw; }

  /**
   * Build a box into `graph` and return its solid NodeId.
   *
   * Struct layout comes from the auto-generated STRUCT_LAYOUTS table
   * (computed with wasm32 alignment rules from the ABI catalogue).
   * The runtime `occtl_prim_box_info_init` initialiser sets defaults;
   * we overwrite dx/dy/dz using the generated offsets.
   */
  makeBox(graph: Graph, info: BoxInfo): NodeId {
    return this.makeSolid(graph, "occtl_prim_box_info_t", "occtl_prim_box_info_init", "occtl_prim_make_box", {
      dx: info.dx,
      dy: info.dy,
      dz: info.dz,
    }, info.placement);
  }

  /** Build a sphere into `graph` and return its solid NodeId. */
  makeSphere(graph: Graph, info: SphereInfo | number): NodeId {
    const options = typeof info === "number" ? { radius: info } : info;
    return this.makeSolid(graph, "occtl_prim_sphere_info_t", "occtl_prim_sphere_info_init", "occtl_prim_make_sphere", { ...options });
  }

  /** Build a cylinder into `graph` and return its solid NodeId. */
  makeCylinder(graph: Graph, info: CylinderInfo): NodeId {
    return this.makeSolid(graph, "occtl_prim_cylinder_info_t", "occtl_prim_cylinder_info_init", "occtl_prim_make_cylinder", { ...info });
  }

  /** Build a cone or truncated cone into `graph` and return its solid NodeId. */
  makeCone(graph: Graph, info: ConeInfo): NodeId {
    return this.makeSolid(graph, "occtl_prim_cone_info_t", "occtl_prim_cone_info_init", "occtl_prim_make_cone", { ...info });
  }

  /** Build a torus into `graph` and return its solid NodeId. */
  makeTorus(graph: Graph, info: TorusInfo): NodeId {
    return this.makeSolid(graph, "occtl_prim_torus_info_t", "occtl_prim_torus_info_init", "occtl_prim_make_torus", { ...info });
  }

  /** Build a wedge into `graph` and return its solid NodeId. */
  makeWedge(graph: Graph, info: WedgeInfo): NodeId {
    return this.makeSolid(graph, "occtl_prim_wedge_info_t", "occtl_prim_wedge_info_init", "occtl_prim_make_wedge", { ...info });
  }

  private makeSolid(
    graph: Graph,
    infoStruct: string,
    initSymbol: string,
    makeSymbol: string,
    fields: Record<string, number | undefined>,
    placement?: BoxInfo["placement"],
  ): NodeId {
    const mod = this._occtl.raw;
    const infoPtr = mallocStruct(infoStruct, mod);
    const idPtr = mod._malloc(8);
    try {
      // Zero the block then run the runtime initialiser.
      const layout = STRUCT_LAYOUTS[infoStruct];
      for (let i = 0; i < layout.size; i += 4) mod.setValue(infoPtr + i, 0, "i32");
      const initFn = (mod as unknown as Record<string, (p: number) => void>)[initSymbol];
      if (initFn) {
        initFn(infoPtr);
      } else {
        mod.setValue(infoPtr, 1, "i32");  // struct_version = 1
      }
      for (const [name, value] of Object.entries(fields)) {
        if (value !== undefined) setField(infoPtr, infoStruct, name, value, mod);
      }
      if (placement) this.writeAxis2Placement(infoPtr, placement);

      const fn = (mod as unknown as Record<string, (g: number, info: number, out: number) => number>)[makeSymbol];
      if (!fn) {
        throw new UnsupportedError(
          `Feature 'prim' is not available in this OCCT-Light build (missing ${makeSymbol})`,
          0n as Uid,
          0,
        );
      }
      const status = fn(graph.pointer, infoPtr, idPtr);
      check(mod, status);
      const lo = BigInt.asUintN(32, BigInt(mod.getValue(idPtr + 0, "i32") >>> 0));
      const hi = BigInt.asUintN(32, BigInt(mod.getValue(idPtr + 4, "i32") >>> 0));
      return NodeId.fromBits((hi << 32n) | lo);
    } finally {
      mod._free(infoPtr);
      mod._free(idPtr);
    }
  }

  private writeAxis2Placement(ptr: number, placement: NonNullable<BoxInfo["placement"]>): void {
    const mod = this._occtl.raw;
    const layout = STRUCT_LAYOUTS["occtl_prim_box_info_t"].fields.placement;
    const axis = STRUCT_LAYOUTS[layout.type];
    const base = ptr + layout.offset;
    const writeVec3 = (field: "location" | "x_dir" | "x_dir_ref", value?: { x: number; y: number; z: number }) => {
      if (!value) return;
      const offset = base + axis.fields[field].offset;
      mod.setValue(offset + 0, value.x, "double");
      mod.setValue(offset + 8, value.y, "double");
      mod.setValue(offset + 16, value.z, "double");
    };
    writeVec3("location", placement.location);
    writeVec3("x_dir", placement.x_dir ?? placement.normal);
    writeVec3("x_dir_ref", placement.x_dir_ref ?? placement.xDirection);
  }
}
