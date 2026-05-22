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

/// Convenience mirror of `occtl_prim_box_info_t`.
#[derive(Copy, Clone, Debug)]
pub struct BoxInfo {
    pub dx: f64,
    pub dy: f64,
    pub dz: f64,
    pub placement: sys::occtl_axis2_placement_t,
}

impl Default for BoxInfo {
    fn default() -> Self {
        let mut info: sys::occtl_prim_box_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_box_info_init(&mut info) };
        Self {
            dx: 0.0,
            dy: 0.0,
            dz: 0.0,
            placement: info.placement,
        }
    }
}

impl BoxInfo {
    pub fn new(dx: f64, dy: f64, dz: f64) -> Self {
        Self {
            dx,
            dy,
            dz,
            ..Self::default()
        }
    }
    pub fn with_placement(mut self, placement: sys::occtl_axis2_placement_t) -> Self {
        self.placement = placement;
        self
    }

    /// Project into the C ABI options struct.
    pub fn to_c(self) -> sys::occtl_prim_box_info_t {
        let mut info: sys::occtl_prim_box_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_box_info_init(&mut info) };
        info.dx = self.dx;
        info.dy = self.dy;
        info.dz = self.dz;
        info.placement = self.placement;
        info
    }
}

/// Convenience mirror of `occtl_prim_sphere_info_t`.
#[derive(Copy, Clone, Debug)]
pub struct SphereInfo {
    pub radius: f64,
    pub angle1: Option<f64>,
    pub angle2: Option<f64>,
    pub angle: Option<f64>,
    pub placement: sys::occtl_axis2_placement_t,
}

impl Default for SphereInfo {
    fn default() -> Self {
        let mut info: sys::occtl_prim_sphere_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_sphere_info_init(&mut info) };
        Self {
            radius: 0.0,
            angle1: None,
            angle2: None,
            angle: None,
            placement: info.placement,
        }
    }
}

impl SphereInfo {
    pub fn new(radius: f64) -> Self {
        Self {
            radius,
            ..Self::default()
        }
    }
    pub fn with_placement(mut self, placement: sys::occtl_axis2_placement_t) -> Self {
        self.placement = placement;
        self
    }

    /// Project into the C ABI options struct.
    pub fn to_c(self) -> sys::occtl_prim_sphere_info_t {
        let mut info: sys::occtl_prim_sphere_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_sphere_info_init(&mut info) };
        info.radius = self.radius;
        info.placement = self.placement;
        if let Some(v) = self.angle1 {
            info.angle1 = v;
        }
        if let Some(v) = self.angle2 {
            info.angle2 = v;
        }
        if let Some(v) = self.angle {
            info.angle = v;
        }
        info
    }
}

/// Convenience mirror of `occtl_prim_cylinder_info_t`.
#[derive(Copy, Clone, Debug)]
pub struct CylinderInfo {
    pub radius: f64,
    pub height: f64,
    pub angle: Option<f64>,
    pub placement: sys::occtl_axis2_placement_t,
}

impl Default for CylinderInfo {
    fn default() -> Self {
        let mut info: sys::occtl_prim_cylinder_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_cylinder_info_init(&mut info) };
        Self {
            radius: 0.0,
            height: 0.0,
            angle: None,
            placement: info.placement,
        }
    }
}

impl CylinderInfo {
    pub fn new(radius: f64, height: f64) -> Self {
        Self {
            radius,
            height,
            ..Self::default()
        }
    }
    pub fn with_placement(mut self, placement: sys::occtl_axis2_placement_t) -> Self {
        self.placement = placement;
        self
    }

    /// Project into the C ABI options struct.
    pub fn to_c(self) -> sys::occtl_prim_cylinder_info_t {
        let mut info: sys::occtl_prim_cylinder_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_cylinder_info_init(&mut info) };
        info.radius = self.radius;
        info.height = self.height;
        info.placement = self.placement;
        if let Some(v) = self.angle {
            info.angle = v;
        }
        info
    }
}

/// Convenience mirror of `occtl_prim_cone_info_t`.
#[derive(Copy, Clone, Debug)]
pub struct ConeInfo {
    pub r1: f64,
    pub r2: f64,
    pub height: f64,
    pub angle: Option<f64>,
    pub placement: sys::occtl_axis2_placement_t,
}

impl Default for ConeInfo {
    fn default() -> Self {
        let mut info: sys::occtl_prim_cone_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_cone_info_init(&mut info) };
        Self {
            r1: 0.0,
            r2: 0.0,
            height: 0.0,
            angle: None,
            placement: info.placement,
        }
    }
}

impl ConeInfo {
    pub fn new(r1: f64, r2: f64, height: f64) -> Self {
        Self {
            r1,
            r2,
            height,
            ..Self::default()
        }
    }
    pub fn with_placement(mut self, placement: sys::occtl_axis2_placement_t) -> Self {
        self.placement = placement;
        self
    }

    /// Project into the C ABI options struct.
    pub fn to_c(self) -> sys::occtl_prim_cone_info_t {
        let mut info: sys::occtl_prim_cone_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_cone_info_init(&mut info) };
        info.r1 = self.r1;
        info.r2 = self.r2;
        info.height = self.height;
        info.placement = self.placement;
        if let Some(v) = self.angle {
            info.angle = v;
        }
        info
    }
}

/// Convenience mirror of `occtl_prim_torus_info_t`.
#[derive(Copy, Clone, Debug)]
pub struct TorusInfo {
    pub r1: f64,
    pub r2: f64,
    pub angle1: Option<f64>,
    pub angle2: Option<f64>,
    pub angle: Option<f64>,
    pub placement: sys::occtl_axis2_placement_t,
}

impl Default for TorusInfo {
    fn default() -> Self {
        let mut info: sys::occtl_prim_torus_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_torus_info_init(&mut info) };
        Self {
            r1: 0.0,
            r2: 0.0,
            angle1: None,
            angle2: None,
            angle: None,
            placement: info.placement,
        }
    }
}

impl TorusInfo {
    pub fn new(r1: f64, r2: f64) -> Self {
        Self {
            r1,
            r2,
            ..Self::default()
        }
    }
    pub fn with_placement(mut self, placement: sys::occtl_axis2_placement_t) -> Self {
        self.placement = placement;
        self
    }

    /// Project into the C ABI options struct.
    pub fn to_c(self) -> sys::occtl_prim_torus_info_t {
        let mut info: sys::occtl_prim_torus_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_torus_info_init(&mut info) };
        info.r1 = self.r1;
        info.r2 = self.r2;
        info.placement = self.placement;
        if let Some(v) = self.angle1 {
            info.angle1 = v;
        }
        if let Some(v) = self.angle2 {
            info.angle2 = v;
        }
        if let Some(v) = self.angle {
            info.angle = v;
        }
        info
    }
}

/// Convenience mirror of `occtl_prim_wedge_info_t`.
#[derive(Copy, Clone, Debug)]
pub struct WedgeInfo {
    pub dx: f64,
    pub dy: f64,
    pub dz: f64,
    pub ltx: f64,
    pub placement: sys::occtl_axis2_placement_t,
}

impl Default for WedgeInfo {
    fn default() -> Self {
        let mut info: sys::occtl_prim_wedge_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_wedge_info_init(&mut info) };
        Self {
            dx: 0.0,
            dy: 0.0,
            dz: 0.0,
            ltx: 0.0,
            placement: info.placement,
        }
    }
}

impl WedgeInfo {
    pub fn new(dx: f64, dy: f64, dz: f64, ltx: f64) -> Self {
        Self {
            dx,
            dy,
            dz,
            ltx,
            ..Self::default()
        }
    }
    pub fn with_placement(mut self, placement: sys::occtl_axis2_placement_t) -> Self {
        self.placement = placement;
        self
    }

    /// Project into the C ABI options struct.
    pub fn to_c(self) -> sys::occtl_prim_wedge_info_t {
        let mut info: sys::occtl_prim_wedge_info_t = unsafe { std::mem::zeroed() };
        unsafe { sys::occtl_prim_wedge_info_init(&mut info) };
        info.dx = self.dx;
        info.dy = self.dy;
        info.dz = self.dz;
        info.ltx = self.ltx;
        info.placement = self.placement;
        info
    }
}

/// Builds an axis-aligned box and returns the new solid NodeId.
pub fn make_box(graph: &Graph, info: BoxInfo) -> Result<NodeId> {
    let raw = info.to_c();
    let mut id: sys::occtl_node_id_t = Default::default();
    check(unsafe { sys::occtl_prim_make_box(graph.as_ptr(), &raw, &mut id) })?;
    Ok(id.into())
}

/// Builds a sphere and returns the new solid NodeId.
pub fn make_sphere(graph: &Graph, info: SphereInfo) -> Result<NodeId> {
    let raw = info.to_c();
    let mut id: sys::occtl_node_id_t = Default::default();
    check(unsafe { sys::occtl_prim_make_sphere(graph.as_ptr(), &raw, &mut id) })?;
    Ok(id.into())
}

/// Builds a cylinder and returns the new solid NodeId.
pub fn make_cylinder(graph: &Graph, info: CylinderInfo) -> Result<NodeId> {
    let raw = info.to_c();
    let mut id: sys::occtl_node_id_t = Default::default();
    check(unsafe { sys::occtl_prim_make_cylinder(graph.as_ptr(), &raw, &mut id) })?;
    Ok(id.into())
}

/// Builds a cone or truncated cone and returns the new solid NodeId.
pub fn make_cone(graph: &Graph, info: ConeInfo) -> Result<NodeId> {
    let raw = info.to_c();
    let mut id: sys::occtl_node_id_t = Default::default();
    check(unsafe { sys::occtl_prim_make_cone(graph.as_ptr(), &raw, &mut id) })?;
    Ok(id.into())
}

/// Builds a torus and returns the new solid NodeId.
pub fn make_torus(graph: &Graph, info: TorusInfo) -> Result<NodeId> {
    let raw = info.to_c();
    let mut id: sys::occtl_node_id_t = Default::default();
    check(unsafe { sys::occtl_prim_make_torus(graph.as_ptr(), &raw, &mut id) })?;
    Ok(id.into())
}

/// Builds a wedge and returns the new solid NodeId.
pub fn make_wedge(graph: &Graph, info: WedgeInfo) -> Result<NodeId> {
    let raw = info.to_c();
    let mut id: sys::occtl_node_id_t = Default::default();
    check(unsafe { sys::occtl_prim_make_wedge(graph.as_ptr(), &raw, &mut id) })?;
    Ok(id.into())
}

/// Helper: builds an `occtl_axis2_placement_t` from origin (x,y,z) with the
/// canonical (Z up, X along +x) orientation.
pub fn placement_at(x: f64, y: f64, z: f64) -> sys::occtl_axis2_placement_t {
    sys::occtl_axis2_placement_t {
        location: sys::occtl_point3_t { x, y, z },
        x_dir: sys::occtl_direction3_t {
            x: 1.0,
            y: 0.0,
            z: 0.0,
        },
        x_dir_ref: sys::occtl_direction3_t {
            x: 0.0,
            y: 1.0,
            z: 0.0,
        },
    }
}
