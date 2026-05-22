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

import { readFileSync, writeFileSync, mkdirSync, existsSync } from "node:fs";
import { spawnSync } from "node:child_process";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

// =====================================================================
// Types mirroring abi_dump.py's schema. Not exhaustive — only what we use.
// =====================================================================
type AbiParam = {
  name: string;
  type: string;          // raw C type as printed
  pointer_depth: number;
  direction: "in" | "out" | "inout";
  ownership: "owns" | "borrows" | null;
  elem_type: string | null;
  doc: string;
};
type AbiFunction = {
  name: string;
  header: string;
  return_type: string;
  return_doc: string | null;
  params: AbiParam[];
  retvals: Array<{ code: string; doc: string }>;
  see_also: string[];
  threadsafe: string;
  doc: string;
  calling_convention: string;
};
type AbiField = {
  name: string;
  type: string;
  doc: string;
  size?: number;
  align?: number;
  offset?: number;
};
type AbiEnumValue = { name: string; value: number; doc: string };
type AbiType = {
  name: string;
  kind: "enum" | "struct" | "opaque_handle" | "value_handle" | "alias";
  header: string;
  doc: string;
  size?: number;
  align?: number;
  values?: AbiEnumValue[];
  fields?: AbiField[];
  bits_field?: string;
  underlying?: string;
};
type AbiConstant = {
  name: string;
  value: number | string;
  header: string;
  doc: string;
};
type Abi = {
  abi_version: number;
  library_version: { major: number; minor: number; patch: number };
  headers: string[];
  constants: AbiConstant[];
  functions: AbiFunction[];
  types: AbiType[];
};

// =====================================================================
// Module assignment. The C ABI is organised by header file; we map each
// header onto a src/ts/<module>.ts TS module.
// =====================================================================
const HEADER_TO_MODULE: Record<string, string> = {
  "occtl.h": "core",
  "occtl_core.h": "core",
  "occtl_geom.h": "geom",
  "occtl_curves_common.h": "geom",
  "occtl_curves.h": "curves",
  "occtl_curves2d.h": "curves2d",
  "occtl_surfaces.h": "surfaces",
  "occtl_topo.h": "topo",
  "occtl_topo_build.h": "topo",
  "occtl_topo_relation.h": "topo",
  "occtl_topo_types.h": "topo",
  "occtl_topo_algo.h": "topo",
  "occtl_prim_feature.h": "prim",
  "occtl_prim_sketch.h": "prim",
  "occtl_prim_solid.h": "prim",
  "occtl_prim_sweep.h": "prim",
  "occtl_prim.h": "prim",
  "occtl_text.h": "text",
  "occtl_bool.h": "bool_",
  "occtl_mesh.h": "mesh",
  "occtl_de.h": "de",
  "occtl_io_brep.h": "io_brep",
  "occtl_io_step.h": "io_step",
  "occtl_io_iges.h": "io_iges",
  "occtl_io_stl.h": "io_stl",
  "occtl_io_obj.h": "io_obj",
  "occtl_io_gltf.h": "io_gltf",
  "occtl_io_ply.h": "io_ply",
  "occtl_io_vrml.h": "io_vrml",
  "occtl_viz.h": "viz",
  "occtl_heal.h": "heal",
};
const MODULES = Array.from(new Set(Object.values(HEADER_TO_MODULE)));

function moduleForHeader(h: string): string {
  return HEADER_TO_MODULE[h]!;
}

function validateKnownHeaders(abi: Abi): void {
  const headers = new Set<string>(abi.headers ?? []);
  for (const fn of abi.functions) headers.add(fn.header);
  const unknown = Array.from(headers).filter((h) => !(h in HEADER_TO_MODULE)).sort();
  if (unknown.length) {
    console.error(`generate_facade (wasm): abi.json contains unmapped headers: ${unknown.join(", ")}`);
    process.exit(2);
  }
}

// =====================================================================
// Naming helpers
// =====================================================================
function snakeToCamel(name: string): string {
  return name.replace(/_([a-z0-9])/g, (_, c) => c.toUpperCase());
}
function snakeToPascal(name: string): string {
  const c = snakeToCamel(name);
  return c.charAt(0).toUpperCase() + c.slice(1);
}
function trimOcctlPrefix(name: string): string {
  return name.replace(/^OCCTL_/, "").replace(/^occtl_/, "");
}

// =====================================================================
// Struct layout computation (WASM-1 / WASM-2 — eliminate hand-written
// offset arithmetic by emitting a generated/_layouts.ts module).
// =====================================================================

const WASM32_TYPE_SIZES: Record<string, { size: number; align: number }> = {
  "uint32_t":    { size: 4, align: 4 },
  "int32_t":     { size: 4, align: 4 },
  "int":         { size: 4, align: 4 },
  "uint8_t":     { size: 1, align: 1 },
  "int8_t":      { size: 1, align: 1 },
  "uint16_t":    { size: 2, align: 2 },
  "int16_t":     { size: 2, align: 2 },
  "const void*": { size: 4, align: 4 },
  "void*":       { size: 4, align: 4 },
  "const void *":{ size: 4, align: 4 },
  "void *":      { size: 4, align: 4 },
  "const char*": { size: 4, align: 4 },
  "const char *":{ size: 4, align: 4 },
  "char*":       { size: 4, align: 4 },
  "char *":      { size: 4, align: 4 },
  "double":      { size: 8, align: 8 },
  "float":       { size: 4, align: 4 },
  "size_t":      { size: 4, align: 4 },
  "uint64_t":    { size: 8, align: 8 },
  "int64_t":     { size: 8, align: 8 },
  "uintptr_t":   { size: 4, align: 4 },
  // OCCT value-handle typedefs (uint64_t payload — libclang may spell as
  // "int" in the JSON; we record the correct wasm32 sizes here).
  "occtl_uid_t":       { size: 8, align: 8 },
  "occtl_node_id_t":   { size: 8, align: 8 },
  "occtl_ref_id_t":    { size: 8, align: 8 },
  "occtl_ref_uid_t":   { size: 8, align: 8 },
  "occtl_rep_id_t":    { size: 8, align: 8 },
  "occtl_rep_uid_t":   { size: 8, align: 8 },
};

interface StructLayout {
  size: number;
  align: number;
  fields: Record<string, { offset: number; size: number; type: string }>;
}

function alignUp(offset: number, align: number): number {
  return ((offset + align - 1) / align | 0) * align;
}

const ARRAY_RE = /^(.+?)\[(\d+)\]$/;

function resolveTypeSize(
  typeName: string,
  types: AbiType[],
  resolved: Map<string, { size: number; align: number }>,
): { size: number; align: number } {
  const trimmed = typeName.trim();

  // Array type e.g. `double[12]`.
  const arrMatch = trimmed.match(ARRAY_RE);
  if (arrMatch) {
    const elem = resolveTypeSize(arrMatch[1], types, resolved);
    const count = parseInt(arrMatch[2], 10);
    return { size: elem.size * count, align: elem.align };
  }

  // Built-in.
  const builtin = WASM32_TYPE_SIZES[trimmed];
  if (builtin) return builtin;

  // Already resolved.
  const cached = resolved.get(trimmed);
  if (cached) return cached;

  // Look up in the ABI types catalogue.
  const tdef = types.find((t) => t.name === trimmed);
  if (!tdef) {
    // Unknown — default to pointer-sized (alias / opaque handle not in types).
    const fallback = { size: 4, align: 4 };
    resolved.set(trimmed, fallback);
    return fallback;
  }

  let result: { size: number; align: number };
  switch (tdef.kind) {
    case "struct":
      result = computeStructSizeAlign(tdef, types, resolved);
      break;
    case "value_handle":
      // Prefer libclang-reported size (handles libclang spelling quirks like
      // uint64_t → "int"). Fall back to resolving the bits_field type.
      if (tdef.size !== undefined) {
        result = { size: tdef.size, align: tdef.size >= 8 ? 8 : 4 };
      } else {
        result = resolveTypeSize(tdef.bits_field ?? "uint32_t", types, resolved);
      }
      break;
    case "alias":
      result = resolveTypeSize(tdef.underlying ?? trimmed, types, resolved);
      break;
    case "enum":
      result = { size: 4, align: 4 };
      break;
    case "opaque_handle":
      result = { size: 4, align: 4 };
      break;
    default:
      result = { size: 4, align: 4 };
      break;
  }
  resolved.set(trimmed, result);
  return result;
}

function computeStructSizeAlign(
  structType: AbiType,
  types: AbiType[],
  resolved: Map<string, { size: number; align: number }>,
): { size: number; align: number } {
  // Prefer libclang-reported size/align when available (avoids libclang type
  // spelling quirks like uint64_t → "int").
  if (structType.size !== undefined && structType.size > 0) {
    return { size: structType.size, align: structType.align ?? structType.size };
  }
  let maxAlign = 1;
  let currentOffset = 0;
  for (const field of structType.fields ?? []) {
    const { size, align } = resolveTypeSize(field.type, types, resolved);
    maxAlign = Math.max(maxAlign, align);
    currentOffset = alignUp(currentOffset, align);
    currentOffset += size;
  }
  const totalSize = alignUp(currentOffset, maxAlign);
  return { size: totalSize, align: maxAlign };
}

function computeLayouts(abi: Abi): Record<string, StructLayout> {
  const layouts: Record<string, StructLayout> = {};
  const resolved = new Map<string, { size: number; align: number }>();
  const structTypes = abi.types.filter((t) => t.kind === "struct");

  for (const st of structTypes) {
    const { size, align } = computeStructSizeAlign(st, abi.types, resolved);
    const fields: Record<string, { offset: number; size: number; type: string }> = {};
    let currentOffset = 0;
    let fieldAlign = 1;
    for (const field of st.fields ?? []) {
      const ftype = resolveTypeSize(field.type, abi.types, resolved);
      fieldAlign = Math.max(fieldAlign, ftype.align);
      currentOffset = alignUp(currentOffset, ftype.align);
      fields[field.name] = {
        offset: currentOffset,
        size: ftype.size,
        type: field.type,
      };
      currentOffset += ftype.size;
    }
    const layoutAlign = Math.max(align, fieldAlign);
    const fieldSize = alignUp(currentOffset, layoutAlign);
    // Some abi.json dumps report `size: 1` for structs whose field layout is
    // still complete. Never allocate less than the last field requires.
    layouts[st.name] = { size: Math.max(size, fieldSize), align: layoutAlign, fields };
  }

  // Also emit layouts for value_handle types (so callers can look them up by
  // the typedef name, e.g. `occtl_uid_t`).
  for (const t of abi.types) {
    if (t.kind !== "value_handle") continue;
    if (layouts[t.name]) continue;
    const { size, align } = resolveTypeSize(t.name, abi.types, resolved);
    const bitsType = t.bits_field ?? "uint32_t";
    layouts[t.name] = {
      size,
      align,
      fields: {
        bits: { offset: 0, size, type: bitsType },
      },
    };
  }

  return layouts;
}

function emitLayoutsTs(abi: Abi): string {
  const layouts = computeLayouts(abi);
  const out: string[] = [];
  out.push("// Copyright (c) 2026 Capgemini Engineering Research and Development.");
  out.push("//");
  out.push("// This file is part of OCCT-Light software library.");
  out.push("//");
  out.push("// This library is free software; you can redistribute it and/or modify it under");
  out.push("// the terms of the GNU Affero General Public License version 3 as published");
  out.push("// by the Free Software Foundation, with an option to use any later version.");
  out.push("// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution");
  out.push("// for complete text of the license and disclaimer of any warranty.");
  out.push("//");
  out.push("// Alternatively, this file may be used under the terms of a commercial");
  out.push("// license or contractual agreement.");
  out.push("// SPDX-License-Identifier: AGPL-3.0-or-later");
  out.push("// AUTO-GENERATED by bindings/wasm/tools/generate_facade.ts — do not edit.");
  out.push("//");
  out.push("// WASM struct-layout helpers. Eliminates hand-written offset arithmetic");
  out.push("// in hand-written src/ts/*.ts by emitting a single source-of-truth");
  out.push("// STRUCT_LAYOUTS table, computed from the ABI catalogue with wasm32");
  out.push("// alignment rules.");
  out.push("");
  out.push('import type { RawModule } from "./_raw.js";');
  out.push("");
  out.push("export interface FieldLayout {");
  out.push("  offset: number;");
  out.push("  size: number;");
  out.push("  type: string;");
  out.push("}");
  out.push("");
  out.push("export interface StructLayout {");
  out.push("  size: number;");
  out.push("  align: number;");
  out.push("  fields: Record<string, FieldLayout>;");
  out.push("}");
  out.push("");
  out.push("export const STRUCT_LAYOUTS: Record<string, StructLayout> = {");

  const sortedNames = Object.keys(layouts).sort();
  for (const name of sortedNames) {
    const l = layouts[name];
    out.push(`  "${name}": {`);
    out.push(`    size: ${l.size},`);
    out.push(`    align: ${l.align},`);
    out.push("    fields: {");
    const fieldNames = Object.keys(l.fields).sort();
    for (const fn of fieldNames) {
      const f = l.fields[fn];
      out.push(`      ${fn}: { offset: ${f.offset}, size: ${f.size}, type: "${f.type}" },`);
    }
    out.push("    },");
    out.push("  },");
  }
  out.push("};");
  out.push("");

  out.push("/** Allocate a block large enough for struct `structName`. */");
  out.push("export function mallocStruct(structName: string, mod: RawModule): number {");
  out.push("  const layout = STRUCT_LAYOUTS[structName];");
  out.push('  if (!layout) throw new Error(`Unknown struct: ${structName}`);');
  out.push("  return mod._malloc(layout.size);");
  out.push("}");
  out.push("");

  out.push("/** Write a value to a field of a struct. */");
  out.push("export function setField(");
  out.push("  ptr: number,");
  out.push("  structName: string,");
  out.push("  fieldName: string,");
  out.push("  value: number | bigint,");
  out.push("  mod: RawModule,");
  out.push("): void {");
  out.push("  const layout = STRUCT_LAYOUTS[structName];");
  out.push('  if (!layout) throw new Error(`Unknown struct: ${structName}`);');
  out.push("  const field = layout.fields[fieldName];");
  out.push('  if (!field) throw new Error(`Unknown field: ${structName}.${fieldName}`);');
  out.push("  const offset = ptr + field.offset;");
  out.push("  const setType = emsvType(field.type);");
  out.push("  if (typeof value === \"bigint\") {");
  out.push("    const lo = Number(value & 0xFFFFFFFFn);");
  out.push("    const hi = Number((value >> 32n) & 0xFFFFFFFFn);");
  out.push("    mod.setValue(offset, lo, \"i32\");");
  out.push("    mod.setValue(offset + 4, hi, \"i32\");");
  out.push("  } else {");
  out.push("    mod.setValue(offset, value, setType);");
  out.push("  }");
  out.push("}");
  out.push("");

  out.push("function emsvType(ctype: string): string {");
  out.push("  const t = ctype.trim();");
  out.push('  if (t === "double") return "double";');
  out.push('  if (t === "float") return "float";');
  out.push('  const arrMatch = t.match(/^(.+?)\\[(\\d+)\\]$/);');
  out.push("  if (arrMatch) return emsvType(arrMatch[1]);");
  out.push('  return "i32";');
  out.push("}");

  return out.join("\n") + "\n";
}

// =====================================================================
// CLI
// =====================================================================
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const BINDING_ROOT = resolve(__dirname, "..");

function parseArgs(argv: string[]): { abi: string; outRoot: string } {
  let abi = resolve(BINDING_ROOT, "..", "..", "build", "abi.json");
  let outRoot = BINDING_ROOT;
  for (let i = 0; i < argv.length; ++i) {
    const a = argv[i];
    if (a === "--abi") abi = resolve(argv[++i]);
    else if (a === "--out") outRoot = resolve(argv[++i]);
  }
  return { abi, outRoot };
}

// =====================================================================
// raw.cc emission
//
// Strategy: for each function we emit a forward-declaration extern "C" line
// (so we don't need to chase header order), plus an Embind `function(name, &fn)`
// registration. We register enums via enum_<>. Opaque handles get a class_<>
// with no constructor — the C functions return raw pointers we surface as
// EM_VAL / value-pointer (passed as size_t through to JS, which uses them via
// dedicated accessor methods).
//
// We do NOT register POD structs via value_object<> generically; the count is
// large and the per-struct field listing must be hand-tuned. The hand-written
// src/handles.cc carries the small handful of structs we *do* want exposed
// as value_object (occtl_point3_t, occtl_dir3_t, occtl_vec3_t, etc.).
// Generated raw.cc focuses on functions + enums.
// =====================================================================
function isVoidReturn(fn: AbiFunction): boolean {
  return fn.return_type === "void";
}
function isPointerReturn(fn: AbiFunction): boolean {
  return fn.return_type.includes("*");
}

function paramCType(p: AbiParam): string {
  return p.type;
}

function collectFunctionPointerAliases(abi: Abi): Set<string> {
  const aliases = new Set<string>();
  for (const t of abi.types) {
    if (t.kind !== "alias") continue;
    if ((t.underlying ?? "").includes("(*)")) {
      aliases.add(t.name);
    }
  }
  return aliases;
}

function emitFnForwardDecl(fn: AbiFunction): string {
  const params = fn.params.length === 0
    ? "void"
    : fn.params.map((p) => `${paramCType(p)} ${p.name}`).join(", ");
  return `extern "C" ${fn.return_type} ${fn.name}(${params});`;
}

// For functions whose parameters include pointers, we emit an `_em` wrapper
// that takes EM_PTR (size_t) integers from JS for each pointer, converts to
// the right C pointer, and forwards. This is the canonical Embind escape
// hatch for "register a function that takes raw pointers" — `function(...)`
// recognises `intptr_t` as a pass-through.
function emitFnEmbindWrapper(fn: AbiFunction, functionPointerAliases: Set<string>): string {
  const lines: string[] = [];
  const args: string[] = [];
  const fwdArgs: string[] = [];

  for (const p of fn.params) {
    const isFunctionPointerAlias = p.pointer_depth === 0 && functionPointerAliases.has(p.type.trim());
    if (p.pointer_depth === 0 && !isFunctionPointerAlias) {
      // value-typed parameter — pass through as-is
      args.push(`${paramCType(p)} ${p.name}`);
      fwdArgs.push(p.name);
    } else {
      // pointer parameter — accept uintptr_t from JS, cast back inside
      args.push(`uintptr_t ${p.name}_ptr`);
      fwdArgs.push(`reinterpret_cast<${paramCType(p)}>(${p.name}_ptr)`);
    }
  }

  let ret = fn.return_type;
  let body: string;
  if (isPointerReturn(fn)) {
    // Return pointer as uintptr_t for JS.
    body = `  return reinterpret_cast<uintptr_t>(${fn.name}(${fwdArgs.join(", ")}));`;
    ret = "uintptr_t";
  } else if (isVoidReturn(fn)) {
    body = `  ${fn.name}(${fwdArgs.join(", ")});`;
  } else {
    body = `  return ${fn.name}(${fwdArgs.join(", ")});`;
  }

  lines.push(`static ${ret} ${fn.name}_em(${args.join(", ") || "void"}) {`);
  lines.push(body);
  lines.push(`}`);
  return lines.join("\n");
}

function emitRawCc(abi: Abi): string {
  const headerLine = "// AUTO-GENERATED by bindings/wasm/tools/generate_facade.ts — do not edit.";
  const out: string[] = [];
  out.push("// Copyright (c) 2026 Capgemini Engineering Research and Development.");
  out.push("//");
  out.push("// This file is part of OCCT-Light software library.");
  out.push("//");
  out.push("// This library is free software; you can redistribute it and/or modify it under");
  out.push("// the terms of the GNU Affero General Public License version 3 as published");
  out.push("// by the Free Software Foundation, with an option to use any later version.");
  out.push("// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution");
  out.push("// for complete text of the license and disclaimer of any warranty.");
  out.push("//");
  out.push("// Alternatively, this file may be used under the terms of a commercial");
  out.push("// license or contractual agreement.");
  out.push("// SPDX-License-Identifier: AGPL-3.0-or-later");
  out.push(headerLine);
  out.push("//");
  out.push("// Embind glue exposing every OCCTL_API function to JavaScript.");
  out.push("// Pointers (handles, options structs, out-pointer arguments) cross the");
  out.push("// boundary as uintptr_t — JS allocates with Module._malloc / reads/writes");
  out.push("// HEAP* directly. See lib/views.ts for the heap-grow invalidation policy.");
  out.push("");
  out.push("#include <emscripten/bind.h>");
  out.push("#include <cstdint>");
  out.push("");
  out.push("// We pull the umbrella public header in for the type declarations every");
  out.push("// generated forward-decl below references. The CMakeLists adds");
  out.push("// `include/` to the include path; the umbrella reads only stdint/stddef +");
  out.push("// other occtl_*.h, so we incur no STL / OCCT dependency here.");
  out.push("#include \"occtl/occtl.h\"");
  out.push("");
  out.push("// === Public C ABI declarations =================================================");
  out.push("// Forward declarations are redundant once we include the umbrella, but we");
  out.push("// keep them as a build-time signature lock: if the generator's view of the");
  out.push("// ABI drifts from the headers, the C++ compiler raises a mismatch error.");
  out.push("");

  for (const fn of abi.functions) {
    out.push(`// ${emitFnForwardDecl(fn)}`);
  }
  out.push("");

  out.push("// === Embind-friendly wrappers ==================================================");
  out.push("namespace {");
  out.push("");
  const functionPointerAliases = collectFunctionPointerAliases(abi);
  for (const fn of abi.functions) {
    out.push(emitFnEmbindWrapper(fn, functionPointerAliases));
    out.push("");
  }
  out.push("} // namespace");
  out.push("");

  // === Bindings registrations, grouped by module ===
  for (const mod of MODULES) {
    const fns = abi.functions.filter((f) => moduleForHeader(f.header) === mod);
    const enums = abi.types.filter(
      (t) => t.kind === "enum" && moduleForHeader(t.header) === mod && t.name.endsWith("_t"),
    );
    if (fns.length === 0 && enums.length === 0) continue;

    out.push(`EMSCRIPTEN_BINDINGS(occtl_${mod}) {`);
    out.push("  using namespace emscripten;");
    out.push("");

    // Enums (bind typedef names such as `occtl_curve_kind_t`).
    for (const e of enums) {
      out.push(`  enum_<${e.name}>("${e.name}")`);
      for (const v of e.values ?? []) {
        out.push(`    .value("${v.name}", ${v.name})`);
      }
      out.push("    ;");
    }
    if (enums.length > 0) out.push("");

    // Functions
    for (const fn of fns) {
      out.push(`  function("${fn.name}", &${fn.name}_em);`);
    }
    out.push("}");
    out.push("");
  }

  return out.join("\n") + "\n";
}

// =====================================================================
// TS emission — per-module generated/<module>.ts files.
// Each module gets:
//   * type aliases for enums (as numeric unions or const enums)
//   * function wrapper objects that call into the Embind module
//   * struct type signatures (TS interfaces)
//
// The hand-written lib/<module>.ts imports from generated/<module>.ts and
// adds the idiomatic surface (handle classes, iterators, options builders).
// =====================================================================
function jsDocForFn(fn: AbiFunction, indent: string = ""): string {
  const lines: string[] = [];
  lines.push(`${indent}/**`);
  const summary = (fn.doc || "").split("\n")[0]?.trim() ?? fn.name;
  lines.push(`${indent} * ${summary}`);
  const fullDoc = (fn.doc || "").trim();
  if (fullDoc.length > summary.length + 1) {
    for (const line of fullDoc.split("\n").slice(1)) {
      const trimmed = line.trim();
      if (trimmed) lines.push(`${indent} * ${trimmed}`);
    }
  }
  // @param
  for (const p of fn.params) {
    const pdoc = (p.doc || "").replace(/\n/g, " ").trim();
    if (pdoc) {
      const dirLabel = p.direction === "out" ? "[out] " : p.direction === "inout" ? "[inout] " : "";
      lines.push(`${indent} * @param ${snakeToCamel(p.name)} — ${dirLabel}${pdoc}`);
    }
  }
  // @returns / @throws
  const returnsStatus = fn.return_type === "occtl_status_t";
  if (fn.retvals && fn.retvals.length > 0) {
    for (const rv of fn.retvals) {
      if (returnsStatus && rv.code !== "OCCTL_OK") {
        lines.push(`${indent} * @throws {Error} ${rv.code} — ${rv.doc.replace(/\n/g, " ").trim()}`);
      } else if (!returnsStatus) {
        lines.push(`${indent} * @returns ${rv.code} — ${rv.doc.replace(/\n/g, " ").trim()}`);
      }
    }
  }
  // @threadsafe
  if (fn.threadsafe) {
    lines.push(`${indent} * @threadsafe ${fn.threadsafe.replace(/\n/g, " ").trim()}`);
  }
  // @see
  if (fn.see_also && fn.see_also.length > 0) {
    for (const sa of fn.see_also) {
      lines.push(`${indent} * @see ${sa}`);
    }
  }
  lines.push(`${indent} */`);
  return lines.join("\n");
}

function cTypeToTsRaw(ctype: string, pointerDepth: number): string {
  // Pointers cross the wire as uintptr_t (number); JS uses _malloc/HEAP* to dance.
  if (pointerDepth > 0) return "number";
  // Strip whitespace / qualifiers
  const t = ctype.replace(/\bconst\b/g, "").replace(/\s+/g, "");
  if (t === "void") return "void";
  if (t === "double" || t === "float") return "number";
  if (t === "int" || t === "int32_t" || t === "uint32_t" || t === "uint8_t" || t === "int8_t" ||
      t === "uint16_t" || t === "int16_t" || t === "size_t" || t === "ptrdiff_t") return "number";
  if (t === "int64_t" || t === "uint64_t") return "bigint";
  if (t === "char") return "number";
  // Enums in abi.json appear as `occtl_<name>_t` — they're numeric in JS.
  if (/^occtl_/.test(t)) return "number";
  return "number";
}

function emitGeneratedTs(abi: Abi, mod: string): string {
  const fns = abi.functions.filter((f) => moduleForHeader(f.header) === mod);
  const enums = abi.types.filter(
    (t) => t.kind === "enum" && moduleForHeader(t.header) === mod && t.name.endsWith("_t"),
  );
  const constants = abi.constants.filter((c) => moduleForHeader(c.header) === mod);

  const out: string[] = [];
  out.push("// Copyright (c) 2026 Capgemini Engineering Research and Development.");
  out.push("//");
  out.push("// This file is part of OCCT-Light software library.");
  out.push("//");
  out.push("// This library is free software; you can redistribute it and/or modify it under");
  out.push("// the terms of the GNU Affero General Public License version 3 as published");
  out.push("// by the Free Software Foundation, with an option to use any later version.");
  out.push("// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution");
  out.push("// for complete text of the license and disclaimer of any warranty.");
  out.push("//");
  out.push("// Alternatively, this file may be used under the terms of a commercial");
  out.push("// license or contractual agreement.");
  out.push("// SPDX-License-Identifier: AGPL-3.0-or-later");
  out.push("// AUTO-GENERATED by bindings/wasm/tools/generate_facade.ts — do not edit.");
  out.push("//");
  out.push(`// Raw Embind bindings for module \`${mod}\` (header ${`occtl_${mod}.h`}).`);
  out.push("// Hand-written lib/<module>.ts imports from this file to build the idiomatic surface.");
  out.push("");
  out.push('import type { RawModule } from "./_raw.js";');
  out.push("");

  // Enum exports as numeric-keyed objects.
  for (const e of enums) {
    out.push(`/** ${e.doc || e.name} */`);
    out.push(`export const ${e.name} = Object.freeze({`);
    for (const v of e.values ?? []) {
      out.push(`  ${v.name}: ${v.value},`);
    }
    out.push("} as const);");
    out.push(`export type ${e.name} = number;`);
    if (e.name.endsWith("_t")) {
      const short = e.name.slice(0, -2);
      out.push(`export const ${short} = ${e.name};`);
      out.push(`export type ${short} = ${e.name};`);
    }
    out.push("");
  }

  // Numeric constants (omit stringy ones like macro shorthand).
  for (const c of constants) {
    if (typeof c.value === "number") {
      out.push(`export const ${c.name} = ${c.value};`);
    }
  }
  if (constants.some((c) => typeof c.value === "number")) out.push("");

  // Function wrappers. We emit a `bindRaw<mod>(module)` factory that closes
  // over the loaded Embind Module and returns one object with every raw call.
  out.push(`export interface Raw${snakeToPascal(mod)} {`);
  for (const fn of fns) {
    const params = fn.params
      .map((p) => `${snakeToCamel(p.name)}: ${cTypeToTsRaw(p.type, p.pointer_depth)}`)
      .join(", ");
    const ret = isPointerReturn(fn) ? "number" : cTypeToTsRaw(fn.return_type, 0);
    out.push(jsDocForFn(fn, "  "));
    out.push(`  ${snakeToCamel(fn.name)}(${params}): ${ret};`);
  }
  out.push("}");
  out.push("");
  out.push("// Shared null-guard: helper to surface a clear error when the Embind");
  out.push("// module lacks a symbol that the ABI dump said should exist. Prevents");
  out.push("// the cryptic `TypeError: mod.foo is not a function` in production.");
  out.push("function _getFn(mod: RawModule, name: string): Function {");
  out.push("  const f = (mod as unknown as Record<string, unknown>)[name];");
  out.push("  if (typeof f !== \"function\") {");
  out.push("    throw new Error(name + \" not exported by this build\");");
  out.push("  }");
  out.push("  return f as Function;");
  out.push("}");
  out.push("");
  out.push(`export function bindRaw${snakeToPascal(mod)}(mod: RawModule): Raw${snakeToPascal(mod)} {`);
  out.push("  return {");
  for (const fn of fns) {
    const params = fn.params.map((p) => snakeToCamel(p.name)).join(", ");
    out.push(`    ${snakeToCamel(fn.name)}: (${
      fn.params.map((p) => snakeToCamel(p.name)).join(", ") || ""
    }) => _getFn(mod, "${fn.name}")(${params}),`);
  }
  out.push("  };");
  out.push("}");
  out.push("");

  // Symbol coverage manifest — every C symbol this module wraps.
  out.push(`export const FUNCTION_NAMES_${mod.toUpperCase()}: ReadonlyArray<string> = [`);
  for (const fn of fns) out.push(`  "${fn.name}",`);
  out.push("] as const;");
  out.push("");
  return out.join("\n");
}

// =====================================================================
// _raw.ts — declaration of the Embind module shape itself.
// Combines every per-module function table; private to the binding.
// =====================================================================
function emitRawTs(abi: Abi): string {
  const out: string[] = [];
  out.push("// Copyright (c) 2026 Capgemini Engineering Research and Development.");
  out.push("//");
  out.push("// This file is part of OCCT-Light software library.");
  out.push("//");
  out.push("// This library is free software; you can redistribute it and/or modify it under");
  out.push("// the terms of the GNU Affero General Public License version 3 as published");
  out.push("// by the Free Software Foundation, with an option to use any later version.");
  out.push("// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution");
  out.push("// for complete text of the license and disclaimer of any warranty.");
  out.push("//");
  out.push("// Alternatively, this file may be used under the terms of a commercial");
  out.push("// license or contractual agreement.");
  out.push("// SPDX-License-Identifier: AGPL-3.0-or-later");
  out.push("// AUTO-GENERATED by bindings/wasm/tools/generate_facade.ts — do not edit.");
  out.push("//");
  out.push("// Type signature for the Embind module factory produced by occtl.js.");
  out.push("// Private to the binding. Userland imports from lib/index.ts.");
  out.push("");
  out.push("export interface RawHeap {");
  out.push("  HEAPU8: Uint8Array;");
  out.push("  HEAPU32: Uint32Array;");
  out.push("  HEAPF32: Float32Array;");
  out.push("  HEAPF64: Float64Array;");
  out.push("}");
  out.push("");
  out.push("export interface RawAllocator {");
  out.push("  _malloc(size: number): number;");
  out.push("  _free(ptr: number): void;");
  out.push("  UTF8ToString(ptr: number, maxBytesToRead?: number): string;");
  out.push("  stringToUTF8(s: string, outPtr: number, maxBytes: number): void;");
  out.push("  lengthBytesUTF8(s: string): number;");
  out.push("  getValue(ptr: number, type: string): number;");
  out.push("  setValue(ptr: number, value: number, type: string): void;");
  out.push("}");
  out.push("");
  out.push("/**");
  out.push(" * The shape returned by `await OcctlModule()` (the Emscripten ES-module factory).");
  out.push(" * Every C function from the public ABI appears as a method here.");
  out.push(" */");
  out.push("export interface RawModule extends RawHeap, RawAllocator {");
  for (const fn of abi.functions) {
    const params = fn.params
      .map((p) => `${snakeToCamel(p.name)}: ${cTypeToTsRaw(p.type, p.pointer_depth)}`)
      .join(", ");
    const ret = isPointerReturn(fn) ? "number" : cTypeToTsRaw(fn.return_type, 0);
    out.push(jsDocForFn(fn, "  "));
    out.push(`  ${fn.name}(${params}): ${ret};`);
  }
  out.push("");
  out.push("  // === Notification hook used by lib/views.ts to invalidate stale HEAP* views.");
  out.push("  // Emscripten reassigns these arrays when ALLOW_MEMORY_GROWTH triggers; we");
  out.push("  // wrap _malloc in lib/abi.ts to call invalidateAllViews() before every alloc.");
  out.push("  __occtlNotifyHeapGrowth?: () => void;");
  out.push("}");
  out.push("");
  out.push("export interface OcctlModuleFactory {");
  out.push("  (opts?: Partial<RawModule>): Promise<RawModule>;");
  out.push("}");
  return out.join("\n") + "\n";
}

// =====================================================================
// Manifest — full list of every function across the ABI, used by the
// coverage test.
// =====================================================================
function emitManifest(abi: Abi): string {
  const out: string[] = [];
  out.push("// Copyright (c) 2026 Capgemini Engineering Research and Development.");
  out.push("//");
  out.push("// This file is part of OCCT-Light software library.");
  out.push("//");
  out.push("// This library is free software; you can redistribute it and/or modify it under");
  out.push("// the terms of the GNU Affero General Public License version 3 as published");
  out.push("// by the Free Software Foundation, with an option to use any later version.");
  out.push("// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution");
  out.push("// for complete text of the license and disclaimer of any warranty.");
  out.push("//");
  out.push("// Alternatively, this file may be used under the terms of a commercial");
  out.push("// license or contractual agreement.");
  out.push("// SPDX-License-Identifier: AGPL-3.0-or-later");
  out.push("// AUTO-GENERATED by bindings/wasm/tools/generate_facade.ts — do not edit.");
  out.push("");
  out.push("export const ALL_OCCTL_FUNCTIONS: ReadonlyArray<string> = [");
  for (const fn of abi.functions) out.push(`  "${fn.name}",`);
  out.push("] as const;");
  out.push("");
  out.push(`export const ABI_VERSION = ${abi.abi_version};`);
  out.push(`export const LIBRARY_VERSION = "${abi.library_version.major}.${abi.library_version.minor}.${abi.library_version.patch}";`);
  out.push("");
  return out.join("\n");
}

// =====================================================================
// Main
// =====================================================================
function main(argv: string[]): void {
  const { abi: abiPath, outRoot } = parseArgs(argv);
  if (!existsSync(abiPath)) {
    console.error(`abi.json not found at ${abiPath}`);
    console.error("Generate it first:  python3 tools/abi_dump.py --output build/abi.json");
    process.exit(2);
  }

  // Shared schema check — defer to tools/abi_schema.py so the contract
  // between abi_dump.py and every generator lives in exactly one place.
  const schemaScript = resolve(join(__dirname, "..", "..", "..", "tools", "abi_schema.py"));
  const schemaResult = spawnSync("python3", [schemaScript, abiPath], { stdio: ["ignore", "pipe", "pipe"] });
  if (schemaResult.status !== 0) {
    console.error(`generate_facade (wasm): abi.json schema check failed:\n${schemaResult.stderr}`);
    process.exit(schemaResult.status ?? 1);
  }

  const abi: Abi = JSON.parse(readFileSync(abiPath, "utf8"));
  validateKnownHeaders(abi);

  console.log(`[generate_facade] reading ${abiPath}`);
  console.log(`[generate_facade] functions: ${abi.functions.length}, types: ${abi.types.length}, constants: ${abi.constants.length}`);

  const nativeDir = join(outRoot, "src", "native");
  const tsDir = join(outRoot, "src", "ts");
  const genDir = join(tsDir, "generated");
  mkdirSync(nativeDir, { recursive: true });
  mkdirSync(tsDir, { recursive: true });
  mkdirSync(genDir, { recursive: true });

  // src/native/raw.cc
  const rawCc = emitRawCc(abi);
  writeFileSync(join(nativeDir, "raw.cc"), rawCc);
  console.log(`[generate_facade] wrote src/native/raw.cc (${rawCc.split("\n").length} lines)`);

  // src/ts/generated/_raw.ts
  const rawTs = emitRawTs(abi);
  writeFileSync(join(genDir, "_raw.ts"), rawTs);
  console.log(`[generate_facade] wrote src/ts/generated/_raw.ts (${rawTs.split("\n").length} lines)`);

  // src/ts/generated/_manifest.ts (drives the coverage test)
  const manifest = emitManifest(abi);
  writeFileSync(join(genDir, "_manifest.ts"), manifest);
  console.log(`[generate_facade] wrote src/ts/generated/_manifest.ts`);

  // src/ts/generated/_layouts.ts (struct layout helpers — WASM-1 / WASM-2 fix)
  const layoutsTs = emitLayoutsTs(abi);
  writeFileSync(join(genDir, "_layouts.ts"), layoutsTs);
  console.log(`[generate_facade] wrote src/ts/generated/_layouts.ts (${layoutsTs.split("\n").length} lines)`);

  // src/ts/generated/<module>.ts
  for (const mod of MODULES) {
    const tsBody = emitGeneratedTs(abi, mod);
    writeFileSync(join(genDir, `${mod}.ts`), tsBody);
    console.log(`[generate_facade] wrote src/ts/generated/${mod}.ts`);
  }

  console.log("[generate_facade] done.");
}

main(process.argv.slice(2));
