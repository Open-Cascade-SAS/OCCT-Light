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

import { describe, it, expect } from "vitest";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";

import { FUNCTION_NAMES_CORE } from "../src/ts/generated/core.js";
import { FUNCTION_NAMES_GEOM } from "../src/ts/generated/geom.js";
import { FUNCTION_NAMES_CURVES } from "../src/ts/generated/curves.js";
import { FUNCTION_NAMES_CURVES2D } from "../src/ts/generated/curves2d.js";
import { FUNCTION_NAMES_SURFACES } from "../src/ts/generated/surfaces.js";
import { FUNCTION_NAMES_TOPO } from "../src/ts/generated/topo.js";
import { FUNCTION_NAMES_PRIM } from "../src/ts/generated/prim.js";
import { FUNCTION_NAMES_TEXT } from "../src/ts/generated/text.js";
import { FUNCTION_NAMES_BOOL_ } from "../src/ts/generated/bool_.js";
import { FUNCTION_NAMES_MESH } from "../src/ts/generated/mesh.js";
import { FUNCTION_NAMES_DE } from "../src/ts/generated/de.js";
import { FUNCTION_NAMES_HEAL } from "../src/ts/generated/heal.js";
import { FUNCTION_NAMES_IO_BREP } from "../src/ts/generated/io_brep.js";
import { FUNCTION_NAMES_IO_STEP } from "../src/ts/generated/io_step.js";
import { FUNCTION_NAMES_IO_IGES } from "../src/ts/generated/io_iges.js";
import { FUNCTION_NAMES_IO_STL } from "../src/ts/generated/io_stl.js";
import { FUNCTION_NAMES_IO_OBJ } from "../src/ts/generated/io_obj.js";
import { FUNCTION_NAMES_IO_GLTF } from "../src/ts/generated/io_gltf.js";
import { FUNCTION_NAMES_IO_PLY } from "../src/ts/generated/io_ply.js";
import { FUNCTION_NAMES_IO_VRML } from "../src/ts/generated/io_vrml.js";
import { FUNCTION_NAMES_VIZ } from "../src/ts/generated/viz.js";
import { ALL_OCCTL_FUNCTIONS, ABI_VERSION, LIBRARY_VERSION } from "../src/ts/generated/_manifest.js";

const __dirname = dirname(fileURLToPath(import.meta.url));
const ABI_PATH = resolve(__dirname, "..", "..", "..", "build", "abi.json");

type AbiFn = { name: string };
type Abi = { abi_version: number; functions: AbiFn[]; library_version: { major: number; minor: number; patch: number } };

function loadAbi(): Abi {
  return JSON.parse(readFileSync(ABI_PATH, "utf8"));
}

function snakeToCamel(s: string): string {
  return s.replace(/_([a-z0-9])/g, (_, c) => c.toUpperCase());
}

describe("tier-3 symbol coverage", () => {
  it("every occtl_* function appears in the generated raw bindings", () => {
    const abi = loadAbi();
    const functions = abi.functions.map((f) => f.name);

    const seen = new Set<string>([
      ...FUNCTION_NAMES_CORE,
      ...FUNCTION_NAMES_GEOM,
      ...FUNCTION_NAMES_CURVES,
      ...FUNCTION_NAMES_CURVES2D,
      ...FUNCTION_NAMES_SURFACES,
      ...FUNCTION_NAMES_TOPO,
      ...FUNCTION_NAMES_PRIM,
      ...FUNCTION_NAMES_TEXT,
      ...FUNCTION_NAMES_BOOL_,
      ...FUNCTION_NAMES_MESH,
      ...FUNCTION_NAMES_DE,
      ...FUNCTION_NAMES_HEAL,
      ...FUNCTION_NAMES_IO_BREP,
      ...FUNCTION_NAMES_IO_STEP,
      ...FUNCTION_NAMES_IO_IGES,
      ...FUNCTION_NAMES_IO_STL,
      ...FUNCTION_NAMES_IO_OBJ,
      ...FUNCTION_NAMES_IO_GLTF,
      ...FUNCTION_NAMES_IO_PLY,
      ...FUNCTION_NAMES_IO_VRML,
      ...FUNCTION_NAMES_VIZ,
    ]);

    const missing = functions.filter((f) => !seen.has(f));
    if (missing.length > 0) {
      throw new Error(
        `Coverage gap: ${missing.length} ABI functions not exposed by the binding:\n  ` +
          missing.slice(0, 20).join("\n  ") +
          (missing.length > 20 ? `\n  …and ${missing.length - 20} more` : ""),
      );
    }
    expect(seen.size).toBe(functions.length);
  });

  it("the manifest matches the on-disk ABI", () => {
    const abi = loadAbi();
    expect(ABI_VERSION).toBe(abi.abi_version);
    expect(LIBRARY_VERSION).toBe(`${abi.library_version.major}.${abi.library_version.minor}.${abi.library_version.patch}`);
    expect(ALL_OCCTL_FUNCTIONS.length).toBe(abi.functions.length);
  });

  it("camelCase mapping is consistent", () => {
    const abi = loadAbi();
    for (const fn of abi.functions.slice(0, 10)) {
      const camel = snakeToCamel(fn.name);
      expect(camel).toMatch(/^occtl[A-Z]/);
    }
  });
});
