#!/usr/bin/env node
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

import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import { dirname, resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { argv, exit, stderr, stdout } from 'node:process';

const __filename = fileURLToPath(import.meta.url);
const __dirname  = dirname(__filename);

// -------- abi.json schema (mirrors tools/abi_dump.py output) ----------------

interface AbiParam {
  name:           string;
  type:           string;
  pointer_depth:  number;
  direction:      'in' | 'out' | 'inout';
  elem_type:      string | null;
  ownership:      'owns' | 'borrows' | null;
  doc:            string;
}

interface AbiFn {
  name:                string;
  header:              string;
  calling_convention:  string;
  return_type:         string;
  return_doc:          string | null;
  params:              AbiParam[];
  retvals:             { code: string; doc: string }[];
  see_also:            string[];
  threadsafe:          string | null;
  doc:                 string;
}

interface AbiField { name: string; type: string; doc: string }

interface AbiType {
  name:    string;
  kind:    'enum' | 'struct' | 'value_handle' | 'opaque_handle' | 'alias';
  header:  string;
  doc:     string;
  values?: { name: string; value: number | string; doc: string }[];
  fields?: AbiField[];
  underlying?: string;
}

interface AbiConst {
  name:   string;
  value:  number | string;
  header: string;
  doc:    string;
}

interface AbiDoc {
  abi_version:     number;
  library_version: string;
  headers:         string[];
  constants:       AbiConst[];
  types:           AbiType[];
  functions:       AbiFn[];
}

// -------- argv --------------------------------------------------------------

function arg(name: string, fallback?: string): string {
  const idx = argv.indexOf(name);
  if (idx === -1) {
    if (fallback === undefined) {
      stderr.write(`generate_facade: missing required flag ${name}\n`);
      exit(2);
    }
    return fallback;
  }
  return argv[idx + 1]!;
}

const abiPath = resolve(arg('--abi', join(__dirname, '..', '..', '..', 'build', 'abi.json')));
const outRoot = resolve(arg('--out', join(__dirname, '..')));
// Overrides live alongside the TS source under tools/, while the compiled JS
// runs from dist-tools/. Look for tools/overrides next to outRoot.
const overridesDir = resolve(join(outRoot, 'tools', 'overrides'));

stdout.write(`generate_facade: reading ${abiPath}\n`);

// Shared schema check — defer to tools/abi_schema.py so the contract between
// abi_dump.py and every generator lives in exactly one place. The CLI exits
// non-zero with a one-line diagnostic on stderr when the catalog drifts.
const schemaScript = resolve(join(__dirname, '..', '..', '..', 'tools', 'abi_schema.py'));
const schemaResult = spawnSync('python3', [schemaScript, abiPath], { stdio: ['ignore', 'pipe', 'pipe'] });
if (schemaResult.status !== 0) {
  stderr.write(`generate_facade (node): abi.json schema check failed:\n${schemaResult.stderr}`);
  exit(schemaResult.status ?? 1);
}

const abi = JSON.parse(readFileSync(abiPath, 'utf-8')) as AbiDoc;

// -------- helpers -----------------------------------------------------------

const NATIVE_DIR = join(outRoot, 'src', 'native');
const TS_DIR = join(outRoot, 'src', 'ts');
const GEN_DIR = join(TS_DIR, 'generated');
mkdirSync(NATIVE_DIR, { recursive: true });
mkdirSync(TS_DIR, { recursive: true });
mkdirSync(GEN_DIR, { recursive: true });

const MODULE_BY_HEADER: Record<string, string> = {
  'occtl.h':           'core',
  'occtl_core.h':      'core',
  'occtl_geom.h':      'geom',
  'occtl_curves_common.h': 'geom',
  'occtl_curves.h':    'geom',
  'occtl_curves2d.h':  'geom',
  'occtl_surfaces.h':  'geom',
  'occtl_topo.h':      'topo',
  'occtl_topo_build.h': 'topo',
  'occtl_topo_relation.h': 'topo',
  'occtl_topo_types.h': 'topo',
  'occtl_topo_algo.h': 'topo',
  'occtl_prim_feature.h': 'prim',
  'occtl_prim_sketch.h': 'prim',
  'occtl_prim_solid.h': 'prim',
  'occtl_prim_sweep.h': 'prim',
  'occtl_prim.h':      'prim',
  'occtl_text.h':      'text',
  'occtl_bool.h':      'bool_',
  'occtl_mesh.h':      'mesh',
  'occtl_de.h':        'de',
  'occtl_io_brep.h':   'io_brep',
  'occtl_io_step.h':   'io_step',
  'occtl_io_iges.h':   'io_iges',
  'occtl_io_stl.h':    'io_stl',
  'occtl_io_obj.h':    'io_obj',
  'occtl_io_gltf.h':   'io_gltf',
  'occtl_io_vrml.h':   'io_vrml',
  'occtl_io_ply.h':    'io_ply',
  'occtl_viz.h':       'viz',
  'occtl_heal.h':      'heal',
};

// Modules whose `lib/<m>.ts` file is hand-written (idiomatic facade). The
// generator routes functions to these module entries for stats and for the
// auto-generated `bool_-raw.gen.ts` shim, but does NOT overwrite the lib file.
const HAND_WRITTEN_MODULES = new Set<string>(['bool_']);

const PRIM_TYPED_NAMES: Record<string, string> = {
  box: 'BoxInfo',
  sphere: 'SphereInfo',
  cylinder: 'CylinderInfo',
  cone: 'ConeInfo',
  torus: 'TorusInfo',
  wedge: 'WedgeInfo',
};

const PRIM_REQUIRED_FIELDS: Record<string, readonly string[]> = {
  box: ['dx', 'dy', 'dz'],
  sphere: ['radius'],
  cylinder: ['radius', 'height'],
  cone: ['r1', 'r2', 'height'],
  torus: ['r1', 'r2'],
  wedge: ['dx', 'dy', 'dz', 'ltx'],
};

const PRIM_TYPED_ORDER = ['box', 'sphere', 'cylinder', 'cone', 'torus', 'wedge'];

function camel(name: string): string {
  return name.replace(/_([a-z0-9])/g, (_m, c: string) => c.toUpperCase());
}

function stripPrefix(fnName: string): string {
  return fnName.replace(/^occtl_/, '');
}

function tsFnName(fnName: string): string {
  return camel(stripPrefix(fnName));
}

function pascal(name: string): string {
  const c = camel(name);
  return c.length ? c[0]!.toUpperCase() + c.slice(1) : c;
}

function cleanDoc(text: string | undefined): string {
  return (text ?? '').replace(/@(c|p)\s+/g, '').replace(/\s+/g, ' ').trim();
}

function moduleFor(fn: AbiFn): string {
  return MODULE_BY_HEADER[fn.header]!;
}

function validateKnownHeaders(doc: AbiDoc): void {
  const headers = new Set<string>(doc.headers ?? []);
  for (const fn of doc.functions) headers.add(fn.header);
  const unknown = [...headers].filter((h) => !(h in MODULE_BY_HEADER)).sort();
  if (unknown.length) {
    stderr.write(`generate_facade (node): abi.json contains unmapped headers: ${unknown.join(', ')}\n`);
    exit(2);
  }
}
validateKnownHeaders(abi);

// Build type lookup tables.
const typesByName = new Map<string, AbiType>();
for (const t of abi.types) typesByName.set(t.name, t);

// Resolve aliases recursively to the underlying canonical type.
function resolveType(typeName: string): AbiType | null {
  let cur = typesByName.get(typeName);
  while (cur && cur.kind === 'alias' && cur.underlying) {
    const next = typesByName.get(cur.underlying);
    if (!next) break;
    cur = next;
  }
  return cur ?? null;
}

function publicPrimName(cName: string): string | null {
  const prefix = 'occtl_prim_';
  const suffix = '_info_t';
  if (!cName.startsWith(prefix) || !cName.endsWith(suffix)) return null;
  const stem = cName.slice(prefix.length, -suffix.length);
  return Object.prototype.hasOwnProperty.call(PRIM_TYPED_NAMES, stem) ? stem : null;
}

// Strip leading "const " and trailing "const" qualifiers, normalise spacing.
function normalisePtrType(spelling: string): string {
  return spelling
    .replace(/\bconst\b/g, '')
    .replace(/\s+/g, ' ')
    .replace(/\s*\*\s*/g, '*')
    .trim();
}

// Strip top-level const but keep pointer asterisks.
function stripConst(spelling: string): string {
  return spelling.replace(/\bconst\b/g, '').replace(/\s+/g, ' ').trim();
}

// Convert "occtl_curve_t *" -> "occtl_curve_t".
function elemTypeOf(spelling: string): string {
  return stripConst(spelling).replace(/\*+$/, '').trim();
}

// -------- Param categorisation ---------------------------------------------
//
// Each ABI parameter is mapped to a "shape". The shape drives the per-arg
// code-emitter helpers.

type ParamShape =
  | { kind: 'scalar';        ctype: string;  jsKind: 'num' | 'int' | 'bool' | 'enum' }
  | { kind: 'id';            ctype: string }                      // node_id / uid / ref / rep
  | { kind: 'pod_value_in';  ctype: string }                       // struct passed by value
  | { kind: 'pod_ptr_in';    ctype: string }                       // const struct*
  | { kind: 'pod_ptr_out';   ctype: string }                       // struct* (direction=out)
  | { kind: 'enum_out';      ctype: string }                       // enum* (out)
  | { kind: 'scalar_out';    ctype: string;  jsKind: 'num' | 'int' | 'bool' }
  | { kind: 'id_out';        ctype: string }
  | { kind: 'string_in' }
  | { kind: 'string_out_buf' }                                     // char* buf with two-call pattern
  | { kind: 'string_out_size' }                                    // size param to two-call
  | { kind: 'string_out_required' }                                // int* out_required
  | { kind: 'handle_in';     handle: string }                       // const? occtl_X_t*
  | { kind: 'handle_out';    handle: string }                       // occtl_X_t**
  | { kind: 'span_struct_in';  elem: string }                      // const T* + count (handled below)
  | { kind: 'span_handle_array_in'; elem: string;  countName: string;  countCtype: string }  // (const ID_t* arr, int/size_t n_arr)
  | { kind: 'span_count_companion'; forName: string }              // count for span_handle_array_in
  | { kind: 'span_pod_view_out'; elem: string;  countNames: string[] }  // const T** out_data + int* out_count (zero-copy)
  | { kind: 'span_pod_view_count'; forName: string }               // count companion(s) for span_pod_view_out
  | { kind: 'numeric_out_buf'; elem: string }                       // out_buf double*/int*/point3_t* (with capacity + out_count companions)
  | { kind: 'numeric_out_capacity' }                                // capacity for out_buf
  | { kind: 'numeric_out_count' }                                   // out_count for out_buf
  | { kind: 'unsupported';   reason: string };

// Names of opaque handles we know how to wrap.
const OPAQUE_HANDLES = new Set<string>([
  'occtl_curve_t',
  'occtl_curve2d_t',
  'occtl_surface_t',
  'occtl_graph_t',
  'occtl_node_iter_t',
  'occtl_topo_explorer_iter_t',
  'occtl_topo_related_iter_t',
  'occtl_topo_axis_hit_iter_t',
  'occtl_topo_touch_iter_t',
  'occtl_topo_intersection_iter_t',
  'occtl_select_iter_t',
  'occtl_select_group_iter_t',
  'occtl_batch_t',
  'occtl_viz_driver_t',
  'occtl_viz_viewer_t',
  'occtl_viz_view_t',
  'occtl_viz_presentable_t',
]);

// JS wrapper class names per handle. NULL means "no ObjectWrap exists — pass
// as Napi::External<T> only". The accessor names differ: TypedHandle<T> exposes
// pointer(); NodeIterHandle exposes ptr() (no TypedHandle base) and GraphHandle
// exposes both — we use ptr() there for consistency with its public API.
const HANDLE_TO_CLASS: Record<string, { cls: string; accessor: string; canAdopt?: boolean } | null> = {
  'occtl_graph_t':         { cls: 'GraphHandle',    accessor: 'ptr', canAdopt: true },
  'occtl_node_iter_t':     { cls: 'NodeIterHandle', accessor: 'ptr', canAdopt: true },
  'occtl_topo_explorer_iter_t': null,
  'occtl_topo_related_iter_t':  null,
  'occtl_topo_axis_hit_iter_t': null,
  'occtl_topo_touch_iter_t': null,
  'occtl_topo_intersection_iter_t': null,
  'occtl_select_iter_t':   null,
  'occtl_select_group_iter_t': null,
  'occtl_batch_t':         null,
  'occtl_viz_driver_t':    null,
  'occtl_viz_viewer_t':    null,
  'occtl_viz_view_t':      null,
  'occtl_viz_presentable_t': null,
};

const ID_TYPES = new Set<string>([
  'occtl_node_id_t',
  'occtl_uid_t',
  'occtl_ref_id_t',
  'occtl_ref_uid_t',
  'occtl_rep_id_t',
  'occtl_rep_uid_t',
  'occtl_joint_id_t',
]);

// Enum types (treated as int32).
const ENUM_TYPES = new Set<string>();
for (const t of abi.types) {
  if (t.kind === 'enum') ENUM_TYPES.add(t.name);
}

// All struct types (POD with fields).
const STRUCT_TYPES = new Set<string>();
for (const t of abi.types) {
  if (t.kind === 'struct') STRUCT_TYPES.add(t.name);
}

function isOcctlPodLike(typeName: string): boolean {
  return /^occtl_.*_t$/.test(typeName)
    && !ID_TYPES.has(typeName)
    && !ENUM_TYPES.has(typeName)
    && !OPAQUE_HANDLES.has(typeName);
}
// Aliases canonicalised: treat alias of struct as struct.
for (const t of abi.types) {
  if (t.kind === 'alias' && t.underlying) {
    const res = resolveType(t.name);
    if (res && res.kind === 'struct') STRUCT_TYPES.add(t.name);
  }
}

function shapeForParam(p: AbiParam, allParams: AbiParam[], idx: number): ParamShape {
  const pd = p.pointer_depth;
  const stripped = stripConst(p.type);  // e.g. "occtl_curve_t**"
  const elem = elemTypeOf(stripped);
  // Heuristic: ABI dumper sometimes mislabels direction. If the name starts with
  // out_ and pointer_depth >= 1, treat as out. Reverse: never demote.
  let direction: 'in' | 'out' | 'inout' = p.direction;
  if (direction === 'in' && pd >= 1 && /^out[_A-Z]/.test(p.name)) {
    direction = 'out';
  }
  // Helpers in this function consult direction via this local override; if the
  // shape result needs to expose it, we mutate p_local before returning.
  const pLocal: AbiParam = { ...p, direction };
  p = pLocal;

  if (pd === 1 && elem === 'char' && p.type.includes('const') && direction === 'in') {
    return { kind: 'string_in' };
  }

  // Special: two-call buffer pattern detection.
  // Pattern: char* buf  +  int bufSize|buf_size  +  int* out_required
  if (pd === 1 && (elem === 'char' || stripped === 'char *' || stripped === 'char*')) {
    return { kind: 'string_out_buf' };
  }
  if (pd === 0 && (p.name === 'bufSize' || p.name === 'buf_size') && (elem === 'int' || elem === 'size_t' || elem === 'int32_t' || elem === 'uint32_t')) {
    const prev = allParams[idx - 1];
    if (prev && stripConst(prev.type) === 'char*') {
      return { kind: 'string_out_size' };
    }
  }
  if (pd === 1 && p.name === 'out_required' && p.direction === 'out' && (elem === 'int' || elem === 'size_t')) {
    return { kind: 'string_out_required' };
  }
  // Numeric two-call: out_buf T* + (capacity|cap) int + out_count int*
  // Only treat as two-call companions if the function actually has the out_buf.
  const hasNumericOutBuf = allParams.some(q => q.name === 'out_buf' && q.pointer_depth === 1);
  if (pd === 1 && p.name === 'out_buf') {
    if (elem === 'double' || elem === 'int' || elem === 'int32_t' || ID_TYPES.has(elem) || elem === 'occtl_point3_t' || elem === 'occtl_point2_t') {
      return { kind: 'numeric_out_buf', elem };
    }
  }
  if (hasNumericOutBuf && pd === 0 && (p.name === 'capacity' || p.name === 'cap') && (elem === 'int' || elem === 'size_t')) {
    return { kind: 'numeric_out_capacity' };
  }
  if (hasNumericOutBuf && pd === 1 && p.name === 'out_count' && p.direction === 'out' && (elem === 'int' || elem === 'size_t')) {
    return { kind: 'numeric_out_count' };
  }

  // Span handle-array pair: (const ID_t* arr, int/size_t n_arr).
  if (pd === 1 && direction === 'in' && ID_TYPES.has(elem)) {
    const next = allParams[idx + 1];
    const singularName = p.name.endsWith('s') ? p.name.slice(0, -1) : p.name;
    const acceptedCountNames = new Set([`n_${p.name}`, `${p.name}_count`, `${singularName}_count`]);
    if (next && next.pointer_depth === 0 && acceptedCountNames.has(next.name)) {
      const nelem = elemTypeOf(stripConst(next.type));
      return { kind: 'span_handle_array_in', elem, countName: next.name, countCtype: nelem };
    }
  }
  // Count companion to a preceding span_handle_array_in.
  if (pd === 0 && idx > 0) {
    const prev = allParams[idx - 1];
    const prevElem = elemTypeOf(stripConst(prev.type));
    const prevSingularName = prev.name.endsWith('s') ? prev.name.slice(0, -1) : prev.name;
    const acceptedCountNames = new Set([`n_${prev.name}`, `${prev.name}_count`, `${prevSingularName}_count`]);
    if (prev.pointer_depth === 1 && ID_TYPES.has(prevElem) && acceptedCountNames.has(p.name)) {
      return { kind: 'span_count_companion', forName: prev.name };
    }
  }

  // Zero-copy POD view return: pd==2 const T** out_data + N count companions.
  if (pd === 2 && direction === 'out' && (elem === 'occtl_point3_t' || elem === 'occtl_point2_t')) {
    const counts: string[] = [];
    for (let k = idx + 1; k < allParams.length; k++) {
      const q = allParams[k];
      if (!q) break;
      const qElem = elemTypeOf(stripConst(q.type));
      const qIsOut = q.direction === 'out' || /^out_/.test(q.name);
      if (q.pointer_depth === 1 && qIsOut && /^out_(count|nb_)/.test(q.name) && (qElem === 'int' || qElem === 'size_t')) {
        counts.push(q.name);
      } else break;
    }
    return { kind: 'span_pod_view_out', elem, countNames: counts };
  }
  // Count companion for a preceding span_pod_view_out.
  if (pd === 1 && /^out_(count|nb_)/.test(p.name) && (elem === 'int' || elem === 'size_t')) {
    // Walk backwards over any earlier `out_*` count companions to find the view.
    let k = idx - 1;
    while (k >= 0) {
      const q = allParams[k];
      if (!q) break;
      if (q.pointer_depth === 2 && (q.direction === 'out' || /^out_/.test(q.name))) {
        const qElem = elemTypeOf(stripConst(q.type));
        if (qElem === 'occtl_point3_t' || qElem === 'occtl_point2_t') {
          return { kind: 'span_pod_view_count', forName: q.name };
        }
        break;
      }
      if (q.pointer_depth === 1 && /^out_(count|nb_)/.test(q.name)) { k--; continue; }
      break;
    }
  }

  if (pd === 0) {
    // By-value param.
    if (ID_TYPES.has(elem)) return { kind: 'id', ctype: elem };
    if (ENUM_TYPES.has(elem)) return { kind: 'scalar', ctype: elem, jsKind: 'enum' };
    if (STRUCT_TYPES.has(elem) || (resolveType(elem)?.kind === 'struct')) {
      return { kind: 'pod_value_in', ctype: elem };
    }
    if (isOcctlPodLike(elem)) {
      return { kind: 'unsupported', reason: `unknown POD value type ${p.type}` };
    }
    if (elem === 'double' || elem === 'float') return { kind: 'scalar', ctype: elem, jsKind: 'num' };
    if (elem === 'int' || elem === 'int32_t' || elem === 'uint32_t' || elem === 'size_t' || elem === 'int64_t' || elem === 'uint64_t') {
      return { kind: 'scalar', ctype: elem, jsKind: 'int' };
    }
    if (elem === 'occtl_status_t') return { kind: 'scalar', ctype: elem, jsKind: 'enum' };
    return { kind: 'unsupported', reason: `unknown by-value type ${p.type}` };
  }

  if (pd === 1) {
    // Pointer param.
    if (OPAQUE_HANDLES.has(elem)) {
      // const? occtl_X_t*  — input handle.
      return { kind: 'handle_in', handle: elem };
    }
    if (ID_TYPES.has(elem) && p.direction === 'out') {
      return { kind: 'id_out', ctype: elem };
    }
    if (ENUM_TYPES.has(elem) && (p.direction === 'out' || p.direction === 'inout')) {
      return { kind: 'enum_out', ctype: elem };
    }
    if ((STRUCT_TYPES.has(elem) || resolveType(elem)?.kind === 'struct')) {
      if (p.direction === 'out' || p.direction === 'inout') {
        return { kind: 'pod_ptr_out', ctype: elem };
      }
      // const struct* in
      return { kind: 'pod_ptr_in', ctype: elem };
    }
    if (isOcctlPodLike(elem)) {
      return { kind: 'unsupported', reason: `unknown POD pointer type ${p.type} direction=${p.direction}` };
    }
    // scalar pointer out
    if (p.direction === 'out' || p.direction === 'inout') {
      if (elem === 'double' || elem === 'float') return { kind: 'scalar_out', ctype: elem, jsKind: 'num' };
      if (elem === 'int' || elem === 'int32_t' || elem === 'uint32_t' || elem === 'size_t' || elem === 'int64_t' || elem === 'uint64_t') {
        return { kind: 'scalar_out', ctype: elem, jsKind: 'int' };
      }
    }
    return { kind: 'unsupported', reason: `unknown pointer-1 type ${p.type} direction=${p.direction}` };
  }

  if (pd === 2) {
    // T** — usually opaque handle out.
    if (OPAQUE_HANDLES.has(elem)) {
      return { kind: 'handle_out', handle: elem };
    }
    return { kind: 'unsupported', reason: `unknown ptr-2 type ${p.type}` };
  }

  return { kind: 'unsupported', reason: `unknown pointer_depth ${pd} for ${p.type}` };
}

// -------- Code emission for unpacking a parameter --------------------------

interface UnpackEmit {
  decls: string[];        // local declarations / setup
  callArg: string;        // expression to pass to the C function
  packOut?: string;       // expression that builds a JS value from this out param
  outKey?: string;        // key name when this is one of several outs
  outShape?: ParamShape;  // for selecting how to wrap
}

function structFieldUnpack(structName: string, srcExpr: string, dstExpr: string, indent: string): string[] {
  const t = resolveType(structName);
  if (!t || t.kind !== 'struct' || !t.fields) {
    return [`${indent}// (no fields for ${structName})`];
  }
  const lines: string[] = [];
  for (const f of t.fields) {
    const ftype = f.type.trim();
    const fname = f.name;
    const dst = `${dstExpr}.${fname}`;
    const src = `${srcExpr}.Get("${fname}")`;
    lines.push(`${indent}if (${srcExpr}.Has("${fname}")) {`);

    // Leave p_next untouched — must remain NULL for forward compat.
    if (fname === 'p_next') {
      lines.push(`${indent}  // p_next must remain NULL`);
      lines.push(`${indent}}`);
      continue;
    }

    if (ftype === 'double' || ftype === 'float') {
      lines.push(`${indent}  ${dst} = ${src}.ToNumber().DoubleValue();`);
    } else if (ftype === 'int' || ftype === 'int32_t' || ftype === 'uint32_t' || ftype === 'size_t' || ftype === 'int64_t' || ftype === 'uint64_t') {
      lines.push(`${indent}  ${dst} = (${ftype})${src}.ToNumber().Int64Value();`);
    } else if (ID_TYPES.has(ftype)) {
      lines.push(`${indent}  { Napi::Value vv = ${src}; uint64_t bits = 0; if (vv.IsBigInt()) { bool lossless = false; bits = vv.As<Napi::BigInt>().Uint64Value(&lossless); } else if (vv.IsNumber()) { bits = (uint64_t)vv.ToNumber().Int64Value(); } else if (vv.IsObject() && vv.As<Napi::Object>().Has("bits")) { Napi::Value b = vv.As<Napi::Object>().Get("bits"); if (b.IsBigInt()) { bool ll = false; bits = b.As<Napi::BigInt>().Uint64Value(&ll); } else { bits = (uint64_t)b.ToNumber().Int64Value(); } } ${dst}.bits = bits; }`);
    } else if (ENUM_TYPES.has(ftype) || ftype.startsWith('occtl_') && ftype.endsWith('_t') && resolveType(ftype)?.kind === 'enum') {
      lines.push(`${indent}  ${dst} = (${ftype})${src}.ToNumber().Int32Value();`);
    } else if (ftype === 'const char *' || ftype === 'const char*') {
      // Need a persistent storage for the Utf8Value — we use a vector held in the
      // caller's frame. The trampoline declares helper string slots; we keep a
      // pointer to one and copy. For struct field, the lifetime of the field is
      // the trampoline's stack; we store a Utf8Value in the surrounding scope
      // via a unique tmp name.
      const tmp = `__s_${fname}_${Math.random().toString(36).slice(2,8)}`;
      // Defer: store in a vector of std::string at trampoline scope. The emitter
      // here just records the value; we need the function-level frame to capture.
      lines.push(`${indent}  __strings.emplace_back(${src}.As<Napi::String>().Utf8Value());`);
      lines.push(`${indent}  ${dst} = __strings.back().c_str();`);
      void tmp;
    } else if (ftype.endsWith('*')) {
      // Borrowed pointer (e.g. const occtl_point3_t*, const occtl_curve_t*).
      const elem = elemTypeOf(ftype);
      if (OPAQUE_HANDLES.has(elem)) {
        const cls = HANDLE_TO_CLASS[elem];
        if (cls) {
          lines.push(`${indent}  { Napi::Value vv = ${src}; if (vv.IsExternal()) { ${dst} = vv.As<Napi::External<${elem}>>().Data(); } else if (vv.IsObject()) { ${cls.cls}* h = Napi::ObjectWrap<${cls.cls}>::Unwrap(vv.As<Napi::Object>()); ${dst} = h ? reinterpret_cast<${elem}*>(h->${cls.accessor}()) : nullptr; } else { ${dst} = nullptr; } }`);
        } else {
          lines.push(`${indent}  { Napi::Value vv = ${src}; ${dst} = vv.IsExternal() ? vv.As<Napi::External<${elem}>>().Data() : nullptr; }`);
        }
      } else if (elem === 'occtl_point3_t' || elem === 'occtl_point2_t' || elem === 'occtl_vector3_t' || elem === 'occtl_oriented_node_t' || ID_TYPES.has(elem) || elem === 'double' || elem === 'int') {
        // Array of POD — allocate from a vector kept at trampoline scope.
        const elemBuf = `__buf_${fname}`;
        lines.push(`${indent}  { Napi::Array __arr = ${src}.As<Napi::Array>(); std::vector<${elem}> ${elemBuf}; ${elemBuf}.resize(__arr.Length());`);
        if (elem === 'occtl_point3_t' || elem === 'occtl_vector3_t') {
          lines.push(`${indent}    for (uint32_t __i = 0; __i < __arr.Length(); ++__i) { Napi::Object __o = __arr.Get(__i).As<Napi::Object>(); ${elemBuf}[__i].x = __o.Get("x").ToNumber().DoubleValue(); ${elemBuf}[__i].y = __o.Get("y").ToNumber().DoubleValue(); if (__o.Has("z")) ${elemBuf}[__i].z = __o.Get("z").ToNumber().DoubleValue(); }`);
        } else if (elem === 'occtl_point2_t') {
          lines.push(`${indent}    for (uint32_t __i = 0; __i < __arr.Length(); ++__i) { Napi::Object __o = __arr.Get(__i).As<Napi::Object>(); ${elemBuf}[__i].x = __o.Get("x").ToNumber().DoubleValue(); ${elemBuf}[__i].y = __o.Get("y").ToNumber().DoubleValue(); }`);
        } else if (ID_TYPES.has(elem)) {
          lines.push(`${indent}    for (uint32_t __i = 0; __i < __arr.Length(); ++__i) { Napi::Value __v = __arr.Get(__i); uint64_t __b = 0; if (__v.IsBigInt()) { bool __l=false; __b = __v.As<Napi::BigInt>().Uint64Value(&__l); } else if (__v.IsNumber()) { __b = (uint64_t)__v.ToNumber().Int64Value(); } ${elemBuf}[__i].bits = __b; }`);
        } else if (elem === 'occtl_oriented_node_t') {
          lines.push(`${indent}    for (uint32_t __i = 0; __i < __arr.Length(); ++__i) { Napi::Object __o = __arr.Get(__i).As<Napi::Object>(); Napi::Value __idv = __o.Get("id"); uint64_t __b = 0; if (__idv.IsBigInt()) { bool __l=false; __b = __idv.As<Napi::BigInt>().Uint64Value(&__l); } else if (__idv.IsNumber()) { __b = (uint64_t)__idv.ToNumber().Int64Value(); } ${elemBuf}[__i].id.bits = __b; ${elemBuf}[__i].orientation = (occtl_orientation_t)(__o.Has("orientation") ? __o.Get("orientation").ToNumber().Int32Value() : 0); }`);
        } else if (elem === 'double') {
          lines.push(`${indent}    for (uint32_t __i = 0; __i < __arr.Length(); ++__i) { ${elemBuf}[__i] = __arr.Get(__i).ToNumber().DoubleValue(); }`);
        } else if (elem === 'int') {
          lines.push(`${indent}    for (uint32_t __i = 0; __i < __arr.Length(); ++__i) { ${elemBuf}[__i] = __arr.Get(__i).ToNumber().Int32Value(); }`);
        }
        // Persist storage across the call: stash into the trampoline-scope blob storage.
        // For pointer fields, we register the storage in a generic vector of unique_ptrs.
        const idTag = `__store_${fname}_${(Math.random()*1e6 | 0)}`;
        lines.push(`${indent}    auto* ${idTag} = new std::vector<${elem}>(std::move(${elemBuf})); __blob_storage.emplace_back([${idTag}](){ delete ${idTag}; }); ${dst} = ${idTag}->data(); }`);
      } else if (resolveType(elem)?.kind === 'struct') {
        // Generic POD struct array — fall back: try to unpack.
        const idTag = `__store_${fname}_${(Math.random()*1e6 | 0)}`;
        lines.push(`${indent}  { Napi::Array __arr = ${src}.As<Napi::Array>(); auto* ${idTag} = new std::vector<${elem}>(__arr.Length()); __blob_storage.emplace_back([${idTag}](){ delete ${idTag}; });`);
        const struct = resolveType(elem)!;
        lines.push(`${indent}    for (uint32_t __i = 0; __i < __arr.Length(); ++__i) { Napi::Object __o = __arr.Get(__i).As<Napi::Object>(); (void)__o;`);
        for (const sf of struct.fields ?? []) {
          if (sf.name === 'p_next' || sf.name === 'struct_version') continue;
          if (sf.type === 'double' || sf.type === 'float') {
            lines.push(`${indent}      if (__o.Has("${sf.name}")) (*${idTag})[__i].${sf.name} = __o.Get("${sf.name}").ToNumber().DoubleValue();`);
          } else if (sf.type === 'int' || sf.type === 'int32_t' || sf.type === 'uint32_t' || sf.type === 'size_t') {
            lines.push(`${indent}      if (__o.Has("${sf.name}")) (*${idTag})[__i].${sf.name} = (${sf.type})__o.Get("${sf.name}").ToNumber().Int64Value();`);
          }
        }
        lines.push(`${indent}    } ${dst} = ${idTag}->data(); }`);
      } else {
        lines.push(`${indent}  // unsupported field pointer type ${ftype}`);
      }
    } else if (/^double\[\d+\]$/.test(ftype)) {
      const len = parseInt(ftype.match(/\[(\d+)\]/)![1]!, 10);
      lines.push(`${indent}  { Napi::Array __a = ${src}.As<Napi::Array>(); for (uint32_t __i = 0; __i < ${len} && __i < __a.Length(); ++__i) ${dst}[__i] = __a.Get(__i).ToNumber().DoubleValue(); }`);
    } else if (resolveType(ftype)?.kind === 'struct') {
      // Nested struct value
      lines.push(`${indent}  { Napi::Object __sub = ${src}.As<Napi::Object>(); (void)__sub;`);
      const sub = resolveType(ftype)!;
      for (const sf of sub.fields ?? []) {
        if (sf.name === 'p_next' || sf.name === 'struct_version') continue;
        const subDst = `${dst}.${sf.name}`;
        const subSrc = `__sub.Get("${sf.name}")`;
        if (sf.type === 'double' || sf.type === 'float') {
          lines.push(`${indent}    if (__sub.Has("${sf.name}")) ${subDst} = ${subSrc}.ToNumber().DoubleValue();`);
        } else if (sf.type === 'int' || sf.type === 'int32_t' || sf.type === 'uint32_t' || sf.type === 'size_t' || sf.type === 'int64_t' || sf.type === 'uint64_t') {
          lines.push(`${indent}    if (__sub.Has("${sf.name}")) ${subDst} = (${sf.type})${subSrc}.ToNumber().Int64Value();`);
        } else if (ENUM_TYPES.has(sf.type) || resolveType(sf.type)?.kind === 'enum') {
          lines.push(`${indent}    if (__sub.Has("${sf.name}")) ${subDst} = (${sf.type})${subSrc}.ToNumber().Int32Value();`);
        } else if (ID_TYPES.has(sf.type)) {
          lines.push(`${indent}    if (__sub.Has("${sf.name}")) { Napi::Value vv = ${subSrc}; uint64_t bits = 0; if (vv.IsBigInt()) { bool ll=false; bits = vv.As<Napi::BigInt>().Uint64Value(&ll); } else if (vv.IsNumber()) { bits = (uint64_t)vv.ToNumber().Int64Value(); } ${subDst}.bits = bits; }`);
        } else if (/^double\[\d+\]$/.test(sf.type)) {
          const len = parseInt(sf.type.match(/\[(\d+)\]/)![1]!, 10);
          lines.push(`${indent}    if (__sub.Has("${sf.name}")) { Napi::Array __a = ${subSrc}.As<Napi::Array>(); for (uint32_t __i = 0; __i < ${len} && __i < __a.Length(); ++__i) ${subDst}[__i] = __a.Get(__i).ToNumber().DoubleValue(); }`);
        } else if (resolveType(sf.type)?.kind === 'struct') {
          // Two-deep nested — flatten one more level (e.g. axis2_placement.location is point3).
          const sub2 = resolveType(sf.type)!;
          lines.push(`${indent}    if (__sub.Has("${sf.name}")) { Napi::Object __sub2 = ${subSrc}.As<Napi::Object>();`);
          for (const sf2 of sub2.fields ?? []) {
            if (sf2.name === 'p_next' || sf2.name === 'struct_version') continue;
            if (sf2.type === 'double' || sf2.type === 'float') {
              lines.push(`${indent}      if (__sub2.Has("${sf2.name}")) ${subDst}.${sf2.name} = __sub2.Get("${sf2.name}").ToNumber().DoubleValue();`);
            } else if (sf2.type === 'int' || sf2.type === 'int32_t' || sf2.type === 'uint32_t' || sf2.type === 'size_t' || sf2.type === 'int64_t' || sf2.type === 'uint64_t') {
              lines.push(`${indent}      if (__sub2.Has("${sf2.name}")) ${subDst}.${sf2.name} = (${sf2.type})__sub2.Get("${sf2.name}").ToNumber().Int64Value();`);
            } else if (/^double\[\d+\]$/.test(sf2.type)) {
              const len = parseInt(sf2.type.match(/\[(\d+)\]/)![1]!, 10);
              lines.push(`${indent}      if (__sub2.Has("${sf2.name}")) { Napi::Array __a = __sub2.Get("${sf2.name}").As<Napi::Array>(); for (uint32_t __i = 0; __i < ${len} && __i < __a.Length(); ++__i) ${subDst}.${sf2.name}[__i] = __a.Get(__i).ToNumber().DoubleValue(); }`);
            } else if (resolveType(sf2.type)?.kind === 'struct') {
              const sub3 = resolveType(sf2.type)!;
              for (const sf3 of sub3.fields ?? []) {
                if (sf3.type === 'double' || sf3.type === 'float') {
                  lines.push(`${indent}      if (__sub2.Has("${sf2.name}")) { Napi::Object __sub3 = __sub2.Get("${sf2.name}").As<Napi::Object>(); if (__sub3.Has("${sf3.name}")) ${subDst}.${sf2.name}.${sf3.name} = __sub3.Get("${sf3.name}").ToNumber().DoubleValue(); }`);
                }
              }
            }
          }
          lines.push(`${indent}    }`);
        }
      }
      lines.push(`${indent}  }`);
    } else {
      lines.push(`${indent}  // unsupported field type ${ftype}`);
    }
    lines.push(`${indent}}`);
  }
  return lines;
}

function structFieldPack(structName: string, srcExpr: string, dstObjExpr: string, indent: string): string[] {
  const t = resolveType(structName);
  if (!t || t.kind !== 'struct' || !t.fields) return [];
  const lines: string[] = [];
  for (const f of t.fields) {
    const ftype = f.type.trim();
    const fname = f.name;
    if (fname === 'p_next' || fname === 'struct_version') continue;
    const src = `${srcExpr}.${fname}`;
    if (ftype === 'double' || ftype === 'float') {
      lines.push(`${indent}${dstObjExpr}.Set("${fname}", Napi::Number::New(env, (double)${src}));`);
    } else if (ftype === 'int' || ftype === 'int32_t' || ftype === 'uint32_t' || ftype === 'size_t' || ftype === 'int64_t' || ftype === 'uint64_t') {
      lines.push(`${indent}${dstObjExpr}.Set("${fname}", Napi::Number::New(env, (double)${src}));`);
    } else if (ID_TYPES.has(ftype)) {
      lines.push(`${indent}${dstObjExpr}.Set("${fname}", Napi::BigInt::New(env, (uint64_t)${src}.bits));`);
    } else if (ENUM_TYPES.has(ftype) || resolveType(ftype)?.kind === 'enum') {
      lines.push(`${indent}${dstObjExpr}.Set("${fname}", Napi::Number::New(env, (int)${src}));`);
    } else if (resolveType(ftype)?.kind === 'struct') {
      lines.push(`${indent}{`);
      lines.push(`${indent}  Napi::Object __nested = Napi::Object::New(env);`);
      lines.push(...structFieldPack(ftype, src, '__nested', indent + '  '));
      lines.push(`${indent}  ${dstObjExpr}.Set("${fname}", __nested);`);
      lines.push(`${indent}}`);
    } else if (ftype === 'const char *' || ftype === 'const char*') {
      lines.push(`${indent}${dstObjExpr}.Set("${fname}", Napi::String::New(env, ${src} ? ${src} : ""));`);
    } else if (/^double\[\d+\]$/.test(ftype)) {
      const len = parseInt(ftype.match(/\[(\d+)\]/)![1]!, 10);
      lines.push(`${indent}{ Napi::Array __a = Napi::Array::New(env, ${len}); for (uint32_t __i = 0; __i < ${len}; ++__i) __a.Set(__i, Napi::Number::New(env, ${src}[__i])); ${dstObjExpr}.Set("${fname}", __a); }`);
    }
    // Pointer-out fields (poles arrays etc.) deliberately omitted — those use
    // dedicated extractor functions in the C ABI.
  }
  return lines;
}

// -------- Per-function trampoline emission ---------------------------------

interface GenResult {
  cc: string;
  category: string;
  unsupported?: string;   // reason if categorisation gave up
}

function generateTrampoline(fn: AbiFn): GenResult {
  // Apply name-based direction heuristics globally before categorisation.
  const params = fn.params.map(p => {
    let direction = p.direction;
    if (direction === 'in' && p.pointer_depth >= 1 && /^out[_A-Z]/.test(p.name)) {
      direction = 'out';
    }
    return { ...p, direction };
  });
  const shapes = params.map((p, i) => shapeForParam(p, params, i));

  // Categorise the function for stats: status-returning vs not, has handle out, etc.
  let category = 'simple';
  const status = fn.return_type === 'occtl_status_t';
  const isVoid = fn.return_type === 'void';
  const returnsStr = fn.return_type === 'const char *' || fn.return_type === 'const char*';
  const returnsInt = ['int', 'int32_t', 'uint32_t', 'int64_t', 'uint64_t', 'size_t'].includes(fn.return_type)
    || ENUM_TYPES.has(fn.return_type);
  const returnsDouble = fn.return_type === 'double' || fn.return_type === 'float';
  const returnsStruct = resolveType(fn.return_type)?.kind === 'struct';

  // Identify expected arg count (only ins that surface as JS args).
  // Out-only params do NOT take a JS arg; they're emitted as locals.
  // String two-call: the JS user passes nothing — we always do the sizing dance.
  const jsArgRoles: Array<{ paramIdx: number; shape: ParamShape }> = [];
  for (let i = 0; i < params.length; i++) {
    const s = shapes[i]!;
    const p = params[i]!;
    if (s.kind === 'string_out_buf' || s.kind === 'string_out_size' || s.kind === 'string_out_required') continue;
    if (s.kind === 'numeric_out_buf' || s.kind === 'numeric_out_capacity' || s.kind === 'numeric_out_count') continue;
    if (s.kind === 'span_count_companion') continue;
    if (s.kind === 'span_pod_view_count') continue;
    if (s.kind === 'span_pod_view_out') continue;
    if (p.name === 'options' && s.kind === 'pod_ptr_in') continue;
    if (p.direction === 'out') {
      // out-only params don't take a JS arg unless they're handle_out (still no arg)
      continue;
    }
    jsArgRoles.push({ paramIdx: i, shape: s });
  }

  // Detect unsupported shapes.
  const unsupportedReasons: string[] = [];
  for (let i = 0; i < shapes.length; i++) {
    const s = shapes[i]!;
    if (s.kind === 'unsupported') unsupportedReasons.push(`${params[i]!.name}: ${s.reason}`);
    if (s.kind === 'pod_value_in') unsupportedReasons.push(`${params[i]!.name}: complex POD value input ${s.ctype}`);
    if (s.kind === 'pod_ptr_out') unsupportedReasons.push(`${params[i]!.name}: complex POD output ${s.ctype}`);
  }
  if (unsupportedReasons.length) {
    return {
      cc: emitUnsupportedTrampoline(fn, unsupportedReasons.join('; ')),
      category: 'unsupported',
      unsupported: unsupportedReasons.join('; '),
    };
  }

  // Build the body.
  const body: string[] = [];
  body.push(`  Napi::Env env = info.Env();`);
  body.push(`  (void)args;`);
  body.push(`  // String-storage / blob-storage scratch space, lifetime = trampoline body.`);
  body.push(`  std::vector<std::string> __strings; __strings.reserve(8);`);
  body.push(`  std::vector<std::function<void()>> __blob_storage;`);
  body.push(`  struct __StorageGuard { std::vector<std::function<void()>>* p; ~__StorageGuard() { if (p) for (auto& f : *p) f(); } } __sg{&__blob_storage};`);

  // 1. Validate arg count.
  body.push(`  if (args.Length() < ${jsArgRoles.length}) {`);
  body.push(`    Napi::TypeError::New(env, "${fn.name}: expected ${jsArgRoles.length} arg(s), got " + std::to_string(args.Length())).ThrowAsJavaScriptException();`);
  body.push(`    return env.Undefined();`);
  body.push(`  }`);

  // 2. Declare locals for every C param, ordered as in C signature.
  const callArgs: string[] = [];
  const outs: Array<{ name: string; pack: string }> = [];
  let jsArgI = 0;

  for (let i = 0; i < params.length; i++) {
    const p = params[i]!;
    const s = shapes[i]!;
    const local = `arg${i}_${p.name.replace(/[^A-Za-z0-9_]/g, '_')}`;

    if (s.kind === 'string_out_buf') continue;
    if (s.kind === 'string_out_size') continue;
    if (s.kind === 'string_out_required') continue;
    if (s.kind === 'numeric_out_buf') continue;
    if (s.kind === 'numeric_out_capacity') continue;
    if (s.kind === 'numeric_out_count') continue;
    if (s.kind === 'span_count_companion') continue;
    if (s.kind === 'span_pod_view_count') continue;

    if (s.kind === 'span_pod_view_out') {
      // Zero-copy view: const T** out_data + 1..N size_t* count companions.
      // Library owns the memory; we alias it via Napi::ArrayBuffer with a no-op
      // deleter. The JS caller must not retain the view past the parent handle.
      // NB: the ABI dumper labels the count param as `int*` but every poles_view
      // header in occtl_curves*.h / occtl_surfaces.h actually uses `size_t*`.
      const viewLocal = `__view_${i}`;
      const sizeofElem = s.elem === 'occtl_point2_t' ? '(2 * sizeof(double))' : '(3 * sizeof(double))';
      body.push(`  const ${s.elem}* ${viewLocal} = nullptr;`);
      callArgs.push(`&${viewLocal}`);
      // Declare count locals up front and route the companions there.
      const countLocals: string[] = [];
      for (let k = 0; k < s.countNames.length; k++) {
        const cname = s.countNames[k]!;
        const cLocal = `__view_${i}_${cname}`;
        body.push(`  size_t ${cLocal} = 0;`);
        countLocals.push(cLocal);
      }
      // Find the matching count companion params (the immediately-following ones)
      // and replace their slot in callArgs by pushing here. We do it inline.
      // Companion parameters do not emit their own iteration (see span_pod_view_count below).
      for (const cl of countLocals) callArgs.push(`&${cl}`);
      // Build a {data, count, ...} JS object. For surfaces we expose nb_u/nb_v.
      const packLines: string[] = [];
      packLines.push(`[&]() -> Napi::Value { `);
      if (s.countNames.length <= 1) {
        const cl = countLocals[0] ?? '0';
        packLines.push(`  size_t __byte_len = (size_t)${cl} * ${sizeofElem}; `);
      } else {
        // Surface: byte_len = nb_u * nb_v * sizeof(elem).
        const prod = countLocals.join(' * ');
        packLines.push(`  size_t __byte_len = (size_t)(${prod}) * ${sizeofElem}; `);
      }
      packLines.push(`  Napi::ArrayBuffer __ab = ${viewLocal} && __byte_len > 0 `);
      packLines.push(`    ? Napi::ArrayBuffer::New(env, (void*)${viewLocal}, __byte_len, [](Napi::Env, void*){}) `);
      packLines.push(`    : Napi::ArrayBuffer::New(env, 0); `);
      packLines.push(`  Napi::Object __o = Napi::Object::New(env); `);
      packLines.push(`  __o.Set("data", __ab); `);
      if (s.countNames.length === 1) {
        const cname = s.countNames[0]!;
        packLines.push(`  __o.Set("${cname.replace(/^out_/, '')}", Napi::Number::New(env, (double)${countLocals[0]})); `);
      } else {
        for (let k = 0; k < s.countNames.length; k++) {
          const cname = s.countNames[k]!.replace(/^out_/, '');
          packLines.push(`  __o.Set("${cname}", Napi::Number::New(env, (double)${countLocals[k]})); `);
        }
      }
      packLines.push(`  return __o; }()`);
      outs.push({ name: 'view', pack: packLines.join('') });
      continue;
    }

    if (s.kind === 'span_handle_array_in') {
      // Input array pair: (const T* arr, n_T n).
      const j = jsArgI++;
      const jsVal = `args.Get(${j}u)`;
      const vecLocal = `__arr_${i}`;
      body.push(`  std::vector<${s.elem}> ${vecLocal};`);
      body.push(`  { Napi::Value __vv = ${jsVal};`);
      body.push(`    if (__vv.IsArray()) {`);
      body.push(`      Napi::Array __a = __vv.As<Napi::Array>();`);
      body.push(`      ${vecLocal}.resize(__a.Length());`);
      body.push(`      for (uint32_t __k = 0; __k < __a.Length(); ++__k) {`);
      body.push(`        Napi::Value __ev = __a.Get(__k); uint64_t __bits = 0;`);
      body.push(`        if (__ev.IsBigInt()) { bool __l = false; __bits = __ev.As<Napi::BigInt>().Uint64Value(&__l); }`);
      body.push(`        else if (__ev.IsNumber()) { __bits = (uint64_t)__ev.ToNumber().Int64Value(); }`);
      body.push(`        else if (__ev.IsObject() && __ev.As<Napi::Object>().Has("bits")) { Napi::Value __b = __ev.As<Napi::Object>().Get("bits"); if (__b.IsBigInt()) { bool __l = false; __bits = __b.As<Napi::BigInt>().Uint64Value(&__l); } else { __bits = (uint64_t)__b.ToNumber().Int64Value(); } }`);
      body.push(`        ${vecLocal}[__k].bits = __bits;`);
      body.push(`      }`);
      body.push(`    } }`);
      callArgs.push(`${vecLocal}.empty() ? nullptr : ${vecLocal}.data()`);
      callArgs.push(`(${s.countCtype})${vecLocal}.size()`);
      continue;
    }

    if (p.direction === 'out' || (p.direction === 'inout' && (s.kind === 'pod_ptr_out' || s.kind === 'scalar_out' || s.kind === 'id_out' || s.kind === 'enum_out' || s.kind === 'handle_out'))) {
      // Output param.
      if (s.kind === 'handle_out') {
        body.push(`  ${s.handle}* ${local} = nullptr;`);
        callArgs.push(`&${local}`);
        const cls = HANDLE_TO_CLASS[s.handle];
        if (cls && cls.canAdopt) {
          // Wrap into the ObjectWrap subclass so the JS side receives a real
          // {close, disposed} handle. The class destructor owns the pointer.
          outs.push({
            name: p.name,
            pack: `[&]() -> Napi::Value { if (!${local}) return env.Null(); Napi::Function __ctor = ${cls.cls}::GetClass(env); Napi::Object __obj = __ctor.New({}); ${cls.cls}* __h = Napi::ObjectWrap<${cls.cls}>::Unwrap(__obj); if (__h) __h->adopt(${local}); return __obj; }()`,
          });
        } else {
          // Wrap into Napi::External<T>; attach a finalizer that calls the C *_free
          // when one exists. occtl_batch_t has no _free (commit/abort consume it).
          const freeFn = `occtl_${s.handle.replace(/^occtl_/, '').replace(/_t$/, '')}_free`;
          const hasFree = abi.functions.some(f => f.name === freeFn);
          if (hasFree) {
            outs.push({
              name: p.name,
              pack: `Napi::External<${s.handle}>::New(env, ${local}, [](Napi::Env, ${s.handle}* p){ if (p) ${freeFn}(p); })`,
            });
          } else {
            outs.push({
              name: p.name,
              pack: `Napi::External<${s.handle}>::New(env, ${local})`,
            });
          }
        }
      } else if (s.kind === 'pod_ptr_out') {
        body.push(`  ${s.ctype} ${local} = {};`);
        callArgs.push(`&${local}`);
        const packLines: string[] = [];
        packLines.push(`[&](){ Napi::Object __o = Napi::Object::New(env);`);
        for (const line of structFieldPack(s.ctype, local, '__o', '')) packLines.push(line.trim());
        packLines.push(`return __o; }()`);
        outs.push({ name: p.name, pack: packLines.join(' ') });
      } else if (s.kind === 'scalar_out') {
        // ABI dump occasionally reports `int *` where the C header uses
        // `uint32_t *`, `size_t *`, or `uint8_t *`. Keep the emitted local type
        // aligned with the public headers so native compilation remains the
        // final guard for generator drift.
        let ct = s.ctype;
        const sizeTCountFunctions = new Set([
          'occtl_de_format_count',
          'occtl_graph_uid_table',
          'occtl_graph_ref_uid_table',
          'occtl_topo_check',
        ]);
        if (p.name === 'out_bytes') {
          ct = 'uint8_t';
        } else if (p.name === 'out_count' && sizeTCountFunctions.has(fn.name)) {
          ct = 'size_t';
        } else if (ct === 'int' && (p.name === 'out_count' || /_count$/.test(p.name) || /^out_(major|minor|patch|version|nb_)/.test(p.name))) {
          ct = 'uint32_t';
        }
        body.push(`  ${ct} ${local} = 0;`);
        callArgs.push(`&${local}`);
        outs.push({ name: p.name, pack: `Napi::Number::New(env, (double)${local})` });
      } else if (s.kind === 'enum_out') {
        body.push(`  ${s.ctype} ${local} = (${s.ctype})0;`);
        callArgs.push(`&${local}`);
        outs.push({ name: p.name, pack: `Napi::Number::New(env, (int)${local})` });
      } else if (s.kind === 'id_out') {
        body.push(`  ${s.ctype} ${local} = {0};`);
        callArgs.push(`&${local}`);
        outs.push({ name: p.name, pack: `Napi::BigInt::New(env, (uint64_t)${local}.bits)` });
      } else {
        return { cc: emitUnsupportedTrampoline(fn, `unhandled out shape kind=${s.kind}`), category: 'unsupported', unsupported: `out kind ${s.kind}` };
      }
      continue;
    }

    // Input param — consume one JS arg.
    const j = jsArgI++;
    const jsVal = `args.Get(${j}u)`;

    switch (s.kind) {
      case 'scalar': {
        if (s.jsKind === 'num') {
          body.push(`  ${s.ctype} ${local} = (${s.ctype})${jsVal}.ToNumber().DoubleValue();`);
        } else {
          body.push(`  ${s.ctype} ${local} = (${s.ctype})${jsVal}.ToNumber().Int64Value();`);
        }
        callArgs.push(local);
        break;
      }
      case 'id': {
        body.push(`  ${s.ctype} ${local} = {0};`);
        body.push(`  { Napi::Value vv = ${jsVal}; uint64_t bits = 0; if (vv.IsBigInt()) { bool ll=false; bits = vv.As<Napi::BigInt>().Uint64Value(&ll); } else if (vv.IsNumber()) { bits = (uint64_t)vv.ToNumber().Int64Value(); } else if (vv.IsObject() && vv.As<Napi::Object>().Has("bits")) { Napi::Value b = vv.As<Napi::Object>().Get("bits"); if (b.IsBigInt()) { bool ll=false; bits = b.As<Napi::BigInt>().Uint64Value(&ll); } else { bits = (uint64_t)b.ToNumber().Int64Value(); } } ${local}.bits = bits; }`);
        callArgs.push(local);
        break;
      }
      case 'pod_value_in': {
        body.push(`  ${s.ctype} ${local} = {};`);
        body.push(`  if (${jsVal}.IsObject()) {`);
        body.push(`    Napi::Object __o = ${jsVal}.As<Napi::Object>();`);
        const lines = structFieldUnpack(s.ctype, '__o', local, '    ');
        body.push(...lines);
        body.push(`  }`);
        callArgs.push(local);
        break;
      }
      case 'pod_ptr_in': {
        body.push(`  ${s.ctype} ${local} = {};`);
        // Prefer the runtime occtl_*_info_init function when one exists — it
        // sets sane defaults (placement, angles, tolerances). Fall back to the
        // struct_version + zero-init.
        const t = resolveType(s.ctype);
        if (t && t.fields && t.fields.some(f => f.name === 'struct_version')) {
          const initFn = `${s.ctype.replace(/_t$/, '')}_init`;
          const hasInit = abi.functions.some(f => f.name === initFn);
          if (hasInit) {
            body.push(`  ${initFn}(&${local});`);
          } else {
            const cname = (s.ctype.replace(/_t$/, '').toUpperCase()) + '_VERSION_1';
            const exists = abi.constants.some(c => c.name === cname);
            if (exists) body.push(`  ${local}.struct_version = ${cname};`);
            else        body.push(`  ${local}.struct_version = 1;`);
          }
        }
        body.push(`  if (${jsVal}.IsObject()) {`);
        body.push(`    Napi::Object __o = ${jsVal}.As<Napi::Object>();`);
        body.push(...structFieldUnpack(s.ctype, '__o', local, '    '));
        body.push(`  }`);
        callArgs.push(`&${local}`);
        break;
      }
      case 'string_in': {
        body.push(`  __strings.emplace_back(${jsVal}.As<Napi::String>().Utf8Value());`);
        body.push(`  const char* ${local} = __strings.back().c_str();`);
        callArgs.push(local);
        break;
      }
      case 'handle_in': {
        const cls = HANDLE_TO_CLASS[s.handle];
        body.push(`  ${s.handle}* ${local} = nullptr;`);
        if (cls) {
          body.push(`  { Napi::Value vv = ${jsVal}; if (vv.IsExternal()) { ${local} = vv.As<Napi::External<${s.handle}>>().Data(); } else if (vv.IsObject()) { ${cls.cls}* h = Napi::ObjectWrap<${cls.cls}>::Unwrap(vv.As<Napi::Object>()); if (h) ${local} = reinterpret_cast<${s.handle}*>(h->${cls.accessor}()); } }`);
        } else {
          body.push(`  { Napi::Value vv = ${jsVal}; if (vv.IsExternal()) { ${local} = vv.As<Napi::External<${s.handle}>>().Data(); } }`);
        }
        callArgs.push(local);
        break;
      }
      default: {
        return { cc: emitUnsupportedTrampoline(fn, `unhandled in-shape kind=${s.kind}`), category: 'unsupported', unsupported: `in kind ${s.kind}` };
      }
    }
  }

  // 3. Two-call buffer pattern detection for the function as a whole.
  // Pattern: presence of (char* buf, int bufSize, int* out_required).
  const twoCallIdxs: number[] = [];
  for (let i = 0; i < shapes.length; i++) {
    if (shapes[i]!.kind === 'string_out_buf') twoCallIdxs.push(i);
  }
  const isTwoCall = twoCallIdxs.length === 1;
  // Numeric two-call pattern: out_buf T* + capacity int + out_count int*.
  const numIdx = shapes.findIndex(s => s.kind === 'numeric_out_buf');
  const isNumericTwoCall = numIdx >= 0 && shapes.some(s => s.kind === 'numeric_out_capacity') && shapes.some(s => s.kind === 'numeric_out_count');

  if (isNumericTwoCall) {
    category = 'numeric_two_call';
    const elemType = (shapes[numIdx] as { kind: 'numeric_out_buf'; elem: string }).elem;
    // Build sizing/refill arg lists.
    const sizing: string[] = [];
    const refill: string[] = [];
    let ca = 0;
    for (let i = 0; i < params.length; i++) {
      const s = shapes[i]!;
      if (s.kind === 'numeric_out_buf') {
        sizing.push('nullptr');
        refill.push('__buf.data()');
      } else if (s.kind === 'numeric_out_capacity') {
        sizing.push('(size_t)0');
        refill.push('__buf.size()');
      } else if (s.kind === 'numeric_out_count') {
        sizing.push('&__count');
        refill.push('&__count');
      } else {
        sizing.push(callArgs[ca]!);
        refill.push(callArgs[ca]!);
        ca++;
      }
    }
    // Use int32_t as buffer element when the API uses int — matches int32_t* headers.
    const vecElem = elemType === 'int' ? 'int32_t' : elemType;
    body.push(`  size_t __count = 0;`);
    body.push(`  occtl_status_t __ns1 = ${fn.name}(${sizing.join(', ')});`);
    body.push(`  if (__ns1 != OCCTL_OK && __ns1 != OCCTL_BUFFER_TOO_SMALL) { ThrowFromStatus(env, __ns1); return env.Undefined(); }`);
    body.push(`  std::vector<${vecElem}> __buf(__count);`);
    body.push(`  if (__count > 0) {`);
    body.push(`    occtl_status_t __ns2 = ${fn.name}(${refill.join(', ')});`);
    body.push(`    if (__ns2 != OCCTL_OK) { ThrowFromStatus(env, __ns2); return env.Undefined(); }`);
    body.push(`  }`);
    // Pack: turn buf into a JS array.
    if (elemType === 'double') {
      body.push(`  Napi::Array __arr = Napi::Array::New(env, __count);`);
      body.push(`  for (size_t __i = 0; __i < __count; ++__i) __arr.Set((uint32_t)__i, Napi::Number::New(env, __buf[__i]));`);
      body.push(`  return __arr;`);
    } else if (elemType === 'int' || elemType === 'int32_t') {
      body.push(`  Napi::Array __arr = Napi::Array::New(env, __count);`);
      body.push(`  for (size_t __i = 0; __i < __count; ++__i) __arr.Set((uint32_t)__i, Napi::Number::New(env, (double)__buf[__i]));`);
      body.push(`  return __arr;`);
    } else if (elemType === 'occtl_point3_t') {
      body.push(`  Napi::Array __arr = Napi::Array::New(env, __count);`);
      body.push(`  for (size_t __i = 0; __i < __count; ++__i) { Napi::Object __o = Napi::Object::New(env); __o.Set("x", Napi::Number::New(env, __buf[__i].x)); __o.Set("y", Napi::Number::New(env, __buf[__i].y)); __o.Set("z", Napi::Number::New(env, __buf[__i].z)); __arr.Set((uint32_t)__i, __o); }`);
      body.push(`  return __arr;`);
    } else if (elemType === 'occtl_point2_t') {
      body.push(`  Napi::Array __arr = Napi::Array::New(env, __count);`);
      body.push(`  for (size_t __i = 0; __i < __count; ++__i) { Napi::Object __o = Napi::Object::New(env); __o.Set("x", Napi::Number::New(env, __buf[__i].x)); __o.Set("y", Napi::Number::New(env, __buf[__i].y)); __arr.Set((uint32_t)__i, __o); }`);
      body.push(`  return __arr;`);
    } else if (ID_TYPES.has(elemType)) {
      body.push(`  Napi::Array __arr = Napi::Array::New(env, __count);`);
      body.push(`  for (size_t __i = 0; __i < __count; ++__i) __arr.Set((uint32_t)__i, Napi::BigInt::New(env, (uint64_t)__buf[__i].bits));`);
      body.push(`  return __arr;`);
    }
    // Stop here for this function-body emit path.
    body.push(`}`);
    const cc = `static Napi::Value Call_${stripPrefix(fn.name)}(Napi::Env env_ignored, const Napi::Array& args, const Napi::CallbackInfo& info) {\n${body.slice(0, -1).join('\n')}\n}\n`;
    return { cc, category };
  }

  // We need to bypass the next block when numeric two-call already ran.
  // (kept on its own line to make patch safer)
  if (isTwoCall) {
    category = 'string_two_call';
    // Replace the relevant args with calls to the two-call dance.
    body.push(`  // Two-call buffer pattern: size then refill.`);
    body.push(`  size_t __required = 0;`);
    // Build a per-call args list with NULL/0 for sizing pass.
    const sizingArgs: string[] = [];
    const refillArgs: string[] = [];
    // Iterate the C params; for char*/size/required substitute appropriately.
    let bufNamed = 'nullptr', sizeNamed = '0';
    for (let i = 0; i < params.length; i++) {
      const s = shapes[i]!;
      if (s.kind === 'string_out_buf') {
        sizingArgs.push('nullptr');
        refillArgs.push('__buf.data()');
        bufNamed = '__buf.data()';
      } else if (s.kind === 'string_out_size') {
        sizingArgs.push('0');
        refillArgs.push('(int)__buf.size()');
        sizeNamed = '(int)__buf.size()';
      } else if (s.kind === 'string_out_required') {
        sizingArgs.push('&__required');
        refillArgs.push('&__required');
      } else {
        // Reuse the local declared earlier.
        // Find the corresponding callArg already collected. We did push them in order.
        // Easier: re-derive.
      }
    }
    // Map non-special args from the collected callArgs (in order).
    // Build the merged call lists by walking params + shapes.
    const sizing: string[] = [];
    const refill: string[] = [];
    let ca = 0;
    for (let i = 0; i < params.length; i++) {
      const s = shapes[i]!;
      if (s.kind === 'string_out_buf') { sizing.push('nullptr'); refill.push('__buf.data()'); continue; }
      if (s.kind === 'string_out_size') { sizing.push('(size_t)0'); refill.push('__buf.size()'); continue; }
      if (s.kind === 'string_out_required') { sizing.push('&__required'); refill.push('&__required'); continue; }
      sizing.push(callArgs[ca]!);
      refill.push(callArgs[ca]!);
      ca++;
    }
    body.push(`  occtl_status_t __s1 = ${fn.name}(${sizing.join(', ')});`);
    body.push(`  if (__s1 != OCCTL_OK && __s1 != OCCTL_BUFFER_TOO_SMALL) { ThrowFromStatus(env, __s1); return env.Undefined(); }`);
    body.push(`  std::vector<char> __buf(__required > 0 ? __required : 1, 0);`);
    body.push(`  occtl_status_t __s2 = ${fn.name}(${refill.join(', ')});`);
    body.push(`  if (__s2 != OCCTL_OK) { ThrowFromStatus(env, __s2); return env.Undefined(); }`);
    body.push(`  return Napi::String::New(env, __buf.data());`);
    void bufNamed; void sizeNamed;
  } else {
    // 4. Make the call.
    const callExpr = `${fn.name}(${callArgs.join(', ')})`;

    if (status) {
      body.push(`  occtl_status_t __status = ${callExpr};`);
      body.push(`  if (__status != OCCTL_OK) { ThrowFromStatus(env, __status); return env.Undefined(); }`);
    } else if (isVoid) {
      body.push(`  ${callExpr};`);
    } else if (returnsStr) {
      body.push(`  const char* __ret = ${callExpr};`);
    } else if (returnsInt) {
      body.push(`  int __ret = (int)${callExpr};`);
    } else if (returnsDouble) {
      body.push(`  double __ret = ${callExpr};`);
    } else if (returnsStruct) {
      body.push(`  ${fn.return_type} __ret = ${callExpr};`);
      body.push(`  (void)__ret;`);
    } else if (fn.return_type === 'const occtl_error_t *') {
      body.push(`  const occtl_error_t* __ret = ${callExpr};`);
    } else {
      body.push(`  auto __ret = ${callExpr};`);
    }

    // 5. Pack outputs.
    if (status) {
      if (outs.length === 0) {
        body.push(`  return env.Undefined();`);
      } else if (outs.length === 1) {
        body.push(`  return ${outs[0]!.pack};`);
      } else {
        body.push(`  Napi::Object __out = Napi::Object::New(env);`);
        for (const o of outs) body.push(`  __out.Set("${o.name}", ${o.pack});`);
        body.push(`  return __out;`);
      }
    } else if (isVoid) {
      if (outs.length === 0) {
        body.push(`  return env.Undefined();`);
      } else if (outs.length === 1) {
        body.push(`  return ${outs[0]!.pack};`);
      } else {
        body.push(`  Napi::Object __out = Napi::Object::New(env);`);
        for (const o of outs) body.push(`  __out.Set("${o.name}", ${o.pack});`);
        body.push(`  return __out;`);
      }
    } else if (returnsStr) {
      body.push(`  return Napi::String::New(env, __ret ? __ret : "");`);
    } else if (returnsInt) {
      body.push(`  return Napi::Number::New(env, __ret);`);
    } else if (returnsDouble) {
      body.push(`  return Napi::Number::New(env, __ret);`);
    } else if (returnsStruct) {
      body.push(`  Napi::Object __out = Napi::Object::New(env);`);
      const t = resolveType(fn.return_type);
      if (t && t.kind === 'struct') {
        body.push(...structFieldPack(fn.return_type, '__ret', '__out', '  '));
      }
      body.push(`  return __out;`);
    } else if (fn.return_type === 'const occtl_error_t *') {
      body.push(`  Napi::Object __out = Napi::Object::New(env);`);
      body.push(`  if (__ret) {`);
      body.push(`    __out.Set("status", Napi::Number::New(env, (int)__ret->status));`);
      body.push(`    __out.Set("message", Napi::String::New(env, __ret->message ? __ret->message : ""));`);
      body.push(`    __out.Set("source", Napi::BigInt::New(env, (uint64_t)__ret->source.bits));`);
      body.push(`    __out.Set("extended", Napi::Number::New(env, __ret->extended));`);
      body.push(`  }`);
      body.push(`  return __out;`);
    } else {
      body.push(`  return env.Undefined();`);
    }
  }

  const cc = `static Napi::Value Call_${stripPrefix(fn.name)}(Napi::Env env_ignored, const Napi::Array& args, const Napi::CallbackInfo& info) {\n${body.join('\n')}\n}\n`;
  return { cc, category };
}

function emitUnsupportedTrampoline(fn: AbiFn, reason: string): string {
  return `static Napi::Value Call_${stripPrefix(fn.name)}(Napi::Env env_ignored, const Napi::Array& args, const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  (void)args;
  Napi::Error e = Napi::Error::New(env, std::string("${fn.name}: not yet supported by the Node trampoline generator — ${reason.replace(/"/g, '\\"').replace(/\n/g, ' ')}"));
  e.Set("name", Napi::String::New(env, "UnsupportedError"));
  e.Set("status", Napi::Number::New(env, (int)OCCTL_UNSUPPORTED));
  e.Set("source", Napi::BigInt::New(env, (uint64_t)0));
  e.Set("extended", Napi::Number::New(env, 0));
  e.ThrowAsJavaScriptException();
  return env.Undefined();
}
`;
}

// Load overrides from disk.
function loadOverride(fnName: string): string | null {
  const path = join(overridesDir, `${stripPrefix(fnName)}.cc.tpl`);
  if (existsSync(path)) return readFileSync(path, 'utf-8');
  return null;
}

function wrapOverride(fn: AbiFn, override: string): string {
  // Use cb_info to avoid colliding with `info` locals declared inside overrides.
  return `static Napi::Value Call_${stripPrefix(fn.name)}(Napi::Env env_ignored, const Napi::Array& args, const Napi::CallbackInfo& cb_info) {
  Napi::Env env = cb_info.Env();
${override}
}
`;
}

// -------- src/native/raw.cc -------------------------------------------------

const rawCc: string[] = [];
rawCc.push(`// Copyright (c) 2026 Capgemini Engineering Research and Development.`);
rawCc.push(`//`);
rawCc.push(`// This file is part of OCCT-Light software library.`);
rawCc.push(`//`);
rawCc.push(`// This library is free software; you can redistribute it and/or modify it under`);
rawCc.push(`// the terms of the GNU Affero General Public License version 3 as published`);
rawCc.push(`// by the Free Software Foundation, with an option to use any later version.`);
rawCc.push(`// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution`);
rawCc.push(`// for complete text of the license and disclaimer of any warranty.`);
rawCc.push(`//`);
rawCc.push(`// Alternatively, this file may be used under the terms of a commercial`);
rawCc.push(`// license or contractual agreement.`);
rawCc.push(`// SPDX-License-Identifier: AGPL-3.0-or-later`);
rawCc.push(`// AUTOGENERATED by bindings/node/tools/generate_facade.ts — do not edit.`);
rawCc.push(`// Source: ${abiPath}`);
rawCc.push(`// Functions: ${abi.functions.length}`);
rawCc.push(`#include <napi.h>`);
rawCc.push(`#include <cstdlib>`);
rawCc.push(`#include <cstdint>`);
rawCc.push(`#include <cstring>`);
rawCc.push(`#include <string>`);
rawCc.push(`#include <vector>`);
rawCc.push(`#include <functional>`);
rawCc.push(`#include <unordered_map>`);
rawCc.push(``);
rawCc.push(`extern "C" {`);
for (const h of abi.headers) rawCc.push(`#include <occtl/${h}>`);
rawCc.push(`}`);
rawCc.push(``);
rawCc.push(`#include "handle_wrapper.h"`);
rawCc.push(``);
rawCc.push(`namespace occtl_node {`);
rawCc.push(``);
rawCc.push(`void ThrowFromStatus(Napi::Env env, occtl_status_t status);`);
rawCc.push(``);
rawCc.push(`// Per-function trampolines. Each consumes a JS Array of args and returns a JS Value.`);
rawCc.push(``);

// Stats
const categoryCount: Record<string, number> = {};
const overrideNames: string[] = [];
const unsupportedFns: Array<{ name: string; reason: string }> = [];

for (const fn of abi.functions) {
  const ovr = loadOverride(fn.name);
  if (ovr) {
    rawCc.push(wrapOverride(fn, ovr));
    categoryCount['override'] = (categoryCount['override'] ?? 0) + 1;
    overrideNames.push(stripPrefix(fn.name));
    continue;
  }
  const r = generateTrampoline(fn);
  rawCc.push(r.cc);
  categoryCount[r.category] = (categoryCount[r.category] ?? 0) + 1;
  if (r.unsupported) unsupportedFns.push({ name: fn.name, reason: r.unsupported });
}

// Dispatch table.
rawCc.push(``);
rawCc.push(`using TrampolineFn = Napi::Value (*)(Napi::Env, const Napi::Array&, const Napi::CallbackInfo&);`);
rawCc.push(``);
rawCc.push(`static const std::unordered_map<std::string, TrampolineFn>& DispatchTable() {`);
rawCc.push(`  static const std::unordered_map<std::string, TrampolineFn> kTable = {`);
for (const fn of abi.functions) {
  rawCc.push(`    {"${stripPrefix(fn.name)}", Call_${stripPrefix(fn.name)}},`);
}
rawCc.push(`  };`);
rawCc.push(`  return kTable;`);
rawCc.push(`}`);
rawCc.push(``);
rawCc.push(`static const char* const kExportedFunctions[] = {`);
for (const fn of abi.functions) {
  rawCc.push(`  "${stripPrefix(fn.name)}",`);
}
rawCc.push(`  nullptr`);
rawCc.push(`};`);
rawCc.push(``);
rawCc.push(`Napi::Array ExportedFunctionsArray(Napi::Env env) {`);
rawCc.push(`  Napi::Array out = Napi::Array::New(env);`);
rawCc.push(`  uint32_t i = 0;`);
rawCc.push(`  for (const char* const* p = kExportedFunctions; *p; ++p, ++i) {`);
rawCc.push(`    out.Set(i, Napi::String::New(env, *p));`);
rawCc.push(`  }`);
rawCc.push(`  return out;`);
rawCc.push(`}`);
rawCc.push(``);
rawCc.push(`Napi::Value RawCall(const Napi::CallbackInfo& info) {`);
rawCc.push(`  Napi::Env env = info.Env();`);
rawCc.push(`  if (info.Length() < 1 || !info[0].IsString()) {`);
rawCc.push(`    Napi::TypeError::New(env, "raw.call: first arg must be a function name string").ThrowAsJavaScriptException();`);
rawCc.push(`    return env.Undefined();`);
rawCc.push(`  }`);
rawCc.push(`  std::string name = info[0].As<Napi::String>().Utf8Value();`);
rawCc.push(`  Napi::Array args = info.Length() > 1 && info[1].IsArray()`);
rawCc.push(`    ? info[1].As<Napi::Array>()`);
rawCc.push(`    : Napi::Array::New(env);`);
rawCc.push(``);
rawCc.push(`  const auto& table = DispatchTable();`);
rawCc.push(`  auto it = table.find(name);`);
rawCc.push(`  if (it == table.end()) {`);
rawCc.push(`    Napi::Error e = Napi::Error::New(env, std::string("occtl raw.call: unknown function name '") + name + "'");`);
rawCc.push(`    e.Set("name", Napi::String::New(env, "UnsupportedError"));`);
rawCc.push(`    e.Set("status", Napi::Number::New(env, (int)OCCTL_UNSUPPORTED));`);
rawCc.push(`    e.Set("source", Napi::BigInt::New(env, (uint64_t)0));`);
rawCc.push(`    e.Set("extended", Napi::Number::New(env, 0));`);
rawCc.push(`    e.ThrowAsJavaScriptException();`);
rawCc.push(`    return env.Undefined();`);
rawCc.push(`  }`);
rawCc.push(`  return it->second(env, args, info);`);
rawCc.push(`}`);
rawCc.push(``);
rawCc.push(`} // namespace occtl_node`);
rawCc.push(``);

writeFileSync(join(NATIVE_DIR, 'raw.cc'), rawCc.join('\n'));
stdout.write(`generate_facade: wrote ${join(NATIVE_DIR, 'raw.cc')}\n`);

// -------- src/ts/generated/_raw.gen.ts --------------------------------------

const rawGen: string[] = [];
rawGen.push(`// Copyright (c) 2026 Capgemini Engineering Research and Development.`);
rawGen.push(`//`);
rawGen.push(`// This file is part of OCCT-Light software library.`);
rawGen.push(`//`);
rawGen.push(`// This library is free software; you can redistribute it and/or modify it under`);
rawGen.push(`// the terms of the GNU Affero General Public License version 3 as published`);
rawGen.push(`// by the Free Software Foundation, with an option to use any later version.`);
rawGen.push(`// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution`);
rawGen.push(`// for complete text of the license and disclaimer of any warranty.`);
rawGen.push(`//`);
rawGen.push(`// Alternatively, this file may be used under the terms of a commercial`);
rawGen.push(`// license or contractual agreement.`);
rawGen.push(`// SPDX-License-Identifier: AGPL-3.0-or-later`);
rawGen.push(`// AUTOGENERATED by bindings/node/tools/generate_facade.ts — do not edit.`);
rawGen.push(``);
rawGen.push(`export const EXPORTED_FUNCTIONS: readonly string[] = Object.freeze([`);
for (const fn of abi.functions) rawGen.push(`  '${stripPrefix(fn.name)}',`);
rawGen.push(`]);`);
rawGen.push(``);
rawGen.push(`export const ABI_VERSION = ${abi.abi_version};`);
rawGen.push(``);
writeFileSync(join(GEN_DIR, '_raw.gen.ts'), rawGen.join('\n'));
stdout.write(`generate_facade: wrote ${join(GEN_DIR, '_raw.gen.ts')}\n`);

// -------- src/ts/generated/<module>.ts --------------------------------------
//
// TS signatures: still pass-through but typed against the call() dispatcher.
// Each function takes ...args and returns the addon call result. We can do a
// little better than `unknown` for common shapes — but the goal here is "no
// regressions" so we keep arg/return types generic.

const moduleOut: Record<string, string[]> = {};
for (const m of ['core', 'geom', 'topo', 'prim', 'text', 'bool_', 'mesh', 'de', 'io_brep', 'io_step', 'io_iges', 'io_stl', 'io_obj', 'io_gltf', 'io_vrml', 'io_ply', 'heal', 'viz']) {
  moduleOut[m] = [
    `// Copyright (c) 2026 Capgemini Engineering Research and Development.`,
    `//`,
    `// This file is part of OCCT-Light software library.`,
    `//`,
    `// This library is free software; you can redistribute it and/or modify it under`,
    `// the terms of the GNU Affero General Public License version 3 as published`,
    `// by the Free Software Foundation, with an option to use any later version.`,
    `// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution`,
    `// for complete text of the license and disclaimer of any warranty.`,
    `//`,
    `// Alternatively, this file may be used under the terms of a commercial`,
    `// license or contractual agreement.`,
    `// SPDX-License-Identifier: AGPL-3.0-or-later`,
    `// AUTOGENERATED by bindings/node/tools/generate_facade.ts — do not edit.`,
    `// Module: ${m}`,
    ``,
    m === 'prim'
      ? `import { loadAddon, type NativeGraph } from '../_raw.js';`
      : `import { loadAddon } from '../_raw.js';`,
    ...(m === 'prim' ? [`import { type NodeId, nodeId } from '../ids.js';`] : []),
    ``,
    `// Each function below mirrors one OCCTL_API entry. They route through the addon's`,
    `// generic .call() dispatcher — see bindings/node/src/native/raw.cc for the per-function`,
    `// unmarshalling.`,
    ``,
  ];
}

for (const fn of abi.functions) {
  const m = moduleFor(fn);
  const tsName = tsFnName(fn.name);
  const key    = stripPrefix(fn.name);

  // Build JSDoc block.
  const jsdocLines: string[] = [];
  const summary = (fn.doc || '').split('\n')[0]?.trim() ?? '';
  jsdocLines.push(`/**`);
  if (summary) {
    jsdocLines.push(` * ${summary}`);
    jsdocLines.push(` *`);
  }
  // @param docs.
  for (const p of fn.params) {
    const pdoc = (p.doc || '').replace(/\n/g, ' ').trim();
    if (pdoc) {
      const dirLabel = p.direction === 'out' ? '[out] ' : p.direction === 'inout' ? '[inout] ' : '';
      jsdocLines.push(` * @param ${camel(p.name)} - ${dirLabel}${pdoc}`);
    }
  }
  // @returns / @throws from retvals.
  const returnsStatus = fn.return_type === 'occtl_status_t';
  if (fn.retvals && fn.retvals.length > 0) {
    for (const rv of fn.retvals) {
      if (returnsStatus && rv.code !== 'OCCTL_OK') {
        jsdocLines.push(` * @throws {Error} ${rv.code} — ${rv.doc.replace(/\n/g, ' ').trim()}`);
      } else if (!returnsStatus) {
        jsdocLines.push(` * @returns ${rv.code} — ${rv.doc.replace(/\n/g, ' ').trim()}`);
      }
    }
  }
  // @threadsafe
  if (fn.threadsafe) {
    jsdocLines.push(` * @threadsafe ${fn.threadsafe.replace(/\n/g, ' ').trim()}`);
  }
  // @see
  if (fn.see_also && fn.see_also.length > 0) {
    for (const sa of fn.see_also) {
      jsdocLines.push(` * @see ${sa}`);
    }
  }
  jsdocLines.push(` */`);
  moduleOut[m]!.push(jsdocLines.join('\n'));

  moduleOut[m]!.push(`export function ${tsName}(...args: unknown[]): unknown {`);
  moduleOut[m]!.push(`  return loadAddon().call('${key}', args);`);
  moduleOut[m]!.push(`}`);
  moduleOut[m]!.push(``);
}

function appendTypedPrimFacade(out: string[]): void {
  const structs = new Map<string, AbiType>();
  for (const t of abi.types) {
    const stem = publicPrimName(t.name);
    if (stem && t.kind === 'struct') structs.set(stem, t);
  }
  out.push(`// Typed primitive facade generated from occtl_prim_*_info_t metadata.`);
  out.push(``);
  for (const stem of PRIM_TYPED_ORDER) {
    const struct = structs.get(stem);
    const iface = PRIM_TYPED_NAMES[stem];
    if (!struct || !iface) continue;
    const fields = (struct.fields ?? []).filter(f => f.name !== 'struct_version' && f.name !== 'p_next');
    out.push(`/** Typed options for ${struct.name}. */`);
    out.push(`export interface ${iface} {`);
    for (const field of fields) {
      const name = camel(field.name);
      const doc = cleanDoc(field.doc);
      const optional = PRIM_REQUIRED_FIELDS[stem]!.includes(field.name) ? '' : '?';
      if (doc) out.push(`  /** ${doc} */`);
      if (field.name === 'placement') {
        out.push(`  ${name}?: { location: { x: number; y: number; z: number }; x_dir: { x: number; y: number; z: number }; x_dir_ref: { x: number; y: number; z: number } };`);
      } else {
        out.push(`  ${name}${optional}: number;`);
      }
    }
    out.push(`}`);
    out.push(``);
  }
  for (const stem of PRIM_TYPED_ORDER) {
    const struct = structs.get(stem);
    const iface = PRIM_TYPED_NAMES[stem];
    if (!struct || !iface) continue;
    const method = `make${pascal(stem)}`;
    const rawName = `prim_make_${stem}`;
    const union = stem === 'sphere' ? `${iface} | number` : iface;
    out.push(`/** Build a ${stem} and return the NodeId of the new topology root. */`);
    out.push(`export function ${method}(graph: NativeGraph, info: ${union}): NodeId {`);
    if (stem === 'sphere') {
      out.push(`  const options = typeof info === 'number' ? { radius: info } : info;`);
      out.push(`  return nodeId(loadAddon().call('${rawName}', [graph, options]) as bigint);`);
    } else {
      out.push(`  return nodeId(loadAddon().call('${rawName}', [graph, info]) as bigint);`);
    }
    out.push(`}`);
    out.push(``);
  }
}

appendTypedPrimFacade(moduleOut.prim!);

moduleOut.core!.push(`export const __EXPORTED_BY_MODULE_CORE: readonly string[] = Object.freeze([`);
for (const fn of abi.functions) {
  if (moduleFor(fn) === 'core') moduleOut.core!.push(`  '${stripPrefix(fn.name)}',`);
}
moduleOut.core!.push(`]);`);
moduleOut.core!.push(``);

for (const m of Object.keys(moduleOut)) {
  if (HAND_WRITTEN_MODULES.has(m)) {
    stdout.write(`generate_facade: left ${m}.ts unchanged (hand-written facade)\n`);
    continue;
  }
  const path = join(GEN_DIR, `${m}.ts`);
  writeFileSync(path, moduleOut[m]!.join('\n'));
  stdout.write(`generate_facade: wrote ${path}\n`);
}

// -------- stats -------------------------------------------------------------
stdout.write(`\ngenerate_facade: emitted ${abi.functions.length} trampolines across ${Object.keys(moduleOut).length} modules.\n`);
stdout.write(`  by category:\n`);
for (const [k, v] of Object.entries(categoryCount).sort((a, b) => b[1] - a[1])) {
  stdout.write(`    ${k}: ${v}\n`);
}
stdout.write(`  overrides (${overrideNames.length}): ${overrideNames.join(', ')}\n`);
if (unsupportedFns.length) {
  stdout.write(`  unsupported by generator (${unsupportedFns.length}):\n`);
  for (const u of unsupportedFns.slice(0, 30)) {
    stdout.write(`    ${u.name}: ${u.reason}\n`);
  }
  if (unsupportedFns.length > 30) stdout.write(`    ... and ${unsupportedFns.length - 30} more\n`);
}
