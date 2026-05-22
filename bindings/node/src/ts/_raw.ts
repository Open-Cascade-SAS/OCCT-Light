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

import { Status, UnsupportedError } from './errors.js';

export interface NativeError {
  /** Primary status code — one of the {@link ../errors.ts | Status} enum values. */
  status:   number;
  /** Human-readable diagnostic message in UTF-8, or empty string. */
  message:  string;
  /** UID of the offending entity, or 0n when not applicable. */
  source:   bigint;   // occtl_uid_t.bits
  /** Driver/module-specific secondary code; 0 means none.  See {@link ../../../docs/design/ABI_PATTERNS.md | ABI_PATTERNS.md §10.4}. */
  extended: number;
}

/** Opaque addon-side handle wrapper. The ObjectWrap subclasses share this shape. */
export interface NativeHandle {
  /** Frees the underlying C handle. Idempotent; sets the pointer to NULL. */
  close(): void;
  /** True after close() has been called. */
  readonly disposed: boolean;
}

/** Iterator handle backing TS Generator<NodeId>. */
export interface NativeNodeIter extends NativeHandle {
  /** Returns the next bits, or null on OCCTL_NOT_FOUND. */
  next(): bigint | null;
}

export type NativeGraph    = NativeHandle;
export type NativeBatch    = NativeHandle;
export type NativeExplorer = NativeNodeIter;
export type NativeRelated  = NativeNodeIter;

/** Shape exported by the compiled `occtl_node.node` addon. The generator emits the
 *  bulk of this surface; here we hand-write the §4 contract entry points. */
export interface OcctlAddon {
  // §4.1 ABI handshake
  runtimeAbiVersion(): number;
  runtimeInit(): void;
  errorLast(): NativeError;
  errorClear(): void;
  statusToString(status: number): string;

  // §4.3 Handles — constructors invoked by lib/handles.ts.
  Graph: new () => NativeGraph;

  // §4.6 Iterators
  graphFaceIterCreate(graph: NativeGraph): NativeNodeIter;
  graphEdgeIterCreate(graph: NativeGraph): NativeNodeIter;
  graphVertexIterCreate(graph: NativeGraph): NativeNodeIter;
  graphSolidIterCreate(graph: NativeGraph): NativeNodeIter;
  nodeIterNext(it: NativeNodeIter): bigint | null;
  nodeIterFree(it: NativeNodeIter): void;

  // Bulk-generated calls — keyed by function name (snake-case, no `occtl_` prefix).
  // The generator emits exactly one entry per OCCTL_API function.
  call(name: string, args: unknown[]): unknown;

  // Surface metadata (generator-baked)
  readonly ABI_VERSION: number;
  readonly EXPORTED_FUNCTIONS: ReadonlyArray<string>;
}

// Load via bindings(). Returns the addon object or throws if it can't be loaded.
import { createRequire } from 'node:module';
const requireBindings = createRequire(import.meta.url);

// eslint-disable-next-line @typescript-eslint/no-explicit-any
let _addon: OcctlAddon | null = null;
let _loadError: Error | null = null;

export function loadAddon(): OcctlAddon {
  if (_addon) return _addon;
  if (_loadError) throw _loadError;
  try {
    const bindings = requireBindings('bindings') as (opts: { bindings: string }) => OcctlAddon;
    const addon = bindings({ bindings: 'occtl_node' });
    const exported = new Set<string>();
    for (const fn of addon.EXPORTED_FUNCTIONS ?? []) {
      const value = String(fn);
      exported.add(value);
      if (value.startsWith('occtl_')) {
        exported.add(value.slice('occtl_'.length));
      }
    }
    const rawCall = addon.call.bind(addon);
    addon.call = (name: string, args: unknown[]): unknown => {
      if (exported.size > 0 && !exported.has(name) && !exported.has(`occtl_${name}`)) {
        throw new UnsupportedError({
          status: Status.Unsupported,
          message: `Feature is not available in this OCCT-Light build (missing occtl_${name})`,
          source: 0n as never,
          extended: 0,
        });
      }
      return rawCall(name, args);
    };
    _addon = addon;
    return _addon!;
  } catch (e) {
    _loadError = e instanceof Error ? e : new Error(String(e));
    throw _loadError;
  }
}

/** Test hook — replace the addon with a mock. */
export function __setAddonForTesting(addon: OcctlAddon | null): void {
  _addon = addon;
  _loadError = null;
}
