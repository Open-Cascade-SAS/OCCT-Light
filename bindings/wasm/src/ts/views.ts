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

import type { RawModule } from "./generated/_raw.js";
import { WasmHeapInvalidatedError } from "./errors.js";

type TypedArrayCtor<T> =
  | (new (buf: ArrayBufferLike, byteOffset: number, length: number) => T);

type HeapName = "HEAPU8" | "HEAPU32" | "HEAPF32" | "HEAPF64";

// We keep this set untyped at the generic-parameter level because views over
// different TypedArray kinds (Float64Array, Uint32Array, …) all need to be
// invalidated uniformly. Erasing to a base interface keeps the Set clean.
interface AnyHeapView { _invalidate(): void; }
const liveViews: Set<AnyHeapView> = new Set();

/** Called by every binding wrapper before it issues a C call that may allocate. */
export function invalidateAllViews(): void {
  for (const v of liveViews) v._invalidate();
  liveViews.clear();
}

/** Diagnostic only — number of currently-tracked views. */
export function liveViewCount(): number {
  return liveViews.size;
}

/**
 * A typed view into WASM linear memory. Lifetime: only until the next
 * OCCT-Light call. To use beyond that, call .copy() to materialise to the
 * JS heap.
 */
export class HeapView<T extends ArrayBufferView> {
  private _view: T | null;
  private readonly _ctor: TypedArrayCtor<T>;
  private readonly _module: RawModule;
  private readonly _heapName: HeapName;
  private readonly _ptr: number;
  private readonly _count: number;

  constructor(
    module: RawModule,
    heapName: HeapName,
    ctor: TypedArrayCtor<T>,
    ptr: number,
    count: number,
  ) {
    this._module = module;
    this._heapName = heapName;
    this._ctor = ctor;
    this._ptr = ptr;
    this._count = count;
    this._view = this._materialise();
    liveViews.add(this);
  }

  private _materialise(): T {
    const heap = this._module[this._heapName] as unknown as { buffer: ArrayBufferLike; BYTES_PER_ELEMENT: number };
    const bpe = heap.BYTES_PER_ELEMENT ?? 8;
    // ptr is a byte offset; convert to element offset for the TypedArray ctor.
    return new this._ctor(heap.buffer, this._ptr, this._count) as T;
  }

  /** Internal — invalidated by the global invalidator. */
  _invalidate(): void {
    this._view = null;
  }

  /** Returns the live view if still valid; throws otherwise. */
  get view(): T {
    if (this._view === null) throw new WasmHeapInvalidatedError("read");
    return this._view;
  }

  /** Copies into a fresh JS-heap-allocated TypedArray, decoupled from WASM memory. */
  copy(): T {
    const live = this.view;
    // Slice produces a new array with its own buffer.
    return (live as unknown as { slice(): T }).slice();
  }

  /** Element count. */
  get length(): number {
    return this._count;
  }

  /** Raw WASM pointer. Useful only for diagnostics; do not arithmetic on this. */
  get pointer(): number {
    return this._ptr;
  }

  /** True if the view is still backed by live memory. */
  get valid(): boolean {
    return this._view !== null;
  }
}

/** Construct a Float64Array view over `count` doubles at `ptr`. */
export function viewF64(mod: RawModule, ptr: number, count: number): HeapView<Float64Array> {
  return new HeapView<Float64Array>(mod, "HEAPF64", Float64Array, ptr, count);
}
/** Construct a Uint32Array view. */
export function viewU32(mod: RawModule, ptr: number, count: number): HeapView<Uint32Array> {
  return new HeapView<Uint32Array>(mod, "HEAPU32", Uint32Array, ptr, count);
}
/** Construct a Float32Array view. */
export function viewF32(mod: RawModule, ptr: number, count: number): HeapView<Float32Array> {
  return new HeapView<Float32Array>(mod, "HEAPF32", Float32Array, ptr, count);
}
/** Construct a Uint8Array view. */
export function viewU8(mod: RawModule, ptr: number, count: number): HeapView<Uint8Array> {
  return new HeapView<Uint8Array>(mod, "HEAPU8", Uint8Array, ptr, count);
}
