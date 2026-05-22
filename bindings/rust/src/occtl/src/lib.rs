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

#![doc = "Idiomatic Rust bindings for OCCT-Light."]

#[cfg(all(occtl_has_topo, occtl_has_bool))]
pub mod bool_;
pub mod error;
pub mod id;
#[cfg(all(occtl_has_topo, occtl_has_io_brep))]
pub mod io_brep;
#[cfg(all(occtl_has_topo, occtl_has_prim))]
pub mod prim;
#[cfg(occtl_has_topo)]
pub mod topo;

pub use error::{Error, Result, Status};
#[cfg(occtl_has_topo)]
pub use id::{NodeId, NodeKind, RefId, RefUid, RepUid};
pub use id::{RepId, Uid};
#[cfg(all(occtl_has_topo, occtl_has_io_brep))]
pub use io_brep::BrepWriteOptions;
#[cfg(all(occtl_has_topo, occtl_has_prim))]
pub use prim::{BoxInfo, ConeInfo, CylinderInfo, SphereInfo, TorusInfo, WedgeInfo};
#[cfg(occtl_has_topo)]
pub use topo::Graph;

/// Re-export of the raw FFI for callers who need to drop down to the C ABI.
pub use occtl_sys as sys;

/// Returns the runtime ABI version compiled into the loaded library.
pub fn abi_version() -> u32 {
    unsafe { occtl_sys::occtl_runtime_abi_version() }
}

/// Initialises the runtime once per process; subsequent calls are no-ops.
/// Returns the ABI version on success.
pub fn init() -> Result<u32> {
    use std::sync::Once;
    static ONCE: Once = Once::new();
    let mut info: occtl_sys::occtl_runtime_init_info_t = unsafe { std::mem::zeroed() };
    unsafe { occtl_sys::occtl_runtime_init_info_init(&mut info) };
    ONCE.call_once(|| unsafe {
        // First call wins; later calls would return INVALID_ARGUMENT
        // (matching the documented "already initialised" contract) which
        // we silently swallow.
        let _ = occtl_sys::occtl_runtime_init(&info);
    });
    Ok(abi_version())
}
