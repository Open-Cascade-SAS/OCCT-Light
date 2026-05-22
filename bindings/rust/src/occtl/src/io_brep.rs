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

use crate::error::{check, Result};
use crate::id::NodeId;
use crate::topo::Graph;
use occtl_sys as sys;
use std::ffi::CString;

/// Mirror of `occtl_io_brep_write_options_t`.
/// The output format (binary / ASCII) is managed internally by the OCCT
/// `DEBREP_Provider`.
#[derive(Copy, Clone, Debug)]
pub struct BrepWriteOptions {
    pub write_triangulation: bool,
}

impl Default for BrepWriteOptions {
    fn default() -> Self {
        Self {
            write_triangulation: true,
        }
    }
}

impl BrepWriteOptions {
    fn to_c(self) -> sys::occtl_io_brep_write_options_t {
        let mut o: sys::occtl_io_brep_write_options_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_io_brep_write_options_init(&mut o) };
        o.write_triangulation = self.write_triangulation as i32;
        o
    }
}

/// Reads a BRep file and returns the new graph + root NodeId.
pub fn read(path: &str) -> Result<(Graph, NodeId)> {
    crate::init()?;
    let cpath = CString::new(path).expect("path contains null byte");
    let mut graph: *mut sys::occtl_graph_t = std::ptr::null_mut();
    let mut root: sys::occtl_node_id_t = Default::default();
    check(unsafe { sys::occtl_io_brep_read(cpath.as_ptr(), &mut graph, &mut root) })?;
    Ok((unsafe { Graph::from_raw(graph) }, root.into()))
}

/// Writes the topology rooted at `root` to a BRep file.
pub fn write(graph: &Graph, root: NodeId, path: &str, options: BrepWriteOptions) -> Result<()> {
    let cpath = CString::new(path).expect("path contains null byte");
    let opts = options.to_c();
    check(unsafe { sys::occtl_io_brep_write(graph.as_ptr(), root.into(), cpath.as_ptr(), &opts) })
}
