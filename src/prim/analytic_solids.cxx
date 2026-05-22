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

//! @file analytic_solids.cxx
//! @brief Analytic primitive solids: Box, Sphere, Cylinder, Cone, Torus,
//!        Wedge.  Co-located here are the runtime initialisers for all
//!        eleven prim info structs that this translation unit owns by
//!        convention (matching the topo module).

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/CurveMath.hxx"

#include <occtl/occtl_prim.h>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakeWedge.hxx>

#include <gp_Ax2.hxx>

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_box_info_init(occtl_prim_box_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_BOX_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_sphere_info_init(occtl_prim_sphere_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_cylinder_info_init(occtl_prim_cylinder_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_CYLINDER_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_cone_info_init(occtl_prim_cone_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_CONE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_torus_info_init(occtl_prim_torus_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_TORUS_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_wedge_info_init(occtl_prim_wedge_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_WEDGE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_halfspace_info_init(occtl_prim_halfspace_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_HALFSPACE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_prism_info_init(occtl_prim_prism_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_revol_info_init(occtl_prim_revol_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_pipe_info_init(occtl_prim_pipe_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_PIPE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_loft_info_init(occtl_prim_loft_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_LOFT_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_box(occtl_graph_t* const               theGraph,
                                                        const occtl_prim_box_info_t* const theInfo,
                                                        occtl_node_id_t* const theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_BOX_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_BOX_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutSolid = OCCTL_NODE_ID_INVALID;
    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_box_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinitePlacement(theInfo->placement, "box") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->dx, "dx") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->dy, "dy") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->dz, "dz") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Ax2 anAxes = OcctL::Geom::ToGpAx2(theInfo->placement);

    BRepPrimAPI_MakeBox aMaker(anAxes, theInfo->dx, theInfo->dy, theInfo->dz);
    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepPrimAPI_MakeBox: dimensions must be positive");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Solid(), *theOutSolid);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_sphere(occtl_graph_t* const                  theGraph,
                         const occtl_prim_sphere_info_t* const theInfo,
                         occtl_node_id_t* const                theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_SPHERE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_SPHERE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutSolid = OCCTL_NODE_ID_INVALID;
    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_sphere_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinitePlacement(theInfo->placement, "sphere") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->radius, "radius") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle1, "angle1") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle2, "angle2") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle, "angle") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Ax2 anAxes = OcctL::Geom::ToGpAx2(theInfo->placement);

    BRepPrimAPI_MakeSphere aMaker(anAxes,
                                  theInfo->radius,
                                  theInfo->angle1,
                                  theInfo->angle2,
                                  theInfo->angle);
    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepPrimAPI_MakeSphere reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutSolid);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_cylinder(occtl_graph_t* const                    theGraph,
                           const occtl_prim_cylinder_info_t* const theInfo,
                           occtl_node_id_t* const                  theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_CYLINDER_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_CYLINDER_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutSolid = OCCTL_NODE_ID_INVALID;
    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_cylinder_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinitePlacement(theInfo->placement, "cylinder") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->radius, "radius") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->height, "height") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle, "angle") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Ax2 anAxes = OcctL::Geom::ToGpAx2(theInfo->placement);

    BRepPrimAPI_MakeCylinder aMaker(anAxes, theInfo->radius, theInfo->height, theInfo->angle);
    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "BRepPrimAPI_MakeCylinder: radius and height must be positive");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutSolid);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_cone(occtl_graph_t* const                theGraph,
                       const occtl_prim_cone_info_t* const theInfo,
                       occtl_node_id_t* const              theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_CONE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_CONE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutSolid = OCCTL_NODE_ID_INVALID;
    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_cone_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinitePlacement(theInfo->placement, "cone") != OCCTL_OK
        || OcctL::Prim::CheckNonNegative(theInfo->r1, "r1") != OCCTL_OK
        || OcctL::Prim::CheckNonNegative(theInfo->r2, "r2") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->height, "height") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle, "angle") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->r1 == 0.0 && theInfo->r2 == 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "r1 and r2 cannot both be zero");
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Ax2 anAxes = OcctL::Geom::ToGpAx2(theInfo->placement);

    BRepPrimAPI_MakeCone aMaker(anAxes, theInfo->r1, theInfo->r2, theInfo->height, theInfo->angle);
    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepPrimAPI_MakeCone reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutSolid);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_torus(occtl_graph_t* const                 theGraph,
                        const occtl_prim_torus_info_t* const theInfo,
                        occtl_node_id_t* const               theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_TORUS_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_TORUS_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutSolid = OCCTL_NODE_ID_INVALID;
    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_torus_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinitePlacement(theInfo->placement, "torus") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->r1, "r1") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->r2, "r2") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle1, "angle1") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle2, "angle2") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle, "angle") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->r2 >= theInfo->r1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "r2 must be smaller than r1");
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Ax2 anAxes = OcctL::Geom::ToGpAx2(theInfo->placement);

    BRepPrimAPI_MakeTorus aMaker(anAxes,
                                 theInfo->r1,
                                 theInfo->r2,
                                 theInfo->angle1,
                                 theInfo->angle2,
                                 theInfo->angle);
    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepPrimAPI_MakeTorus reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutSolid);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_wedge(occtl_graph_t* const                 theGraph,
                        const occtl_prim_wedge_info_t* const theInfo,
                        occtl_node_id_t* const               theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_WEDGE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_WEDGE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutSolid = OCCTL_NODE_ID_INVALID;
    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_wedge_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinitePlacement(theInfo->placement, "wedge") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->dx, "dx") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->dy, "dy") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->dz, "dz") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->ltx, "ltx") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->ltx < 0.0 || theInfo->ltx > theInfo->dx)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "ltx must be in the range [0, dx]");
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Ax2 anAxes = OcctL::Geom::ToGpAx2(theInfo->placement);

    BRepPrimAPI_MakeWedge aMaker(anAxes, theInfo->dx, theInfo->dy, theInfo->dz, theInfo->ltx);
    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepPrimAPI_MakeWedge reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Solid(), *theOutSolid);
  });
}

} // extern "C"
