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
import { FUNCTION_NAMES_GEOM, bindRawGeom } from "./generated/geom.js";

export { FUNCTION_NAMES_GEOM };

/** 3D point. */
export interface Point3 { x: number; y: number; z: number; }
/** 3D direction (assumed unit). */
export interface Dir3 { x: number; y: number; z: number; }
/** 3D vector. */
export interface Vec3 { x: number; y: number; z: number; }
/** 2D point (parametric space). */
export interface Point2 { x: number; y: number; }
/** 2D direction. */
export interface Dir2 { x: number; y: number; }
/** 2D vector. */
export interface Vec2 { x: number; y: number; }

/** Axis-1 placement: a point + a direction. */
export interface Axis1Placement { location: Point3; direction: Dir3; }
/** Axis-2 placement: full right-handed local frame. */
export interface Axis2Placement { location: Point3; normal: Dir3; xDirection: Dir3; }

export class GeomModule {
  private readonly _raw: ReturnType<typeof bindRawGeom>;
  private readonly _occtl: LoadedOcctl;
  constructor(occtl: LoadedOcctl) {
    this._occtl = occtl;
    this._raw = bindRawGeom(occtl.raw);
  }
  get raw(): ReturnType<typeof bindRawGeom> { return this._raw; }

  /**
   * Write a Point3 into a freshly-allocated WASM-side struct. Returns the
   * pointer; the caller is responsible for `_free`-ing it. Layout:
   * three contiguous f64s starting at offset 0.
   */
  writePoint3(p: Point3): number {
    const mod = this._occtl.raw;
    const ptr = mod._malloc(24);
    mod.setValue(ptr + 0,  p.x, "double");
    mod.setValue(ptr + 8,  p.y, "double");
    mod.setValue(ptr + 16, p.z, "double");
    return ptr;
  }
  readPoint3(ptr: number): Point3 {
    const mod = this._occtl.raw;
    return {
      x: mod.getValue(ptr + 0,  "double"),
      y: mod.getValue(ptr + 8,  "double"),
      z: mod.getValue(ptr + 16, "double"),
    };
  }
}
