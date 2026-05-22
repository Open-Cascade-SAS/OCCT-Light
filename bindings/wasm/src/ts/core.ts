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

import type { LoadedOcctl } from "./abi.js";
import { FUNCTION_NAMES_CORE, bindRawCore } from "./generated/core.js";

export { FUNCTION_NAMES_CORE };
export { Status, OcctLError, AbiMismatchError } from "./errors.js";

export class CoreModule {
  private readonly _raw: ReturnType<typeof bindRawCore>;
  private readonly _occtl: LoadedOcctl;

  constructor(occtl: LoadedOcctl) {
    this._occtl = occtl;
    this._raw = bindRawCore(occtl.raw);
  }

  /** Returns the runtime's reported ABI version. */
  abiVersion(): number {
    return this._raw.occtlRuntimeAbiVersion();
  }

  /** Returns the runtime library version as { major, minor, patch }. */
  version(): { major: number; minor: number; patch: number } {
    // occtl_runtime_version takes three out int* parameters and returns void.
    const mod = this._occtl.raw;
    const buf = mod._malloc(12);
    try {
      this._raw.occtlRuntimeVersion(buf, buf + 4, buf + 8);
      return {
        major: mod.getValue(buf, "i32"),
        minor: mod.getValue(buf + 4, "i32"),
        patch: mod.getValue(buf + 8, "i32"),
      };
    } finally {
      mod._free(buf);
    }
  }

  /** Translates a status code to a human-readable string. */
  statusToString(status: number): string {
    const ptr = this._raw.occtlStatusToString(status);
    return ptr === 0 ? "" : this._occtl.raw.UTF8ToString(ptr);
  }

  /** Clears the thread-local error slot. */
  clearError(): void {
    this._raw.occtlErrorClear();
  }

  /** Exposes the raw bindings for advanced users. Private surface. */
  get raw(): ReturnType<typeof bindRawCore> {
    return this._raw;
  }
}
