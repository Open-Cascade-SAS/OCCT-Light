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
import { AbiMismatchError, Status, OcctLError } from './errors.js';
import { uid } from './ids.js';
import * as fs from 'node:fs';
import * as path from 'node:path';

/** ABI version this binding was generated against. The generator overrides this
 *  constant in dist/abi.js; do not edit by hand. */
export const ABI_VERSION = 1;

/** Binding package version. Tracks package.json. */
export const BINDING_VERSION = '0.1.0';

/** Option struct version constants — keep in lockstep with build/abi.json constants.
 *  The generator extends this map. */
export const STRUCT_VERSIONS: Readonly<Record<string, number>> = Object.freeze({
  occtl_runtime_init_info:          1,
  occtl_curve_bspline_create_info:  1,
  occtl_curve_bezier_create_info:   1,
  occtl_curve_trimmed_create_info:  1,
  occtl_curve_offset_create_info:   1,
  occtl_prim_box_info:              1,
});

const FEATURE_TO_MODULES: Readonly<Record<string, readonly string[]>> = Object.freeze({
  core: ["core"],
  geom: ["geom", "curves", "curves2d", "surfaces"],
  topo: ["topo", "topo_algo"],
  prim: ["prim"],
  text: ["text"],
  bool: ["bool_"],
  mesh: ["mesh"],
  heal: ["heal"],
  de: ["de"],
  io_brep: ["io_brep"],
  io_step: ["io_step"],
  io_iges: ["io_iges"],
  io_stl: ["io_stl"],
  io_obj: ["io_obj"],
  io_gltf: ["io_gltf"],
  io_vrml: ["io_vrml"],
  io_ply: ["io_ply"],
  viz: ["viz"],
});

function candidateFeatureManifestPaths(): readonly string[] {
  const explicit = process.env.OCCTL_FEATURES_PATH;
  if (explicit && explicit.trim().length > 0) {
    return [explicit];
  }
  const libraryPath = process.env.OCCTL_LIBRARY_PATH;
  if (!libraryPath || libraryPath.trim().length === 0) {
    return [];
  }
  const parent = path.dirname(libraryPath);
  return [
    path.join(libraryPath, "OCCTLFeatures.json"),
    path.join(parent && parent.length > 0 ? parent : libraryPath, "OCCTLFeatures.json"),
  ];
}

function runtimeAvailableModules(): ReadonlySet<string> {
  for (const manifestPath of candidateFeatureManifestPaths()) {
    try {
      const json = JSON.parse(fs.readFileSync(manifestPath, "utf-8")) as { binding_features?: unknown };
      if (!Array.isArray(json.binding_features)) {
        continue;
      }
      const modules = new Set<string>();
      for (const feature of json.binding_features) {
        const key = String(feature).trim();
        if (!key) continue;
        const mapped = FEATURE_TO_MODULES[key];
        if (mapped && mapped.length > 0) {
          for (const moduleName of mapped) modules.add(moduleName);
        } else {
          modules.add(key);
        }
      }
      if (modules.size > 0) {
        return modules;
      }
    } catch {
      // Keep probing candidates.
    }
  }

  const addon = loadAddon();
  const exported = addon.EXPORTED_FUNCTIONS ?? [];
  const inferred = new Set<string>(["core"]);
  const hasPrefix = (prefix: string): boolean => exported.some((fn) => String(fn).startsWith(prefix));
  if (hasPrefix("occtl_curve") || hasPrefix("occtl_geom_") || hasPrefix("occtl_surface_")) {
    inferred.add("geom");
    inferred.add("curves");
    inferred.add("curves2d");
    inferred.add("surfaces");
  }
  if (hasPrefix("occtl_graph_") || hasPrefix("occtl_topo_")) {
    inferred.add("topo");
    inferred.add("topo_algo");
  }
  if (hasPrefix("occtl_prim_")) inferred.add("prim");
  if (hasPrefix("occtl_text_")) inferred.add("text");
  if (hasPrefix("occtl_bool_")) inferred.add("bool_");
  if (hasPrefix("occtl_mesh_")) inferred.add("mesh");
  if (hasPrefix("occtl_heal_")) inferred.add("heal");
  if (hasPrefix("occtl_de_")) inferred.add("de");
  if (hasPrefix("occtl_io_brep_")) inferred.add("io_brep");
  if (hasPrefix("occtl_io_step_")) inferred.add("io_step");
  if (hasPrefix("occtl_io_iges_")) inferred.add("io_iges");
  if (hasPrefix("occtl_io_stl_")) inferred.add("io_stl");
  if (hasPrefix("occtl_io_obj_")) inferred.add("io_obj");
  if (hasPrefix("occtl_io_gltf_")) inferred.add("io_gltf");
  if (hasPrefix("occtl_io_vrml_")) inferred.add("io_vrml");
  if (hasPrefix("occtl_io_ply_")) inferred.add("io_ply");
  if (hasPrefix("occtl_viz_")) inferred.add("viz");
  return inferred;
}

/** Modules available in the installed OCCT-Light feature-set. */
export const AVAILABLE_MODULES: ReadonlySet<string> = runtimeAvailableModules();

let _initialised = false;

/** Returns the runtime ABI version reported by the loaded shared library. */
export function runtimeAbiVersion(): number {
  return loadAddon().runtimeAbiVersion();
}

/** Initialise the OCCT-Light runtime. Idempotent. Checks ABI version on first
 *  call and throws AbiMismatchError on mismatch. */
export function init(): void {
  if (_initialised) return;
  const addon = loadAddon();
  const runtime = addon.runtimeAbiVersion();
  if (runtime !== ABI_VERSION) {
    throw new AbiMismatchError({
      status:   Status.VersionMismatch,
      message:  `OCCT-Light ABI mismatch: binding compiled against ${ABI_VERSION}, runtime reports ${runtime}`,
      source:   uid(0n),
      extended: 0,
    });
  }
  try {
    addon.runtimeInit();
  } catch (e) {
    // Re-throw as a typed OcctLError if the addon emits a plain Error.
    if (e instanceof OcctLError) throw e;
    throw new OcctLError({
      status:   Status.Internal,
      message:  e instanceof Error ? e.message : String(e),
      source:   uid(0n),
      extended: 0,
    });
  }
  _initialised = true;
}

/** Force the init flag back to false. Test-only hook. */
export function __resetInitForTesting(): void { _initialised = false; }
