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

package org.occtl;

import com.sun.jna.Memory;

import org.occtl.generated.OcctlGenerated;
import org.occtl.generated.OcctlTypes;

/** Primitive-builder helpers. */
public final class Prim {
    private Prim() {}

    public static NodeId makeBox(Graph graph, double dx, double dy, double dz) {
        OcctL.requireFeature("prim", "occtl_prim_make_box");
        return makeBox(graph, new BoxInfo(dx, dy, dz));
    }

    public static NodeId makeBox(Graph graph, BoxInfo box) {
        OcctL.requireFeature("prim", "occtl_prim_make_box");
        OcctlTypes.OcctlPrimBoxInfo info = new OcctlTypes.OcctlPrimBoxInfo();
        info.struct_version = (int) OcctlGenerated.OCCTL_PRIM_BOX_INFO_VERSION_1;
        info.p_next = null;

        info.placement = toNativePlacement(box.placement());
        info.dx = box.dx();
        info.dy = box.dy();
        info.dz = box.dz();
        info.write();

        Memory outSolid = new Memory(8);
        OcctL.check(OcctL.RAW.occtl_prim_make_box(graph.nativeHandle(), info.getPointer(), outSolid));
        return new NodeId(outSolid.getLong(0));
    }

    public static NodeId makeSphere(Graph graph, double radius) {
        OcctL.requireFeature("prim", "occtl_prim_make_sphere");
        OcctlTypes.OcctlPrimSphereInfo info = new OcctlTypes.OcctlPrimSphereInfo();
        info.struct_version = (int) OcctlGenerated.OCCTL_PRIM_SPHERE_INFO_VERSION_1;
        info.p_next = null;
        info.placement = toNativePlacement(Axis2Placement.identity());
        info.radius = radius;
        info.write();

        Memory outSolid = new Memory(8);
        OcctL.check(OcctL.RAW.occtl_prim_make_sphere(graph.nativeHandle(), info.getPointer(), outSolid));
        return new NodeId(outSolid.getLong(0));
    }

    public static NodeId makeCylinder(Graph graph, double radius, double height) {
        OcctL.requireFeature("prim", "occtl_prim_make_cylinder");
        OcctlTypes.OcctlPrimCylinderInfo info = new OcctlTypes.OcctlPrimCylinderInfo();
        info.struct_version = (int) OcctlGenerated.OCCTL_PRIM_CYLINDER_INFO_VERSION_1;
        info.p_next = null;
        info.placement = toNativePlacement(Axis2Placement.identity());
        info.radius = radius;
        info.height = height;
        info.write();

        Memory outSolid = new Memory(8);
        OcctL.check(OcctL.RAW.occtl_prim_make_cylinder(graph.nativeHandle(), info.getPointer(), outSolid));
        return new NodeId(outSolid.getLong(0));
    }

    public static NodeId makeCone(Graph graph, double r1, double r2, double height) {
        OcctL.requireFeature("prim", "occtl_prim_make_cone");
        OcctlTypes.OcctlPrimConeInfo info = new OcctlTypes.OcctlPrimConeInfo();
        info.struct_version = (int) OcctlGenerated.OCCTL_PRIM_CONE_INFO_VERSION_1;
        info.p_next = null;
        info.placement = toNativePlacement(Axis2Placement.identity());
        info.r1 = r1;
        info.r2 = r2;
        info.height = height;
        info.write();

        Memory outSolid = new Memory(8);
        OcctL.check(OcctL.RAW.occtl_prim_make_cone(graph.nativeHandle(), info.getPointer(), outSolid));
        return new NodeId(outSolid.getLong(0));
    }

    public static NodeId makeTorus(Graph graph, double r1, double r2) {
        OcctL.requireFeature("prim", "occtl_prim_make_torus");
        OcctlTypes.OcctlPrimTorusInfo info = new OcctlTypes.OcctlPrimTorusInfo();
        info.struct_version = (int) OcctlGenerated.OCCTL_PRIM_TORUS_INFO_VERSION_1;
        info.p_next = null;
        info.placement = toNativePlacement(Axis2Placement.identity());
        info.r1 = r1;
        info.r2 = r2;
        info.write();

        Memory outSolid = new Memory(8);
        OcctL.check(OcctL.RAW.occtl_prim_make_torus(graph.nativeHandle(), info.getPointer(), outSolid));
        return new NodeId(outSolid.getLong(0));
    }

    public static NodeId makeWedge(Graph graph, double dx, double dy, double dz, double ltx) {
        OcctL.requireFeature("prim", "occtl_prim_make_wedge");
        OcctlTypes.OcctlPrimWedgeInfo info = new OcctlTypes.OcctlPrimWedgeInfo();
        info.struct_version = (int) OcctlGenerated.OCCTL_PRIM_WEDGE_INFO_VERSION_1;
        info.p_next = null;
        info.placement = toNativePlacement(Axis2Placement.identity());
        info.dx = dx;
        info.dy = dy;
        info.dz = dz;
        info.ltx = ltx;
        info.write();

        Memory outSolid = new Memory(8);
        OcctL.check(OcctL.RAW.occtl_prim_make_wedge(graph.nativeHandle(), info.getPointer(), outSolid));
        return new NodeId(outSolid.getLong(0));
    }

    private static OcctlTypes.OcctlAxis2Placement toNativePlacement(Axis2Placement placement) {
        Axis2Placement p = placement != null ? placement : Axis2Placement.identity();

        OcctlTypes.OcctlAxis2Placement out = new OcctlTypes.OcctlAxis2Placement();
        out.location = new OcctlTypes.OcctlPoint3();
        out.location.x = p.location().x();
        out.location.y = p.location().y();
        out.location.z = p.location().z();

        out.x_dir = new OcctlTypes.OcctlDirection3();
        out.x_dir.x = p.xDir().x();
        out.x_dir.y = p.xDir().y();
        out.x_dir.z = p.xDir().z();

        out.x_dir_ref = new OcctlTypes.OcctlDirection3();
        out.x_dir_ref.x = p.xDirRef().x();
        out.x_dir_ref.y = p.xDirRef().y();
        out.x_dir_ref.z = p.xDirRef().z();
        return out;
    }
}
