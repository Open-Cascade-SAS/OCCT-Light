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

import { describe, it, expect } from 'vitest';
import * as occtl from '../src/ts/index.js';
import * as core from '../src/ts/generated/core.js';
import * as geom from '../src/ts/generated/geom.js';
import * as topo from '../src/ts/generated/topo.js';
import * as prim from '../src/ts/generated/prim.js';

describe('backfill — core', () => {
  it('runtime_version yields a SemVer triple', () => {
    occtl.init();
    const v = core.runtimeVersion() as { out_major: number; out_minor: number; out_patch: number };
    expect(v).toBeDefined();
    expect(typeof v.out_major).toBe('number');
    expect(typeof v.out_minor).toBe('number');
    expect(typeof v.out_patch).toBe('number');
  });

  it('status_to_string returns a non-empty string', () => {
    const s = core.statusToString(0) as string;
    expect(typeof s).toBe('string');
    expect(s.length).toBeGreaterThan(0);
  });
});

describe('backfill — geom', () => {
  it('point3_translate moves a point by a vector', () => {
    const p = geom.point3Translate({ x: 1, y: 2, z: 3 }, { x: 4, y: 5, z: 6 }) as { x: number; y: number; z: number };
    expect(p.x).toBeCloseTo(5);
    expect(p.y).toBeCloseTo(7);
    expect(p.z).toBeCloseTo(9);
  });

  it('point3_distance returns Euclidean distance', () => {
    const d = geom.point3Distance({ x: 0, y: 0, z: 0 }, { x: 3, y: 4, z: 0 });
    expect(d).toBeCloseTo(5);
  });

  it('curve_create_line returns a bigint rep id', () => {
    occtl.init();
    const g = new occtl.Graph();
    try {
      const result = geom.curveCreateLine(g._native(), {
        location:  { x: 0, y: 0, z: 0 },
        direction: { x: 1, y: 0, z: 0 },
      });
      expect(result).toBeTypeOf('bigint');
    } finally {
      g.close();
    }
  });
});

describe('backfill — topo', () => {
  it('graph_create + make_vertex + vertex_count', () => {
    occtl.init();
    const g = new occtl.Graph();
    try {
      const v1 = g.makeVertex({ x: 0, y: 0, z: 0 });
      const v2 = g.makeVertex({ x: 1, y: 0, z: 0 });
      expect(v1).toBeDefined();
      expect(v2).toBeDefined();

      const nbVerts = topo.graphNbVertices(g._native()) as number;
      expect(nbVerts).toBeGreaterThanOrEqual(2);
    } finally {
      g.close();
    }
  });

  it('graph_edge_count + graph_face_count report counts after make_box', () => {
    occtl.init();
    const g = new occtl.Graph();
    try {
      g.makeBox({ dx: 1, dy: 1, dz: 1 });
      const nbEdges = topo.graphEdgeCount(g._native()) as number;
      const nbFaces = topo.graphFaceCount(g._native()) as number;
      expect(nbEdges).toBe(12);
      expect(nbFaces).toBe(6);
    } finally {
      g.close();
    }
  });
});

describe('backfill — prim', () => {
  it('make_box yields a node', () => {
    occtl.init();
    const g = new occtl.Graph();
    try {
      const node = g.makeBox({ dx: 1, dy: 2, dz: 3 });
      expect(node).toBeDefined();
    } finally {
      g.close();
    }
  });

  it('make_sphere builds a sphere solid', () => {
    occtl.init();
    const g = new occtl.Graph();
    try {
      const node = prim.makeSphere(g._native(), 5.0);
      expect(typeof node).toBe('bigint');
    } finally {
      g.close();
    }
  });

  it('make_cylinder builds a cylinder solid', () => {
    occtl.init();
    const g = new occtl.Graph();
    try {
      const node = g.makeCylinder({ radius: 3.0, height: 4.0 });
      expect(typeof node).toBe('bigint');
    } finally {
      g.close();
    }
  });

  it('Graph primitive helpers cover the generated typed facade', () => {
    occtl.init();
    const g = new occtl.Graph();
    try {
      expect(typeof g.makeCone({ r1: 3.0, r2: 1.0, height: 5.0 })).toBe('bigint');
      expect(typeof g.makeTorus({ r1: 5.0, r2: 1.0 })).toBe('bigint');
      expect(typeof g.makeWedge({ dx: 4.0, dy: 3.0, dz: 2.0, ltx: 1.0 })).toBe('bigint');
    } finally {
      g.close();
    }
  });
});

describe('backfill — text', () => {
  it('text_make_faces dispatcher is registered', () => {
    // The `text` module is not linked in the minimal node binding preset, so we
    // cannot call into the C ABI without crashing on unresolved symbols. We do
    // assert the trampoline dispatcher would route the name — the EXPORTED list
    // is generated from abi.json, so its presence proves the generator wired
    // the entry.
    expect(occtl.EXPORTED_FUNCTIONS).toContain('text_make_faces');
    expect(occtl.EXPORTED_FUNCTIONS).toContain('text_faces_info_init');
  });
});

// Clean up iso scratch file before the test suite ends — no leakage to the
// next agent run.
