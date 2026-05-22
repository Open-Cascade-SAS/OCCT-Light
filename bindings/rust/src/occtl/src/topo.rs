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
use crate::id::{NodeId, RepId, RepUid, Uid};
use crate::prim::{self, BoxInfo, ConeInfo, CylinderInfo, SphereInfo, TorusInfo, WedgeInfo};
use occtl_sys as sys;
use std::marker::PhantomData;
use std::ptr;

/// Structured validation issue reported by [`Graph::check_issues`].
#[derive(Copy, Clone, Debug)]
pub struct CheckIssue {
    pub node_id: NodeId,
    pub context_node_id: NodeId,
    pub status_bit: u32,
    pub severity: sys::occtl_topo_check_severity_t,
}

/// Owning handle around `occtl_graph_t*`.
///
/// `Send` because the C ABI's read-only guarantees are per-graph;
/// `!Sync` because mutation is single-threaded.
pub struct Graph {
    ptr: *mut sys::occtl_graph_t,
}

// SAFETY: the C library is responsible for thread-affinity inside the graph.
// Moving an exclusive owner across threads is safe; sharing is not.
unsafe impl Send for Graph {}

impl Graph {
    /// Creates an empty graph.
    pub fn new() -> Result<Self> {
        crate::init()?;
        let mut ptr: *mut sys::occtl_graph_t = ptr::null_mut();
        check(unsafe { sys::occtl_graph_create(&mut ptr) })?;
        Ok(Self { ptr })
    }

    /// Creates an empty graph (naming-aligned alias for cross-language parity).
    pub fn create() -> Result<Self> {
        Self::new()
    }

    /// Wraps an existing raw handle (takes ownership).
    ///
    /// # Safety
    /// `ptr` must be the result of `occtl_graph_create` or a function that
    /// hands back ownership of an `occtl_graph_t*`.
    pub unsafe fn from_raw(ptr: *mut sys::occtl_graph_t) -> Self {
        Self { ptr }
    }

    /// Wraps an existing raw handle (takes ownership) and validates non-null.
    ///
    /// # Safety
    /// `ptr` must be an owned `occtl_graph_t*` returned by the C ABI.
    pub unsafe fn from_pointer_unsafe(ptr: *mut sys::occtl_graph_t) -> Result<Self> {
        if ptr.is_null() {
            return Err(crate::error::Error {
                status: sys::occtl_status::OCCTL_INVALID_HANDLE,
                message: "Graph::from_pointer_unsafe received a null pointer".to_string(),
            });
        }
        Ok(Self { ptr })
    }

    /// Borrows the underlying C pointer for direct ABI calls.
    #[inline]
    pub fn as_ptr(&self) -> *mut sys::occtl_graph_t {
        self.ptr
    }

    /// Returns the active solid count.
    pub fn solid_count(&self) -> usize {
        let mut count = 0usize;
        check(unsafe { sys::occtl_graph_solid_count(self.ptr, &mut count) }).expect("solid count");
        count
    }
    pub fn shell_count(&self) -> usize {
        let mut count = 0usize;
        check(unsafe { sys::occtl_graph_shell_count(self.ptr, &mut count) }).expect("shell count");
        count
    }
    pub fn face_count(&self) -> usize {
        let mut count = 0usize;
        check(unsafe { sys::occtl_graph_face_count(self.ptr, &mut count) }).expect("face count");
        count
    }
    pub fn wire_count(&self) -> usize {
        let mut count = 0usize;
        check(unsafe { sys::occtl_graph_wire_count(self.ptr, &mut count) }).expect("wire count");
        count
    }
    pub fn edge_count(&self) -> usize {
        let mut count = 0usize;
        check(unsafe { sys::occtl_graph_edge_count(self.ptr, &mut count) }).expect("edge count");
        count
    }
    pub fn vertex_count(&self) -> usize {
        let mut count = 0usize;
        check(unsafe { sys::occtl_graph_vertex_count(self.ptr, &mut count) })
            .expect("vertex count");
        count
    }
    pub fn compound_count(&self) -> usize {
        let mut count = 0usize;
        check(unsafe { sys::occtl_graph_compound_count(self.ptr, &mut count) })
            .expect("compound count");
        count
    }

    /// Runs graph validation and returns all reported issues.
    pub fn check_issues(&self) -> Result<Vec<CheckIssue>> {
        let mut count: usize = 0;
        check(unsafe { sys::occtl_topo_check(self.ptr, ptr::null_mut(), 0, &mut count) })?;
        if count == 0 {
            return Ok(Vec::new());
        }

        let mut native: Vec<sys::occtl_topo_check_issue_t> =
            vec![unsafe { std::mem::zeroed() }; count];
        check(unsafe {
            sys::occtl_topo_check(self.ptr, native.as_mut_ptr(), native.len(), &mut count)
        })?;
        native.truncate(count);
        Ok(native
            .into_iter()
            .map(|issue| CheckIssue {
                node_id: issue.node_id.into(),
                context_node_id: issue.context_node_id.into(),
                status_bit: issue.status_bit,
                severity: issue.severity,
            })
            .collect())
    }

    /// Returns true when graph validation reports no issues.
    pub fn is_valid(&self) -> Result<bool> {
        Ok(self.check_issues()?.is_empty())
    }

    /// Returns the kind of a node id.
    pub fn node_kind(&self, id: NodeId) -> Result<sys::occtl_node_kind_t> {
        let mut kind: sys::occtl_node_kind_t = sys::occtl_node_kind::OCCTL_KIND_INVALID;
        check(unsafe { sys::occtl_graph_node_kind(self.ptr, id.into(), &mut kind) })?;
        Ok(kind)
    }

    /// Returns the persistent UID for a node.
    pub fn uid_of(&self, id: NodeId) -> Result<Uid> {
        let mut uid: sys::occtl_uid_t = Default::default();
        check(unsafe { sys::occtl_graph_uid_from_node_id(self.ptr, id.into(), &mut uid) })?;
        Ok(uid.into())
    }

    pub fn rep_uid_of(&self, id: RepId) -> Result<RepUid> {
        let mut uid: sys::occtl_rep_uid_t = Default::default();
        check(unsafe { sys::occtl_graph_rep_uid_from_rep_id(self.ptr, id.into(), &mut uid) })?;
        Ok(uid.into())
    }

    pub fn rep_id_of(&self, uid: RepUid) -> Result<RepId> {
        let mut id: sys::occtl_rep_id_t = Default::default();
        check(unsafe { sys::occtl_graph_rep_id_from_rep_uid(self.ptr, uid.into(), &mut id) })?;
        Ok(id.into())
    }

    fn fetch_history(
        &self,
        input: Option<Uid>,
        f: unsafe extern "C" fn(
            *const sys::occtl_graph_t,
            sys::occtl_uid_t,
            *mut sys::occtl_uid_t,
            usize,
            *mut usize,
        ) -> sys::occtl_status_t,
    ) -> Result<Vec<Uid>> {
        let mut count: usize = 0;
        let uid = input.unwrap_or(Uid::default());
        let status = unsafe { f(self.ptr, uid.into(), ptr::null_mut(), 0, &mut count) };
        if status == sys::occtl_status::OCCTL_NOT_FOUND {
            return Ok(Vec::new());
        }
        check(status)?;
        let mut out = vec![sys::occtl_uid_t::default(); count];
        check(unsafe {
            f(
                self.ptr,
                uid.into(),
                out.as_mut_ptr(),
                out.len(),
                &mut count,
            )
        })?;
        out.truncate(count);
        Ok(out.into_iter().map(Into::into).collect())
    }

    pub fn history_modified(&self, input: Uid) -> Result<Vec<Uid>> {
        self.fetch_history(Some(input), sys::occtl_graph_history_modified)
    }

    pub fn history_generated(&self, input: Uid) -> Result<Vec<Uid>> {
        self.fetch_history(Some(input), sys::occtl_graph_history_generated)
    }

    pub fn history_deleted_all(&self) -> Result<Vec<Uid>> {
        let mut count: usize = 0;
        let status = unsafe {
            sys::occtl_graph_history_deleted_all(self.ptr, ptr::null_mut(), 0, &mut count)
        };
        if status == sys::occtl_status::OCCTL_NOT_FOUND {
            return Ok(Vec::new());
        }
        check(status)?;
        let mut out = vec![sys::occtl_uid_t::default(); count];
        check(unsafe {
            sys::occtl_graph_history_deleted_all(self.ptr, out.as_mut_ptr(), out.len(), &mut count)
        })?;
        out.truncate(count);
        Ok(out.into_iter().map(Into::into).collect())
    }

    #[cfg(occtl_has_prim)]
    /// Builds a box in this graph.
    pub fn make_box(&self, dx: f64, dy: f64, dz: f64) -> Result<NodeId> {
        prim::make_box(self, BoxInfo::new(dx, dy, dz))
    }

    #[cfg(occtl_has_prim)]
    /// Builds a sphere in this graph.
    pub fn make_sphere(&self, radius: f64) -> Result<NodeId> {
        prim::make_sphere(self, SphereInfo::new(radius))
    }

    #[cfg(occtl_has_prim)]
    /// Builds a cylinder in this graph.
    pub fn make_cylinder(&self, radius: f64, height: f64) -> Result<NodeId> {
        prim::make_cylinder(self, CylinderInfo::new(radius, height))
    }

    #[cfg(occtl_has_prim)]
    /// Builds a cone or truncated cone in this graph.
    pub fn make_cone(&self, r1: f64, r2: f64, height: f64) -> Result<NodeId> {
        prim::make_cone(self, ConeInfo::new(r1, r2, height))
    }

    #[cfg(occtl_has_prim)]
    /// Builds a torus in this graph.
    pub fn make_torus(&self, r1: f64, r2: f64) -> Result<NodeId> {
        prim::make_torus(self, TorusInfo::new(r1, r2))
    }

    #[cfg(occtl_has_prim)]
    /// Builds a wedge in this graph.
    pub fn make_wedge(&self, dx: f64, dy: f64, dz: f64, ltx: f64) -> Result<NodeId> {
        prim::make_wedge(self, WedgeInfo::new(dx, dy, dz, ltx))
    }

    #[cfg(occtl_has_bool)]
    /// Runs the Fuse boolean operation.
    pub fn fuse(&self, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
        crate::bool_::fuse(self, objects, tools)
    }
    #[cfg(occtl_has_bool)]
    /// Runs the Fuse boolean operation with explicit options.
    pub fn fuse_with_options(
        &self,
        objects: &[NodeId],
        tools: &[NodeId],
        options: crate::bool_::BoolOptions,
    ) -> Result<NodeId> {
        crate::bool_::fuse_with_options(self, objects, tools, options)
    }

    #[cfg(occtl_has_bool)]
    /// Runs the Cut boolean operation.
    pub fn cut(&self, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
        crate::bool_::cut(self, objects, tools)
    }
    #[cfg(occtl_has_bool)]
    /// Runs the Cut boolean operation with explicit options.
    pub fn cut_with_options(
        &self,
        objects: &[NodeId],
        tools: &[NodeId],
        options: crate::bool_::BoolOptions,
    ) -> Result<NodeId> {
        crate::bool_::cut_with_options(self, objects, tools, options)
    }

    #[cfg(occtl_has_bool)]
    /// Runs the Common boolean operation.
    pub fn common(&self, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
        crate::bool_::common(self, objects, tools)
    }
    #[cfg(occtl_has_bool)]
    /// Runs the Common boolean operation with explicit options.
    pub fn common_with_options(
        &self,
        objects: &[NodeId],
        tools: &[NodeId],
        options: crate::bool_::BoolOptions,
    ) -> Result<NodeId> {
        crate::bool_::common_with_options(self, objects, tools, options)
    }

    #[cfg(occtl_has_bool)]
    /// Runs the Section boolean operation.
    pub fn section(&self, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
        crate::bool_::section(self, objects, tools)
    }
    #[cfg(occtl_has_bool)]
    /// Runs the Section boolean operation with explicit options.
    pub fn section_with_options(
        &self,
        objects: &[NodeId],
        tools: &[NodeId],
        options: crate::bool_::BoolOptions,
    ) -> Result<NodeId> {
        crate::bool_::section_with_options(self, objects, tools, options)
    }

    #[cfg(occtl_has_bool)]
    /// Runs the Split boolean operation.
    pub fn split(&self, objects: &[NodeId], tools: &[NodeId]) -> Result<NodeId> {
        crate::bool_::split(self, objects, tools)
    }
    #[cfg(occtl_has_bool)]
    /// Runs the Split boolean operation with explicit options.
    pub fn split_with_options(
        &self,
        objects: &[NodeId],
        tools: &[NodeId],
        options: crate::bool_::BoolOptions,
    ) -> Result<NodeId> {
        crate::bool_::split_with_options(self, objects, tools, options)
    }

    /// Iterator over every active face.
    pub fn face_iter(&self) -> Result<NodeIter<'_>> {
        let mut it: *mut sys::occtl_node_iter_t = ptr::null_mut();
        check(unsafe { sys::occtl_graph_face_iter_create(self.ptr, &mut it) })?;
        Ok(NodeIter::new(it))
    }

    /// Iterator over every active solid.
    pub fn solid_iter(&self) -> Result<NodeIter<'_>> {
        let mut it: *mut sys::occtl_node_iter_t = ptr::null_mut();
        check(unsafe { sys::occtl_graph_solid_iter_create(self.ptr, &mut it) })?;
        Ok(NodeIter::new(it))
    }
}

impl Drop for Graph {
    fn drop(&mut self) {
        unsafe { sys::occtl_graph_free(self.ptr) };
    }
}

/// RAII iterator yielding successive `NodeId`s; tied to the parent graph's lifetime.
pub struct NodeIter<'g> {
    ptr: *mut sys::occtl_node_iter_t,
    _marker: PhantomData<&'g Graph>,
}

impl<'g> NodeIter<'g> {
    fn new(ptr: *mut sys::occtl_node_iter_t) -> Self {
        Self {
            ptr,
            _marker: PhantomData,
        }
    }
}

impl<'g> Drop for NodeIter<'g> {
    fn drop(&mut self) {
        unsafe { sys::occtl_node_iter_free(self.ptr) };
    }
}

impl<'g> Iterator for NodeIter<'g> {
    type Item = NodeId;
    fn next(&mut self) -> Option<NodeId> {
        let mut id: sys::occtl_node_id_t = Default::default();
        let st = unsafe { sys::occtl_node_iter_next(self.ptr, &mut id) };
        if st == sys::occtl_status::OCCTL_OK {
            Some(id.into())
        } else {
            None
        }
    }
}
