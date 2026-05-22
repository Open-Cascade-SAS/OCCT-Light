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

import { describe, it, expect, beforeAll } from "vitest";
import { existsSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";

const __dirname = dirname(fileURLToPath(import.meta.url));
const WASM_PATH = resolve(__dirname, "..", "dist", "occtl.wasm");

describe("OCCT-Light WASM smoke test", () => {
  let occtl: import("../src/ts/index.js").Occtl;
  let Occtl: typeof import("../src/ts/index.js").Occtl;
  let errors: typeof import("../src/ts/errors.js");

  beforeAll(async () => {
    if (!existsSync(WASM_PATH)) {
      throw new Error(
        `Missing ${WASM_PATH}. Build the WASM artifacts before running smoke tests.`,
      );
    }
    // Dynamic import so this file parses even when lib/ has not been built.
    const idx = await import("../src/ts/index.js");
    Occtl = idx.Occtl;
    errors = await import("../src/ts/errors.js");
    occtl = await Occtl.load();
  });

  it("performs the ABI handshake on load", () => {
    expect(occtl.runtimeAbiVersion()).toBe(occtl.ABI_VERSION);
  });

  it("reports a non-zero library version", () => {
    const v = occtl.core.version();
    expect(v.major).toBeGreaterThanOrEqual(0);
    expect(v.minor).toBeGreaterThanOrEqual(0);
    expect(v.patch).toBeGreaterThanOrEqual(0);
  });

  it("statusToString returns a human-readable string", () => {
    const s = occtl.core.statusToString(errors.Status.OK);
    expect(typeof s).toBe("string");
    expect(s.length).toBeGreaterThan(0);
  });

  it("builds a vertex and iterates over the graph's vertices", () => {
    const g = new occtl.Graph();
    try {
      g.makeVertex({ x: 1, y: 2, z: 3 });
      const verts = [...g.vertices()];
      expect(verts.length).toBe(1);
    } finally {
      g.dispose();
    }
  });

  it("supports graph-level primitive and boolean helpers", () => {
    const g = new occtl.Graph();
    try {
      const a = g.makeBox({ dx: 10, dy: 10, dz: 10 });
      const b = g.makeBox({ dx: 10, dy: 10, dz: 10, placement: { location: { x: 5, y: 0, z: 0 } } });
      const fused = g.fuse([a], [b], { runParallel: true, buildHistory: false });
      expect(typeof fused).toBe("bigint");
    } finally {
      g.dispose();
    }
  });

  it("forces an error path and surfaces a typed exception", () => {
    expect(() => occtl.Graph.create.call({}, /* missing args */)).toThrow();
  });

  it("status enum mirrors the C ABI codes", () => {
    expect(errors.Status.OK).toBe(0);
    expect(errors.Status.INVALID_HANDLE).toBe(3);
    expect(errors.Status.WRONG_KIND).toBe(17);
  });
});
