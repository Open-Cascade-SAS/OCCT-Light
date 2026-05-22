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
import { type NodeId } from '../src/ts/ids.js';

describe('bool', () => {
  it('fuse two overlapping boxes returns root', () => {
    const g = new occtl.Graph();
    try {
      const a = g.makeBox({ dx: 10, dy: 10, dz: 10 });
      const b = g.makeBox({ dx: 10, dy: 10, dz: 10 });
      const r = occtl.bool_.fuse(g, [a as NodeId], [b as NodeId]);
      expect(r).not.toBe(0n);
    } finally {
      g.close();
    }
  });

  it('graph.historyModified returns an array', () => {
    const g = new occtl.Graph();
    try {
      const a = g.makeBox({ dx: 10, dy: 10, dz: 10 });
      const b = g.makeBox({ dx: 10, dy: 10, dz: 10 });
      occtl.bool_.fuse(g, [a as NodeId], [b as NodeId]);
      const out = g.historyModified(g.uidOf(a));
      expect(Array.isArray(out)).toBe(true);
    } finally {
      g.close();
    }
  });

  it('buildHistory false still returns a root', () => {
    const g = new occtl.Graph();
    try {
      const a = g.makeBox({ dx: 5, dy: 5, dz: 5 });
      const b = g.makeBox({ dx: 5, dy: 5, dz: 5 });
      const r = occtl.bool_.cut(g, [a as NodeId], [b as NodeId], { buildHistory: false });
      expect(r).not.toBe(0n);
    } finally {
      g.close();
    }
  });
});
