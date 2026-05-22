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
import { InvalidHandleError } from "./errors.js";
import { Uid } from "./ids.js";

const FINALIZERS_ENABLED =
  typeof FinalizationRegistry !== "undefined";

type FreeFn = (mod: RawModule, ptr: number) => void;

interface FinalizerPayload {
  ptr: number;
  free: FreeFn;
  module: RawModule;
  type: string;
}

const registry: FinalizationRegistry<FinalizerPayload> | null =
  FINALIZERS_ENABLED
    ? new FinalizationRegistry<FinalizerPayload>((p) => {
        try {
          if (p.ptr !== 0) p.free(p.module, p.ptr);
        } catch {
          // Finalizers must not throw. Swallow & log via console.warn.
          // eslint-disable-next-line no-console
          console.warn(`occtl: finalizer for ${p.type} threw; handle leak likely`);
        }
      })
    : null;

/**
 * Base class for every opaque handle wrapper.
 *
 * Provides:
 *   - `pointer` getter (validates not-disposed)
 *   - explicit `.dispose()` / `[Symbol.dispose]()`
 *   - FinalizationRegistry-based safety net
 */
export abstract class Disposable {
  protected _ptr: number;
  protected readonly _module: RawModule;
  protected readonly _typeName: string;
  protected readonly _free: FreeFn;

  constructor(module: RawModule, ptr: number, typeName: string, free: FreeFn) {
    if (ptr === 0) throw new InvalidHandleError(`null pointer for ${typeName}`, Uid.invalid(), 0);
    this._module = module;
    this._ptr = ptr;
    this._typeName = typeName;
    this._free = free;
    if (registry) registry.register(this, { ptr, free, module, type: typeName }, this);
  }

  get pointer(): number {
    if (this._ptr === 0) throw new InvalidHandleError(`${this._typeName} already disposed`, Uid.invalid(), 0);
    return this._ptr;
  }

  get disposed(): boolean {
    return this._ptr === 0;
  }

  dispose(): void {
    if (this._ptr === 0) return;
    if (registry) registry.unregister(this);
    const p = this._ptr;
    this._ptr = 0;
    this._free(this._module, p);
  }

  [Symbol.dispose](): void {
    this.dispose();
  }

  /** Alias used by libraries expecting close() / IDisposable conventions. */
  close(): void {
    this.dispose();
  }
}
