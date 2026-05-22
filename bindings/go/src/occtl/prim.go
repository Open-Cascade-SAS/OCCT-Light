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

//go:build occtl_prim && occtl_topo

package occtl

/*
#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>
#include <occtl/occtl_prim.h>
*/
import "C"

// PlacementAt builds an occtl_axis2_placement_t at (x,y,z) with the
// canonical (Z up, X along +x) orientation.
func PlacementAt(x, y, z float64) C.occtl_axis2_placement_t {
	var p C.occtl_axis2_placement_t
	p.location.x = C.double(x)
	p.location.y = C.double(y)
	p.location.z = C.double(z)
	p.x_dir.x = 1
	p.x_dir.y = 0
	p.x_dir.z = 0
	p.x_dir_ref.x = 0
	p.x_dir_ref.y = 1
	p.x_dir_ref.z = 0
	return p
}

// MakeBox builds an axis-aligned box and returns the new solid node id.
func MakeBox(g *Graph, dx, dy, dz float64) (NodeId, error) {
	return MakeBoxAt(g, dx, dy, dz, PlacementAt(0, 0, 0))
}

// MakeBoxAt builds a box with a custom placement.
func MakeBoxAt(g *Graph, dx, dy, dz float64, placement C.occtl_axis2_placement_t) (NodeId, error) {
	var info C.occtl_prim_box_info_t
	C.occtl_prim_box_info_init(&info)
	info.placement = placement
	info.dx = C.double(dx)
	info.dy = C.double(dy)
	info.dz = C.double(dz)
	var id C.occtl_node_id_t
	if err := check(C.occtl_prim_make_box(g.ptr, &info, &id)); err != nil {
		return NodeId{}, err
	}
	return NodeId{raw: id}, nil
}

// MakeSphere builds a sphere and returns the new solid node id.
func MakeSphere(g *Graph, radius float64) (NodeId, error) {
	return MakeSphereAt(g, radius, PlacementAt(0, 0, 0))
}

// MakeSphereAt builds a sphere with a custom placement.
func MakeSphereAt(g *Graph, radius float64, placement C.occtl_axis2_placement_t) (NodeId, error) {
	var info C.occtl_prim_sphere_info_t
	C.occtl_prim_sphere_info_init(&info)
	info.placement = placement
	info.radius = C.double(radius)
	var id C.occtl_node_id_t
	if err := check(C.occtl_prim_make_sphere(g.ptr, &info, &id)); err != nil {
		return NodeId{}, err
	}
	return NodeId{raw: id}, nil
}

// MakeCylinder builds a cylinder and returns the new solid node id.
func MakeCylinder(g *Graph, radius, height float64) (NodeId, error) {
	return MakeCylinderAt(g, radius, height, PlacementAt(0, 0, 0))
}

// MakeCylinderAt builds a cylinder with a custom placement.
func MakeCylinderAt(g *Graph, radius, height float64, placement C.occtl_axis2_placement_t) (NodeId, error) {
	var info C.occtl_prim_cylinder_info_t
	C.occtl_prim_cylinder_info_init(&info)
	info.placement = placement
	info.radius = C.double(radius)
	info.height = C.double(height)
	var id C.occtl_node_id_t
	if err := check(C.occtl_prim_make_cylinder(g.ptr, &info, &id)); err != nil {
		return NodeId{}, err
	}
	return NodeId{raw: id}, nil
}

// MakeCone builds a cone or truncated cone and returns the new solid node id.
func MakeCone(g *Graph, r1, r2, height float64) (NodeId, error) {
	return MakeConeAt(g, r1, r2, height, PlacementAt(0, 0, 0))
}

// MakeConeAt builds a cone or truncated cone with a custom placement.
func MakeConeAt(g *Graph, r1, r2, height float64, placement C.occtl_axis2_placement_t) (NodeId, error) {
	var info C.occtl_prim_cone_info_t
	C.occtl_prim_cone_info_init(&info)
	info.placement = placement
	info.r1 = C.double(r1)
	info.r2 = C.double(r2)
	info.height = C.double(height)
	var id C.occtl_node_id_t
	if err := check(C.occtl_prim_make_cone(g.ptr, &info, &id)); err != nil {
		return NodeId{}, err
	}
	return NodeId{raw: id}, nil
}

// MakeTorus builds a torus and returns the new solid node id.
func MakeTorus(g *Graph, r1, r2 float64) (NodeId, error) {
	return MakeTorusAt(g, r1, r2, PlacementAt(0, 0, 0))
}

// MakeTorusAt builds a torus with a custom placement.
func MakeTorusAt(g *Graph, r1, r2 float64, placement C.occtl_axis2_placement_t) (NodeId, error) {
	var info C.occtl_prim_torus_info_t
	C.occtl_prim_torus_info_init(&info)
	info.placement = placement
	info.r1 = C.double(r1)
	info.r2 = C.double(r2)
	var id C.occtl_node_id_t
	if err := check(C.occtl_prim_make_torus(g.ptr, &info, &id)); err != nil {
		return NodeId{}, err
	}
	return NodeId{raw: id}, nil
}

// MakeWedge builds a wedge and returns the new solid node id.
func MakeWedge(g *Graph, dx, dy, dz, ltx float64) (NodeId, error) {
	return MakeWedgeAt(g, dx, dy, dz, ltx, PlacementAt(0, 0, 0))
}

// MakeWedgeAt builds a wedge with a custom placement.
func MakeWedgeAt(g *Graph, dx, dy, dz, ltx float64, placement C.occtl_axis2_placement_t) (NodeId, error) {
	var info C.occtl_prim_wedge_info_t
	C.occtl_prim_wedge_info_init(&info)
	info.placement = placement
	info.dx = C.double(dx)
	info.dy = C.double(dy)
	info.dz = C.double(dz)
	info.ltx = C.double(ltx)
	var id C.occtl_node_id_t
	if err := check(C.occtl_prim_make_wedge(g.ptr, &info, &id)); err != nil {
		return NodeId{}, err
	}
	return NodeId{raw: id}, nil
}

// MakeBox builds an axis-aligned box in this graph.
func (g *Graph) MakeBox(dx, dy, dz float64) (NodeId, error) { return MakeBox(g, dx, dy, dz) }

// MakeSphere builds a sphere in this graph.
func (g *Graph) MakeSphere(radius float64) (NodeId, error) { return MakeSphere(g, radius) }

// MakeCylinder builds a cylinder in this graph.
func (g *Graph) MakeCylinder(radius, height float64) (NodeId, error) {
	return MakeCylinder(g, radius, height)
}

// MakeCone builds a cone or truncated cone in this graph.
func (g *Graph) MakeCone(r1, r2, height float64) (NodeId, error) { return MakeCone(g, r1, r2, height) }

// MakeTorus builds a torus in this graph.
func (g *Graph) MakeTorus(r1, r2 float64) (NodeId, error) { return MakeTorus(g, r1, r2) }

// MakeWedge builds a wedge in this graph.
func (g *Graph) MakeWedge(dx, dy, dz, ltx float64) (NodeId, error) {
	return MakeWedge(g, dx, dy, dz, ltx)
}
