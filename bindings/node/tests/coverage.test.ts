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

import { readFileSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it, expect } from 'vitest';

import { EXPORTED_FUNCTIONS } from '../src/ts/generated/_raw.gen.js';

const __dirname = dirname(fileURLToPath(import.meta.url));

interface AbiDoc {
  functions: { name: string; header?: string }[];
}

const abiPath = resolve(__dirname, '..', '..', '..', 'build', 'abi.json');
const abi = JSON.parse(readFileSync(abiPath, 'utf-8')) as AbiDoc;

function candidateFeaturesPaths(): string[] {
  const explicit = process.env.OCCTL_FEATURES_PATH;
  if (explicit && explicit.trim().length > 0) return [explicit];
  const libraryPath = process.env.OCCTL_LIBRARY_PATH;
  if (!libraryPath || libraryPath.trim().length === 0) return [];
  const parent = resolve(libraryPath, '..');
  return [
    resolve(libraryPath, 'OCCTLFeatures.json'),
    resolve(parent, 'OCCTLFeatures.json'),
  ];
}

function enabledHeadersFromManifest(): Set<string> | null {
  const featureToHeaders: Record<string, string[]> = {
    core: ['occtl.h', 'occtl_core.h'],
    geom: ['occtl_geom.h', 'occtl_curves.h', 'occtl_curves2d.h', 'occtl_curves_common.h', 'occtl_surfaces.h'],
    topo: ['occtl_topo.h', 'occtl_topo_types.h', 'occtl_topo_algo.h', 'occtl_topo_build.h', 'occtl_topo_relation.h'],
    prim: ['occtl_prim.h', 'occtl_prim_solid.h', 'occtl_prim_sketch.h', 'occtl_prim_sweep.h', 'occtl_prim_feature.h'],
    text: ['occtl_text.h'],
    bool: ['occtl_bool.h'],
    mesh: ['occtl_mesh.h'],
    heal: ['occtl_heal.h'],
    io_brep: ['occtl_io_brep.h'],
    io_step: ['occtl_io_step.h'],
    io_iges: ['occtl_io_iges.h'],
    io_stl: ['occtl_io_stl.h'],
    io_obj: ['occtl_io_obj.h'],
    io_gltf: ['occtl_io_gltf.h'],
    io_vrml: ['occtl_io_vrml.h'],
    io_ply: ['occtl_io_ply.h'],
    de: ['occtl_de.h'],
    viz: ['occtl_viz.h'],
  };

  for (const path of candidateFeaturesPaths()) {
    try {
      const payload = JSON.parse(readFileSync(path, 'utf-8')) as { binding_features?: string[] };
      if (!Array.isArray(payload.binding_features)) continue;
      const headers = new Set<string>(['occtl.h', 'occtl_core.h']);
      for (const feature of payload.binding_features) {
        for (const header of (featureToHeaders[feature] ?? [])) headers.add(header);
      }
      return headers;
    } catch {
      // try next manifest candidate
    }
  }
  return null;
}

describe('coverage', () => {
  it('every OCCTL_API function has a binding entry', () => {
    const exported = new Set(EXPORTED_FUNCTIONS);
    const enabledHeaders = enabledHeadersFromManifest();
    const missing: string[] = [];
    for (const f of abi.functions) {
      if (enabledHeaders && !enabledHeaders.has(f.header ?? '')) continue;
      const key = f.name.replace(/^occtl_/, '');
      if (!exported.has(key)) missing.push(f.name);
    }
    if (missing.length > 0) {
      // eslint-disable-next-line no-console
      console.error(`Missing binding entries (${missing.length}):\n  ${missing.slice(0, 20).join('\n  ')}${missing.length > 20 ? `\n  ... and ${missing.length - 20} more` : ''}`);
    }
    expect(missing).toEqual([]);
  });

  it('exports table is non-empty', () => {
    expect(EXPORTED_FUNCTIONS.length).toBeGreaterThan(0);
  });
});
