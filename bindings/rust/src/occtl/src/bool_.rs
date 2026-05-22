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

/// Options for boolean operations.  Mirrors `occtl_bool_options_t`.
#[derive(Copy, Clone, Debug)]
pub struct BoolOptions {
    pub fuzzy_value: f64,
    pub run_parallel: bool,
    pub simplify_result: bool,
    pub simplify_angular_tolerance: f64,
    pub build_history: bool,
}

impl Default for BoolOptions {
    fn default() -> Self {
        let mut o: sys::occtl_bool_options_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_bool_options_init(&mut o) };
        Self {
            fuzzy_value: o.fuzzy_value,
            run_parallel: o.run_parallel != 0,
            simplify_result: o.simplify_result != 0,
            simplify_angular_tolerance: o.simplify_angular_tolerance,
            build_history: o.build_history != 0,
        }
    }
}

impl BoolOptions {
    fn to_c(self) -> sys::occtl_bool_options_t {
        let mut o: sys::occtl_bool_options_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_bool_options_init(&mut o) };
        o.fuzzy_value = self.fuzzy_value;
        o.run_parallel = self.run_parallel as i32;
        o.simplify_result = self.simplify_result as i32;
        o.simplify_angular_tolerance = self.simplify_angular_tolerance;
        o.build_history = self.build_history as i32;
        o
    }
}

type BoolFn = unsafe extern "C" fn(
    *mut sys::occtl_graph_t,
    *const sys::occtl_node_id_t,
    usize,
    *const sys::occtl_node_id_t,
    usize,
    *const sys::occtl_bool_options_t,
    *mut sys::occtl_node_id_t,
) -> sys::occtl_status_t;

fn run_op(
    op: BoolFn,
    graph: &Graph,
    objects: &[NodeId],
    tools: &[NodeId],
    options: BoolOptions,
) -> Result<NodeId> {
    let opts = options.to_c();
    let mut root: sys::occtl_node_id_t = Default::default();
    let obj_ptr = objects.as_ptr() as *const sys::occtl_node_id_t;
    let tool_ptr = tools.as_ptr() as *const sys::occtl_node_id_t;
    check(unsafe {
        op(
            graph.as_ptr(),
            obj_ptr,
            objects.len(),
            tool_ptr,
            tools.len(),
            &opts,
            &mut root,
        )
    })?;
    Ok(root.into())
}

/// Boolean Fuse — union of `objects` and `tools`.
pub fn fuse(graph: &Graph, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
    fuse_with_options(graph, objects, tools, BoolOptions::default())
}

/// Boolean Fuse with explicit options.
pub fn fuse_with_options(
    graph: &Graph,
    objects: &[NodeId],
    tools: &[NodeId],
    options: BoolOptions,
) -> Result<NodeId> {
    run_op(sys::occtl_bool_fuse, graph, objects, tools, options)
}

/// Boolean Cut — `objects` minus `tools`.
pub fn cut(graph: &Graph, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
    cut_with_options(graph, objects, tools, BoolOptions::default())
}

/// Boolean Cut with explicit options.
pub fn cut_with_options(
    graph: &Graph,
    objects: &[NodeId],
    tools: &[NodeId],
    options: BoolOptions,
) -> Result<NodeId> {
    run_op(sys::occtl_bool_cut, graph, objects, tools, options)
}

/// Boolean Common — intersection.
pub fn common(graph: &Graph, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
    common_with_options(graph, objects, tools, BoolOptions::default())
}

/// Boolean Common with explicit options.
pub fn common_with_options(
    graph: &Graph,
    objects: &[NodeId],
    tools: &[NodeId],
    options: BoolOptions,
) -> Result<NodeId> {
    run_op(sys::occtl_bool_common, graph, objects, tools, options)
}

/// Boolean Section — intersection curve(s).
pub fn section(graph: &Graph, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
    section_with_options(graph, objects, tools, BoolOptions::default())
}

/// Boolean Section with explicit options.
pub fn section_with_options(
    graph: &Graph,
    objects: &[NodeId],
    tools: &[NodeId],
    options: BoolOptions,
) -> Result<NodeId> {
    run_op(sys::occtl_bool_section, graph, objects, tools, options)
}

/// Boolean Split — split objects by tools without merging.
pub fn split(graph: &Graph, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
    split_with_options(graph, objects, tools, BoolOptions::default())
}

/// Boolean Split with explicit options.
pub fn split_with_options(
    graph: &Graph,
    objects: &[NodeId],
    tools: &[NodeId],
    options: BoolOptions,
) -> Result<NodeId> {
    run_op(sys::occtl_bool_split, graph, objects, tools, options)
}
