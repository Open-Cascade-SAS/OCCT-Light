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

import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { resolve, dirname, basename } from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it, expect } from 'vitest';

import * as occtl from '../../src/ts/index.js';
import { type NodeId, type Uid } from '../../src/ts/ids.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const parityDir = resolve(__dirname, '..', '..', '..', '..', 'tests', 'binding_parity');
const envScenario = process.env.OCCTL_PARITY_SCENARIO;
const scenarioPaths = envScenario
  ? [resolve(envScenario)]
  : readdirSync(parityDir)
    .filter((name) => name.endsWith('.json'))
    .map((name) => resolve(parityDir, name))
    .sort();

interface Scenario {
  scenario: string;
  expected: Record<string, unknown>;
  params?: Record<string, unknown>;
}

type Actual = Record<string, string | number | boolean>;

describe('parity scenarios', () => {
  for (const scenarioPath of scenarioPaths) {
    it(`${basename(scenarioPath, '.json')} matches expected fields`, () => {
      expect(existsSync(scenarioPath)).toBe(true);
      const scenario = JSON.parse(readFileSync(scenarioPath, 'utf-8')) as Scenario;
      const actual = runScenario(scenario);
      const matches = matchesExpected(scenario.expected, actual);

      // The runner prints one JSON line for the cross-binding parity harness.
      // eslint-disable-next-line no-console
      console.log(JSON.stringify({
        scenario: scenario.scenario,
        binding: 'node',
        actual,
        matches,
      }));

      expect(matches).toBe(true);
    });
  }
});

function runScenario(scenario: Scenario): Actual {
  switch (scenario.scenario) {
    case 'build_box': return runBuildBox(scenario);
    case 'fuse_two_boxes': return runFuseTwoBoxes();
    case 'cut_box_corner': return runCutBoxCorner();
    case 'common_two_overlapping_boxes': return runCommonTwoOverlappingBoxes();
    case 'section_two_overlapping_boxes': return runSectionTwoOverlappingBoxes();
    case 'split_box_by_box': return runSplitBoxByBox();
    case 'history_modified_after_fuse': return runHistoryModifiedAfterFuse();
    default: throw new Error(`Unsupported parity scenario: ${scenario.scenario}`);
  }
}

function runBuildBox(scenario: Scenario): Actual {
  const params = scenario.params ?? {};
  const dx = numberParam(params.dx, 10);
  const dy = numberParam(params.dy, 10);
  const dz = numberParam(params.dz, 5);

  const graph = new occtl.Graph();
  try {
    graph.makeBox({ dx, dy, dz });
    return {
      face_count: [...graph.faces()].length,
      edge_count: [...graph.edges()].length,
      vertex_count: [...graph.vertices()].length,
      solid_count: [...graph.solids()].length,
    };
  } finally {
    graph.close();
  }
}

function runFuseTwoBoxes(): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = graph.makeBox({ dx: 10, dy: 10, dz: 10 });
    const boxB = graph.makeBox(box(10, 10, 10, 5, 0, 0));
    const boxAFaceUids = [...graph.faces()].slice(0, 6).map((face) => graph.uidOf(face));

    const result = occtl.bool_.fuse(graph, [boxA], [boxB]);
    const historyModifiedNonempty = boxAFaceUids.some((uid) => hasHistoryImage(graph, uid));
    return {
      root_kind: graph.nodeKind(result),
      history_modified_nonempty_on_first_face_of_box_a: historyModifiedNonempty,
    };
  } finally {
    graph.close();
  }
}

function runCutBoxCorner(): Actual {
  const graph = new occtl.Graph();
  try {
    const boxNode = graph.makeBox({ dx: 10, dy: 10, dz: 10 });
    const tool = graph.makeBox(box(5, 5, 5, 7.5, 7.5, 7.5));
    const result = occtl.bool_.cut(graph, [boxNode], [tool]);
    return {
      root_kind: graph.nodeKind(result),
      solid_count: [...graph.solids()].length,
    };
  } finally {
    graph.close();
  }
}

function runCommonTwoOverlappingBoxes(): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = graph.makeBox({ dx: 10, dy: 10, dz: 10 });
    const boxB = graph.makeBox(box(10, 10, 10, 5, 5, 5));
    const result = occtl.bool_.common(graph, [boxA], [boxB]);
    return {
      root_kind: graph.nodeKind(result),
      solid_count: [...graph.solids()].length,
    };
  } finally {
    graph.close();
  }
}

function runSectionTwoOverlappingBoxes(): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = graph.makeBox({ dx: 10, dy: 10, dz: 10 });
    const boxB = graph.makeBox(box(10, 10, 10, 5, 0, 0));
    const edgesBefore = [...graph.edges()].length;
    const result = occtl.bool_.section(graph, [boxA], [boxB]);
    return {
      root_kind: graph.nodeKind(result),
      edge_count_increased: [...graph.edges()].length > edgesBefore,
    };
  } finally {
    graph.close();
  }
}

function runSplitBoxByBox(): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = graph.makeBox({ dx: 10, dy: 10, dz: 10 });
    const boxB = graph.makeBox(box(10, 10, 10, 0, 0, 5));
    const result = occtl.bool_.split(graph, [boxA], [boxB]);
    return {
      root_kind: graph.nodeKind(result),
      compound_count: graph.nbCompounds,
      solid_count: [...graph.solids()].length,
    };
  } finally {
    graph.close();
  }
}

function runHistoryModifiedAfterFuse(): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = graph.makeBox({ dx: 10, dy: 10, dz: 10 });
    const boxB = graph.makeBox(box(10, 10, 10, 5, 0, 0));
    const faceUids = [...graph.faces()].slice(0, 12).map((face) => graph.uidOf(face));
    const boxAFaceUids = faceUids.slice(0, 6);
    const boxBFaceUids = faceUids.slice(6, 12);

    occtl.bool_.fuse(graph, [boxA], [boxB]);
    return {
      box_a_modified_nonempty: boxAFaceUids.some((uid) => hasHistoryImage(graph, uid)),
      box_b_modified_nonempty: boxBFaceUids.some((uid) => hasHistoryImage(graph, uid)),
    };
  } finally {
    graph.close();
  }
}

function hasHistoryImage(graph: occtl.Graph, uid: Uid): boolean {
  return graph.historyModified(uid).length !== 0 || graph.historyGenerated(uid).length !== 0;
}

function box(dx: number, dy: number, dz: number, x: number, y: number, z: number): occtl.prim.BoxInfo {
  return {
    dx,
    dy,
    dz,
    placement: {
      location: { x, y, z },
      x_dir: { x: 1, y: 0, z: 0 },
      x_dir_ref: { x: 0, y: 1, z: 0 },
    },
  };
}

function numberParam(value: unknown, fallback: number): number {
  return typeof value === 'number' ? value : fallback;
}

function matchesExpected(expected: Record<string, unknown>, actual: Actual): boolean {
  for (const [name, expectedValue] of Object.entries(expected)) {
    if (name.endsWith('_at_least')) {
      const actualName = name.slice(0, -'_at_least'.length);
      if (typeof actual[actualName] !== 'number'
        || (actual[actualName] as number) < (expectedValue as number)) {
        return false;
      }
      continue;
    }

    if (name.endsWith('_in')) {
      const actualName = name.slice(0, -'_in'.length);
      if (!Array.isArray(expectedValue) || !expectedValue.includes(actual[actualName])) {
        return false;
      }
      continue;
    }

    if (actual[name] !== expectedValue) {
      return false;
    }
  }
  return true;
}
