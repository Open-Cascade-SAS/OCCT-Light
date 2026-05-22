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

import { existsSync, readdirSync, readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { basename, dirname, resolve } from "node:path";

const __dirname = dirname(fileURLToPath(import.meta.url));
const PARITY_DIR = resolve(__dirname, "..", "..", "..", "..", "tests", "binding_parity");
const WASM_PATH = resolve(__dirname, "..", "..", "dist", "occtl.wasm");
const envScenario = process.env.OCCTL_PARITY_SCENARIO;
const scenarioPaths = envScenario
  ? [resolve(envScenario)]
  : readdirSync(PARITY_DIR)
    .filter((name) => name.endsWith(".json"))
    .map((name) => resolve(PARITY_DIR, name))
    .sort();

interface Scenario {
  scenario: string;
  expected: Record<string, unknown>;
  params?: Record<string, unknown>;
}

type Actual = Record<string, string | number | boolean>;
type LoadedFacade = Awaited<ReturnType<typeof import("../../src/ts/index.js").Occtl.load>>;
type Graph = InstanceType<LoadedFacade["Graph"]>;
type NodeId = import("../../src/ts/ids.js").NodeId;
type Uid = import("../../src/ts/ids.js").Uid;

async function main(): Promise<void> {
  for (const path of scenarioPaths) {
    const scenario: Scenario = JSON.parse(readFileSync(path, "utf8"));
    if (!existsSync(WASM_PATH)) {
      const reason = "dist/occtl.wasm not built; build WASM artifacts before running parity";
      process.stdout.write(JSON.stringify({
        scenario: scenario.scenario,
        binding: "@occtl/wasm",
        status: "error",
        reason,
      }) + "\n");
      throw new Error(reason);
    }

    const { Occtl } = await import("../../src/ts/index.js");
    const occtl = await Occtl.load();
    const actual = runScenario(occtl, scenario);
    const matches = matchesExpected(scenario.expected, actual);
    process.stdout.write(JSON.stringify({
      scenario: scenario.scenario,
      binding: "@occtl/wasm",
      status: matches ? "ok" : "mismatch",
      actual,
      matches,
    }) + "\n");
    if (!matches) {
      throw new Error(`${basename(path)} did not match expected parity fields`);
    }
  }
}

function runScenario(occtl: LoadedFacade, scenario: Scenario): Actual {
  switch (scenario.scenario) {
    case "build_box": return runBuildBox(occtl, scenario);
    case "fuse_two_boxes": return runFuseTwoBoxes(occtl);
    case "cut_box_corner": return runCutBoxCorner(occtl);
    case "common_two_overlapping_boxes": return runCommonTwoOverlappingBoxes(occtl);
    case "section_two_overlapping_boxes": return runSectionTwoOverlappingBoxes(occtl);
    case "split_box_by_box": return runSplitBoxByBox(occtl);
    case "history_modified_after_fuse": return runHistoryModifiedAfterFuse(occtl);
    default: throw new Error(`Unsupported parity scenario: ${scenario.scenario}`);
  }
}

function runBuildBox(occtl: LoadedFacade, scenario: Scenario): Actual {
  const params = scenario.params ?? {};
  const graph = new occtl.Graph();
  try {
    occtl.prim.makeBox(graph, {
      dx: numberParam(params.dx, 10),
      dy: numberParam(params.dy, 10),
      dz: numberParam(params.dz, 5),
    });
    return counts(graph);
  } finally {
    graph.dispose();
  }
}

function runFuseTwoBoxes(occtl: LoadedFacade): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = occtl.prim.makeBox(graph, { dx: 10, dy: 10, dz: 10 });
    const boxB = occtl.prim.makeBox(graph, box(10, 10, 10, 5, 0, 0));
    const boxAFaceUids = [...graph.faces()].slice(0, 6).map((face) => graph.uidOf(face));
    const result = occtl.bool_.fuse(graph, [boxA], [boxB]);
    return {
      root_kind: graph.nodeKind(result),
      history_modified_nonempty_on_first_face_of_box_a: boxAFaceUids.some((uid) => hasHistoryImage(graph, uid)),
    };
  } finally {
    graph.dispose();
  }
}

function runCutBoxCorner(occtl: LoadedFacade): Actual {
  const graph = new occtl.Graph();
  try {
    const boxNode = occtl.prim.makeBox(graph, { dx: 10, dy: 10, dz: 10 });
    const tool = occtl.prim.makeBox(graph, box(5, 5, 5, 7.5, 7.5, 7.5));
    const result = occtl.bool_.cut(graph, [boxNode], [tool]);
    return {
      root_kind: graph.nodeKind(result),
      solid_count: [...graph.solids()].length,
    };
  } finally {
    graph.dispose();
  }
}

function runCommonTwoOverlappingBoxes(occtl: LoadedFacade): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = occtl.prim.makeBox(graph, { dx: 10, dy: 10, dz: 10 });
    const boxB = occtl.prim.makeBox(graph, box(10, 10, 10, 5, 5, 5));
    const result = occtl.bool_.common(graph, [boxA], [boxB]);
    return {
      root_kind: graph.nodeKind(result),
      solid_count: [...graph.solids()].length,
    };
  } finally {
    graph.dispose();
  }
}

function runSectionTwoOverlappingBoxes(occtl: LoadedFacade): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = occtl.prim.makeBox(graph, { dx: 10, dy: 10, dz: 10 });
    const boxB = occtl.prim.makeBox(graph, box(10, 10, 10, 5, 0, 0));
    const edgesBefore = [...graph.edges()].length;
    const result = occtl.bool_.section(graph, [boxA], [boxB]);
    return {
      root_kind: graph.nodeKind(result),
      edge_count_increased: [...graph.edges()].length > edgesBefore,
    };
  } finally {
    graph.dispose();
  }
}

function runSplitBoxByBox(occtl: LoadedFacade): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = occtl.prim.makeBox(graph, { dx: 10, dy: 10, dz: 10 });
    const boxB = occtl.prim.makeBox(graph, box(10, 10, 10, 0, 0, 5));
    const result = occtl.bool_.split(graph, [boxA], [boxB]);
    return {
      root_kind: graph.nodeKind(result),
      compound_count: graph.nbCompounds,
      solid_count: [...graph.solids()].length,
    };
  } finally {
    graph.dispose();
  }
}

function runHistoryModifiedAfterFuse(occtl: LoadedFacade): Actual {
  const graph = new occtl.Graph();
  try {
    const boxA = occtl.prim.makeBox(graph, { dx: 10, dy: 10, dz: 10 });
    const boxB = occtl.prim.makeBox(graph, box(10, 10, 10, 5, 0, 0));
    const faceUids = [...graph.faces()].slice(0, 12).map((face) => graph.uidOf(face));
    occtl.bool_.fuse(graph, [boxA], [boxB]);
    return {
      box_a_modified_nonempty: faceUids.slice(0, 6).some((uid) => hasHistoryImage(graph, uid)),
      box_b_modified_nonempty: faceUids.slice(6, 12).some((uid) => hasHistoryImage(graph, uid)),
    };
  } finally {
    graph.dispose();
  }
}

function counts(graph: Graph): Actual {
  return {
    face_count: [...graph.faces()].length,
    edge_count: [...graph.edges()].length,
    vertex_count: [...graph.vertices()].length,
    solid_count: [...graph.solids()].length,
  };
}

function hasHistoryImage(graph: Graph, uid: Uid): boolean {
  return graph.historyModified(uid).length !== 0 || graph.historyGenerated(uid).length !== 0;
}

function box(dx: number, dy: number, dz: number, x: number, y: number, z: number) {
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
  return typeof value === "number" ? value : fallback;
}

function matchesExpected(expected: Record<string, unknown>, actual: Actual): boolean {
  for (const [name, expectedValue] of Object.entries(expected)) {
    if (name.endsWith("_at_least")) {
      const actualName = name.slice(0, -"_at_least".length);
      if (typeof actual[actualName] !== "number"
        || (actual[actualName] as number) < (expectedValue as number)) {
        return false;
      }
      continue;
    }

    if (name.endsWith("_in")) {
      const actualName = name.slice(0, -"_in".length);
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

main().catch((err: Error) => {
  process.stderr.write(`parity runner failed: ${err.message}\n${err.stack ?? ""}\n`);
  process.exit(1);
});
