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
import { mkdtempSync, rmSync } from 'node:fs';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import * as occtl from '../src/ts/index.js';

describe('smoke', () => {
  it('abi handshake', () => {
    expect(occtl.runtimeAbiVersion()).toBe(occtl.ABI_VERSION);
  });

  it('build vertex and iterate', () => {
    // Use the explicit-resource-management form. Falls back to .close() if the
    // runtime does not support `using` yet.
    const g = new occtl.Graph();
    try {
      const v = g.makeVertex({ x: 1, y: 2, z: 3 });
      const vertices = [...g.vertices()];
      expect(vertices.length).toBeGreaterThan(0);
      expect(vertices[0]).toBe(v);
    } finally {
      g.close();
    }
  });

  it('supports Graph.create alias', () => {
    const g = occtl.Graph.create();
    try {
      const v = g.makeVertex({ x: 0, y: 0, z: 0 });
      expect(v).not.toBe(occtl.NODE_ID_INVALID);
    } finally {
      g.close();
    }
  });

  it('adopts graph results from modeling operations', () => {
    const g = new occtl.Graph();
    try {
      const box = g.makeBox({ dx: 1, dy: 2, dz: 3 });
      const moved = g.translated(box, { x: 10, y: 0, z: 0 });
      try {
        expect([...moved.graph.solids()].length).toBeGreaterThan(0);
        expect(moved.root).not.toBe(occtl.NODE_ID_INVALID);
      } finally {
        moved.graph.close();
      }
    } finally {
      g.close();
    }
  });

  it('supports Graph boolean helper methods', () => {
    const g = new occtl.Graph();
    try {
      const boxA = g.makeBox({ dx: 10, dy: 10, dz: 10 });
      const boxB = g.makeBox({ dx: 10, dy: 10, dz: 10 });
      const root = g.fuse([boxA], [boxB], { buildHistory: false, runParallel: true });
      expect(root).not.toBe(occtl.NODE_ID_INVALID);
    } finally {
      g.close();
    }
  });

  it('validates graphs and round-trips BRep files', () => {
    const dir = mkdtempSync(join(tmpdir(), 'occtl-node-smoke-'));
    const path = join(dir, 'box.brep');
    const g = new occtl.Graph();
    try {
      const box = g.makeBox({ dx: 1, dy: 2, dz: 3 });
      expect(g.checkIssues()).toEqual([]);
      expect(g.isValid()).toBe(true);
      g.writeBrep(box, path);
    } finally {
      g.close();
    }

    const read = occtl.Graph.readBrep(path);
    try {
      expect(read.root).not.toBe(occtl.NODE_ID_INVALID);
      expect(read.graph.isValid()).toBe(true);
      expect([...read.graph.solids()].length).toBe(1);
    } finally {
      read.graph.close();
      rmSync(dir, { recursive: true, force: true });
    }
  });

  it('round-trips STL memory buffers', () => {
    const g = new occtl.Graph();
    try {
      const box = g.makeBox({ dx: 1, dy: 2, dz: 3 });
      occtl.mesh.meshGenerate(g._native(), [box], {});
      const buffer = g.writeStlMemory(box);
      expect(Buffer.isBuffer(buffer)).toBe(true);
      expect(buffer.length).toBeGreaterThan(0);

      const read = occtl.Graph.readStlMemory(buffer);
      try {
        expect(read.root).not.toBe(occtl.NODE_ID_INVALID);
        expect([...read.graph.faces()].length).toBeGreaterThan(0);
      } finally {
        read.graph.close();
      }
    } finally {
      g.close();
    }
  });

  it('round-trips DE memory buffers by format id', () => {
    const g = new occtl.Graph();
    try {
      const box = g.makeBox({ dx: 1, dy: 2, dz: 3 });
      occtl.mesh.meshGenerate(g._native(), [box], {});
      const buffer = g.writeDeMemory(box, 'stl');
      expect(Buffer.isBuffer(buffer)).toBe(true);
      expect(buffer.length).toBeGreaterThan(0);

      const read = occtl.Graph.readDeMemory('stl', buffer);
      try {
        expect(read.root).not.toBe(occtl.NODE_ID_INVALID);
        expect([...read.graph.faces()].length).toBeGreaterThan(0);
      } finally {
        read.graph.close();
      }
    } finally {
      g.close();
    }
  });

  it('exposes DE string-array metadata', () => {
    const formats = occtl.de.deSupportedFormats() as string[];
    expect(formats).toContain('brep');
    const brepExtensions = occtl.de.deFormatExtensions('brep') as string[];
    expect(brepExtensions).toContain('.brep');
  });

  it('round-trips UID and RefUID wire bytes', () => {
    const uid = 0x050000000000002an as occtl.Uid;
    const uidBytes = occtl.core.uidToBytes(uid);
    expect(Buffer.isBuffer(uidBytes)).toBe(true);
    expect((uidBytes as Buffer).length).toBe(16);
    expect(occtl.core.uidFromBytes(uidBytes)).toBe(uid);

    const refUid = 0x0200000000000007n as occtl.RefUid;
    const refUidBytes = occtl.topo.refUidToBytes(refUid);
    expect(Buffer.isBuffer(refUidBytes)).toBe(true);
    expect((refUidBytes as Buffer).length).toBe(16);
    expect(occtl.topo.refUidFromBytes(refUidBytes)).toBe(refUid);
  });

  it('error path carries message', () => {
    expect(() => occtl.Graph.fromPointerUnsafe(0n)).toThrowError(occtl.InvalidHandleError);
  });
});
