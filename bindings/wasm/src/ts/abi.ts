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

import type { OcctlModuleFactory, RawModule } from "./generated/_raw.js";
import { ABI_VERSION } from "./generated/_manifest.js";
import { STRUCT_LAYOUTS } from "./generated/_layouts.js";
import { AbiMismatchError, Status, makeErrorForStatus } from "./errors.js";
import { Uid } from "./ids.js";
import { invalidateAllViews } from "./views.js";

/**
 * Public surface of a loaded OCCT-Light WASM module.
 *
 * Returned by `await Occtl.load()`. Stable across calls — call `load()`
 * once per worker/process.
 */
export interface LoadedOcctl {
  readonly raw: RawModule;
  readonly ABI_VERSION: number;
  runtimeAbiVersion(): number;
  runtimeShutdown(): void;
}

let cached: Promise<LoadedOcctl> | null = null;

/**
 * Locates and invokes the Emscripten module factory `occtl.js`. The factory
 * is itself an ES module export — its location depends on how the host
 * host bundle includes us. We try several conventional paths and surface a
 * clear error if none work.
 */
async function loadFactory(): Promise<OcctlModuleFactory> {
  // Candidate paths, tried in order. The dist/* one is canonical for the
  // packaged distribution; ./occtl.js works when the user bundles it
  // alongside lib/index.js.
  //
  // We use dynamic, string-variable imports so tsc does not try to resolve
  // these at compile time (occtl.js is a build artefact that may not exist
  // when the TS sources are checked).
  const candidates = ["./occtl.js", "../dist/occtl.js"];
  for (const path of candidates) {
    try {
      const importer = new Function("p", "return import(p)") as (p: string) => Promise<unknown>;
      const mod = await importer(path) as { default?: OcctlModuleFactory } & OcctlModuleFactory;
      return (mod.default ?? mod) as OcctlModuleFactory;
    } catch {
      // fallthrough to next candidate
    }
  }
  throw new Error(
    "Cannot locate occtl.js — the Emscripten factory was not found alongside " +
    "this module. Either run a WASM build (`emmake make`) so dist/occtl.js exists, " +
    "or pass a pre-loaded module to Occtl.fromRawModule().",
  );
}

/**
 * Wraps the loaded module so every public C call invalidates HeapView
 * snapshots before issuing the call. This is the conservative
 * heap-grow-safety policy described in BINDINGS.md §4.7.
 *
 * **HEAPF64-grow invalidation.** Emscripten may call `HEAPF64.grow()`
 * between any two WASM calls, which moves the underlying ArrayBuffer and
 * invalidates every live TypedArray view.  The `wrapModule*` functions
 * here run `invalidateAllViews()` before every `occtl_*` entry point so
 * the next view access always sees the current buffer.  See the
 * [HEAPF64-view README warning](../../README.md) and
 * [ABI_PATTERNS.md §4.7](../../../docs/design/ABI_PATTERNS.md).
 */
function wrapModuleForViewInvalidation(raw: RawModule): RawModule {
  return new Proxy(raw, {
    get(target, prop, receiver) {
      const name = String(prop);
      const value = Reflect.get(target, prop, receiver);
      if (value === undefined && (name.startsWith("occtl_") || name.startsWith("_occtl_"))) {
        return (): never => {
          throw makeErrorForStatus(
            Status.UNSUPPORTED,
            `Feature is not available in this OCCT-Light build (missing ${name})`,
            Uid.invalid(),
            0,
          );
        };
      }
      if (typeof value !== "function") return value;
      // Only public OCCTL_API entry points start with `occtl_` or `_occtl_`.
      // Heap accessor methods (UTF8ToString, _malloc, etc.) pass through.
      if (!name.startsWith("occtl_") && !name.startsWith("_occtl_")) {
        return value;
      }
      return function wrapped(this: unknown, ...args: unknown[]): unknown {
        invalidateAllViews();
        // Also invalidate before calls that allocate WASM memory.
        return (value as (...a: unknown[]) => unknown).apply(target, args);
      };
    },
  });
}

/**
 * Build a facade from a pre-loaded raw module. Useful for tests or when the
 * caller wants to provide its own Emscripten factory.
 */
export function fromRawModule(raw: RawModule): LoadedOcctl {
  const wrapped = wrapModuleForViewInvalidation(raw);

  // ABI handshake.
  const runtimeAbi = wrapped.occtl_runtime_abi_version();
  if (runtimeAbi !== ABI_VERSION) {
    throw new AbiMismatchError(ABI_VERSION, runtimeAbi);
  }

  // Initialise the runtime. Pass NULL — defaults.
  const status = wrapped.occtl_runtime_init(0);
  if (status !== Status.OK) {
    // Already initialised is OK across multiple load() calls on the same Module.
    // The C ABI returns OCCTL_INVALID_ARGUMENT for double-init; tolerate.
    if (status !== Status.INVALID_ARGUMENT) {
      const err = readLastError(wrapped);
      throw makeErrorForStatus(status, err.message, err.source, err.extended);
    }
  }

  return {
    raw: wrapped,
    ABI_VERSION,
    runtimeAbiVersion: () => wrapped.occtl_runtime_abi_version(),
    runtimeShutdown: () => { wrapped.occtl_runtime_shutdown(); },
  };
}

/**
 * Reads the thread-local last-error record. Pulls fields out of the
 * occtl_error_t struct using the auto-generated STRUCT_LAYOUTS table
 * (computed with wasm32 alignment rules from the ABI catalogue).
 */
export function readLastError(raw: RawModule): { status: number; message: string; source: Uid; extended: number } {
  const ptr = raw.occtl_error_last();
  if (ptr === 0) {
    return { status: 0, message: "", source: Uid.invalid(), extended: 0 };
  }
  const layout = STRUCT_LAYOUTS["occtl_error_t"];
  if (!layout) {
    // Fallback: hardcoded offsets as a last resort (should not happen).
    const status = raw.getValue(ptr + 0, "i32");
    const msgPtr = raw.getValue(ptr + 4, "i32");
    const message = msgPtr === 0 ? "" : raw.UTF8ToString(msgPtr);
    const lo = BigInt.asUintN(32, BigInt(raw.getValue(ptr + 8, "i32") >>> 0));
    const hi = BigInt.asUintN(32, BigInt(raw.getValue(ptr + 12, "i32") >>> 0));
    const source = Uid.fromBits((hi << 32n) | lo);
    const extended = raw.getValue(ptr + 16, "i32");
    return { status, message, source, extended };
  }
  const f = layout.fields;
  const status = raw.getValue(ptr + f.status.offset, "i32");
  const msgPtr = raw.getValue(ptr + f.message.offset, "i32");
  const message = msgPtr === 0 ? "" : raw.UTF8ToString(msgPtr);
  const lo = BigInt.asUintN(32, BigInt(raw.getValue(ptr + f.source.offset, "i32") >>> 0));
  const hi = BigInt.asUintN(32, BigInt(raw.getValue(ptr + f.source.offset + 4, "i32") >>> 0));
  const source = Uid.fromBits((hi << 32n) | lo);
  const extended = raw.getValue(ptr + f.extended.offset, "i32");
  return { status, message, source, extended };
}

/**
 * Translates a status code returned by a raw C call into a thrown JS
 * exception. Returns silently on Status.OK. The end-of-iteration
 * sentinel (Status.NOT_FOUND on iterator next) is the caller's
 * responsibility — this helper does not special-case it.
 */
export function check(raw: RawModule, status: number): void {
  if (status === Status.OK) return;
  const err = readLastError(raw);
  throw makeErrorForStatus(status, err.message || "(no message)", err.source, err.extended);
}

/**
 * Top-level loader. Async — Emscripten module instantiation is async.
 * Cached: subsequent calls return the same loaded module.
 */
export const Occtl = {
  /** Load (or return cached) OCCT-Light WASM module. */
  async load(opts: { factory?: OcctlModuleFactory } = {}): Promise<LoadedOcctl> {
    if (cached) return cached;
    cached = (async () => {
      const factory = opts.factory ?? (await loadFactory());
      const raw = await factory();
      return fromRawModule(raw);
    })();
    return cached;
  },

  /** Drop the cached instance — primarily for tests. */
  reset(): void {
    cached = null;
  },

  /** Construct a facade from an already-loaded RawModule (no async). */
  fromRawModule,

  /** Constant for ABI handshake matching. */
  ABI_VERSION,
};
