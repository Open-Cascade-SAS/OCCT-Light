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

#include "IdConvert.hxx"
#include "TopoMath.hxx"

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include "../geom/RepLookup.hxx"

#include <BRepGraph_EditorView.hxx>
#include <BRepGraph_RefsIterator.hxx>
#include <BRepGraph_RefsView.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_TopoView.hxx>

#include <BRepGraphInc_Definition.hxx>
#include <BRepGraphInc_Reference.hxx>

#include <GeomAdaptor_TransformedCurve.hxx>

#include <NCollection_Array1.hxx>
#include <NCollection_DynamicArray.hxx>
#include <NCollection_LinearVector.hxx>

#include <gp_Pnt.hxx>

#include <Precision.hxx>

#include <TopAbs.hxx>

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_replace_edge_curve(occtl_graph_t* const  theGraph,
                                                                  const occtl_node_id_t theEdge,
                                                                  const occtl_rep_id_t  theCurveId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    if (theCurveId.bits == 0)
    {
      theGraph->graph.Editor().Edges().SetCurve3DRepId(anEdgeId, BRepGraph_Curve3DRepId());
      return OCCTL_OK;
    }

    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve3D");
      return OCCTL_WRONG_KIND;
    }

    BRepGraph_Curve3DRepId aCurve3DId(static_cast<uint32_t>(aRawId.Index));
    theGraph->graph.Editor().Edges().SetCurve3DRepId(anEdgeId, aCurve3DId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_replace_face_surface(occtl_graph_t* const  theGraph,
                                  const occtl_node_id_t theFace,
                                  const occtl_rep_id_t  theSurfaceId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    if (theSurfaceId.bits == 0)
    {
      theGraph->graph.Editor().Faces().SetSurfaceRepId(aFaceId, BRepGraph_SurfaceRepId());
      return OCCTL_OK;
    }

    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theSurfaceId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Surface");
      return OCCTL_WRONG_KIND;
    }

    BRepGraph_SurfaceRepId aSurfId(static_cast<uint32_t>(aRawId.Index));
    theGraph->graph.Editor().Faces().SetSurfaceRepId(aFaceId, aSurfId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_replace_coedge_pcurve(occtl_graph_t* const  theGraph,
                                   const occtl_node_id_t theCoedge,
                                   const occtl_rep_id_t  thePcurveId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    if (thePcurveId.bits == 0)
    {
      theGraph->graph.Editor().CoEdges().SetPCurve(aCoEdgeId, occ::handle<Geom2d_Curve>());
      return OCCTL_OK;
    }

    const occ::handle<Geom2d_Curve>& aCurve2D = OcctL::Geom::Curve2DFromRep(theGraph, thePcurveId);
    theGraph->graph.Editor().CoEdges().SetPCurve(aCoEdgeId, aCurve2D);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_add_pcurve(occtl_graph_t* const      theGraph,
                                                          const occtl_node_id_t     theEdge,
                                                          const occtl_node_id_t     theFace,
                                                          const occtl_rep_id_t      thePcurveId,
                                                          const double              theFirst,
                                                          const double              theLast,
                                                          const occtl_orientation_t theOrientation)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(thePcurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "pcurve rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve2D");
      return OCCTL_WRONG_KIND;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    const occ::handle<Geom2d_Curve>& aCurve2D = OcctL::Geom::Curve2DFromRep(theGraph, thePcurveId);
    theGraph->graph.Editor().CoEdges().AddPCurve(anEdgeId,
                                                 aFaceId,
                                                 aCurve2D,
                                                 theFirst,
                                                 theLast,
                                                 OcctL::Topo::ToOcctOrientation(theOrientation));
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_shell_add_face(occtl_graph_t* const      theGraph,
                            const occtl_node_id_t     theShell,
                            const occtl_node_id_t     theFace,
                            const occtl_orientation_t theOrientation)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_ShellId aShellId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theShell, BRepGraph_NodeId::Kind::Shell, aShellId))
    {
      return aStatus;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    const BRepGraph_FaceRefId aRefId =
      theGraph->graph.Editor().Shells().AddFace(aShellId,
                                                aFaceId,
                                                OcctL::Topo::ToOcctOrientation(theOrientation));
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR, "Shells().AddFace returned invalid ref");
      return OCCTL_ERROR;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_shell_remove_face(occtl_graph_t* const  theGraph,
                                                                 const occtl_node_id_t theShell,
                                                                 const occtl_node_id_t theFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_ShellId aShellId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theShell, BRepGraph_NodeId::Kind::Shell, aShellId))
    {
      return aStatus;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    const NCollection_DynamicArray<BRepGraph_FaceRefId>& aFaceRefs =
      theGraph->graph.Refs().Faces().IdsOf(aShellId);
    for (const BRepGraph_FaceRefId& aRefId : aFaceRefs)
    {
      const BRepGraphInc::FaceRef& aFR = theGraph->graph.Refs().Faces().Entry(aRefId);
      if (!aFR.IsRemoved && aFR.FaceDefId == aFaceId)
      {
        (void)theGraph->graph.Editor().Shells().RemoveFace(aShellId, aRefId);
        return OCCTL_OK;
      }
    }

    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "face not found in shell");
    return OCCTL_NOT_FOUND;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_add_holes(occtl_graph_t* const         theGraph,
                                                              const occtl_node_id_t        theFace,
                                                              const occtl_node_id_t* const theHoles,
                                                              const size_t theHoleCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theHoles == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "holes is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theHoleCount == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "hole_count is zero");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    NCollection_LinearVector<BRepGraph_WireId> aRequestedHoles;
    aRequestedHoles.Reserve(theHoleCount);
    for (size_t anI = 0; anI < theHoleCount; ++anI)
    {
      for (size_t aPrev = 0; aPrev < anI; ++aPrev)
      {
        if (theHoles[aPrev].bits == theHoles[anI].bits)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "duplicate hole wire id");
          return OCCTL_INVALID_ARGUMENT;
        }
      }

      BRepGraph_WireId aWireId;
      if (const occtl_status_t aStatus =
            OcctL::Topo::ToTypedId(theGraph, theHoles[anI], BRepGraph_NodeId::Kind::Wire, aWireId))
      {
        return aStatus;
      }
      aRequestedHoles.Append(aWireId);
    }

    for (BRepGraph_RefsWireOfFace anIt(theGraph->graph, aFaceId); anIt.More(); anIt.Next())
    {
      const BRepGraphInc::WireRef& aRef = theGraph->graph.Refs().Wires().Entry(anIt.CurrentId());
      for (size_t anI = 0; anI < aRequestedHoles.Size(); ++anI)
      {
        const BRepGraph_WireId aWireId = aRequestedHoles.Value(anI);
        if (aRef.WireDefId == aWireId)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                                 "wire is already referenced by face");
          return OCCTL_INVALID_ARGUMENT;
        }
      }
    }

    for (size_t anI = 0; anI < aRequestedHoles.Size(); ++anI)
    {
      const BRepGraph_WireId    aWireId = aRequestedHoles.Value(anI);
      const BRepGraph_WireRefId aRefId =
        theGraph->graph.Editor().Faces().AddWire(aFaceId, aWireId, false, TopAbs_FORWARD);
      if (!aRefId.IsValid())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR, "Faces().AddWire returned invalid ref");
        return OCCTL_ERROR;
      }
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_remove_holes(occtl_graph_t* const         theGraph,
                               const occtl_node_id_t        theFace,
                               const occtl_node_id_t* const theHoles,
                               const size_t                 theHoleCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theHoleCount > 0 && theHoles == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "holes is NULL when hole_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theHoleCount == 0 && theHoles != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "holes is non-NULL when hole_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    NCollection_LinearVector<BRepGraph_WireId> aRequestedHoles;
    aRequestedHoles.Reserve(theHoleCount);
    for (size_t anI = 0; anI < theHoleCount; ++anI)
    {
      for (size_t aPrev = 0; aPrev < anI; ++aPrev)
      {
        if (theHoles[aPrev].bits == theHoles[anI].bits)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "duplicate hole wire id");
          return OCCTL_INVALID_ARGUMENT;
        }
      }

      BRepGraph_WireId aWireId;
      if (const occtl_status_t aStatus =
            OcctL::Topo::ToTypedId(theGraph, theHoles[anI], BRepGraph_NodeId::Kind::Wire, aWireId))
      {
        return aStatus;
      }
      aRequestedHoles.Append(aWireId);
    }

    NCollection_LinearVector<BRepGraph_WireRefId> aRefsToRemove;
    if (theHoleCount == 0)
    {
      for (BRepGraph_RefsWireOfFace anIt(theGraph->graph, aFaceId); anIt.More(); anIt.Next())
      {
        const BRepGraphInc::WireRef& aRef = theGraph->graph.Refs().Wires().Entry(anIt.CurrentId());
        if (!aRef.IsOuter)
        {
          aRefsToRemove.Append(anIt.CurrentId());
        }
      }
    }
    else
    {
      NCollection_Array1<unsigned char> aFound(aRequestedHoles.Size());
      for (size_t anI = 0; anI < aFound.Size(); ++anI)
      {
        aFound.ChangeAt(anI) = 0u;
      }
      for (BRepGraph_RefsWireOfFace anIt(theGraph->graph, aFaceId); anIt.More(); anIt.Next())
      {
        const BRepGraphInc::WireRef& aRef = theGraph->graph.Refs().Wires().Entry(anIt.CurrentId());
        for (size_t anI = 0; anI < aRequestedHoles.Size(); ++anI)
        {
          if (aRef.WireDefId == aRequestedHoles.Value(anI))
          {
            if (aRef.IsOuter)
            {
              OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                                     "requested wire is the outer wire");
              return OCCTL_NOT_FOUND;
            }
            aRefsToRemove.Append(anIt.CurrentId());
            aFound.ChangeAt(anI) = 1u;
          }
        }
      }

      for (size_t anI = 0; anI < aFound.Size(); ++anI)
      {
        if (aFound.At(anI) == 0u)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                                 "requested hole wire is not referenced by face");
          return OCCTL_NOT_FOUND;
        }
      }
    }

    for (size_t anI = 0; anI < aRefsToRemove.Size(); ++anI)
    {
      const BRepGraph_WireRefId aRefId = aRefsToRemove.Value(anI);
      if (!theGraph->graph.Editor().Faces().RemoveWire(aFaceId, aRefId))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "Faces().RemoveWire returned false");
        return OCCTL_NOT_FOUND;
      }
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_solid_add_shell(occtl_graph_t* const      theGraph,
                             const occtl_node_id_t     theSolid,
                             const occtl_node_id_t     theShell,
                             const occtl_orientation_t theOrientation)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_SolidId aSolidId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theSolid, BRepGraph_NodeId::Kind::Solid, aSolidId))
    {
      return aStatus;
    }

    BRepGraph_ShellId aShellId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theShell, BRepGraph_NodeId::Kind::Shell, aShellId))
    {
      return aStatus;
    }

    const BRepGraph_ShellRefId aRefId =
      theGraph->graph.Editor().Solids().AddShell(aSolidId,
                                                 aShellId,
                                                 OcctL::Topo::ToOcctOrientation(theOrientation));
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR, "Solids().AddShell returned invalid ref");
      return OCCTL_ERROR;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_solid_remove_shell(occtl_graph_t* const  theGraph,
                                                                  const occtl_node_id_t theSolid,
                                                                  const occtl_node_id_t theShell)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_SolidId aSolidId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theSolid, BRepGraph_NodeId::Kind::Solid, aSolidId))
    {
      return aStatus;
    }

    BRepGraph_ShellId aShellId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theShell, BRepGraph_NodeId::Kind::Shell, aShellId))
    {
      return aStatus;
    }

    const NCollection_DynamicArray<BRepGraph_ShellRefId>& aShellRefs =
      theGraph->graph.Refs().Shells().IdsOf(aSolidId);
    for (const BRepGraph_ShellRefId& aRefId : aShellRefs)
    {
      const BRepGraphInc::ShellRef& aSR = theGraph->graph.Refs().Shells().Entry(aRefId);
      if (!aSR.IsRemoved && aSR.ShellDefId == aShellId)
      {
        (void)theGraph->graph.Editor().Solids().RemoveShell(aSolidId, aRefId);
        return OCCTL_OK;
      }
    }

    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "shell not found in solid");
    return OCCTL_NOT_FOUND;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_compound_add_child(occtl_graph_t* const      theGraph,
                                const occtl_node_id_t     theCompound,
                                const occtl_node_id_t     theChild,
                                const occtl_orientation_t theOrientation)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CompoundId aCompId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCompound, BRepGraph_NodeId::Kind::Compound, aCompId))
    {
      return aStatus;
    }

    const BRepGraph_NodeId aChildNodeId = OcctL::Topo::UnpackNodeId(theChild);
    if (!aChildNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "child NodeId is invalid");
      return OCCTL_NOT_FOUND;
    }

    const BRepGraph_ChildRefId aRefId =
      theGraph->graph.Editor().Compounds().AddChild(aCompId,
                                                    aChildNodeId,
                                                    OcctL::Topo::ToOcctOrientation(theOrientation));
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR,
                                             "Compounds().AddChild returned invalid ref");
      return OCCTL_ERROR;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_compound_remove_child(occtl_graph_t* const  theGraph,
                                   const occtl_node_id_t theCompound,
                                   const occtl_node_id_t theChild)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CompoundId aCompId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCompound, BRepGraph_NodeId::Kind::Compound, aCompId))
    {
      return aStatus;
    }

    const BRepGraph_NodeId aChildNodeId = OcctL::Topo::UnpackNodeId(theChild);
    if (!aChildNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "child NodeId is invalid");
      return OCCTL_NOT_FOUND;
    }

    const NCollection_DynamicArray<BRepGraph_ChildRefId>& aChildRefs =
      theGraph->graph.Refs().Children().IdsOf(aCompId);
    for (const BRepGraph_ChildRefId& aRefId : aChildRefs)
    {
      const BRepGraphInc::ChildRef& aCR = theGraph->graph.Refs().Children().Entry(aRefId);
      if (!aCR.IsRemoved && aCR.ChildDefId == aChildNodeId)
      {
        (void)theGraph->graph.Editor().Compounds().RemoveChild(aCompId, aRefId);
        return OCCTL_OK;
      }
    }

    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "child not found in compound");
    return OCCTL_NOT_FOUND;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_split(occtl_graph_t* const   theGraph,
                                                          const occtl_node_id_t  theEdge,
                                                          const double           theParameter,
                                                          occtl_node_id_t* const theOutEdge1,
                                                          occtl_node_id_t* const theOutEdge2)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutEdge1 == nullptr || theOutEdge2 == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out-param is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    if (!BRepGraph_Tool::Edge::HasCurve(theGraph->graph, anEdgeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "edge has no 3D curve; cannot split");
      return OCCTL_NOT_FOUND;
    }

    const double aTolerance = BRepGraph_Tool::Edge::Tolerance(theGraph->graph, anEdgeId);
    GeomAdaptor_TransformedCurve anAdaptor =
      BRepGraph_Tool::Edge::CurveAdaptor(theGraph->graph, anEdgeId);
    const gp_Pnt aSplitPoint = anAdaptor.EvalD0(theParameter);

    const BRepGraph_VertexId aSplitVertex =
      theGraph->graph.Editor().Vertices().Add(aSplitPoint, aTolerance);
    if (!aSplitVertex.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR, "Vertices().Add returned invalid");
      return OCCTL_ERROR;
    }

    BRepGraph_EdgeId aSubA, aSubB;
    theGraph->graph.Editor().Edges().Split(anEdgeId, aSplitVertex, theParameter, aSubA, aSubB);
    if (!aSubA.IsValid() || !aSubB.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR, "Edges().Split returned invalid edge");
      return OCCTL_ERROR;
    }

    *theOutEdge1 = OcctL::Topo::PackNodeId(aSubA);
    *theOutEdge2 = OcctL::Topo::PackNodeId(aSubB);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_add_internal_vertex(occtl_graph_t* const  theGraph,
                                      const occtl_node_id_t theEdge,
                                      const occtl_node_id_t theVertex)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    const BRepGraph_VertexRefId aRefId =
      theGraph->graph.Editor().Edges().AddInternalVertex(anEdgeId, aVertId, TopAbs_INTERNAL);
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR,
                                             "Edges().AddInternalVertex returned invalid ref");
      return OCCTL_ERROR;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_remove_vertex(occtl_graph_t* const  theGraph,
                                                                  const occtl_node_id_t theEdge,
                                                                  const occtl_node_id_t theVertex)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    const BRepGraphInc::EdgeDef& anEdgeDef = theGraph->graph.Topo().Edges().Definition(anEdgeId);

    auto checkRef = [&](const BRepGraph_VertexRefId theRefId) -> bool {
      if (!theRefId.IsValid())
      {
        return false;
      }
      const BRepGraphInc::VertexRef& aVR = theGraph->graph.Refs().Vertices().Entry(theRefId);
      if (!aVR.IsRemoved && aVR.VertexDefId == aVertId)
      {
        (void)theGraph->graph.Editor().Edges().RemoveVertex(anEdgeId, theRefId);
        return true;
      }
      return false;
    };

    if (checkRef(anEdgeDef.StartVertexRefId))
    {
      return OCCTL_OK;
    }
    if (checkRef(anEdgeDef.EndVertexRefId))
    {
      return OCCTL_OK;
    }

    for (const BRepGraph_VertexRefId& aRefId : anEdgeDef.InternalVertexRefIds)
    {
      if (checkRef(aRefId))
      {
        return OCCTL_OK;
      }
    }

    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "vertex not found on edge");
    return OCCTL_NOT_FOUND;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_curves_to_wire(occtl_graph_t* const   theGraph,
                                                              const occtl_rep_id_t*  theCurveIds,
                                                              const size_t           theCount,
                                                              occtl_node_id_t* const theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutWire == nullptr || theCount == 0 || theCurveIds == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_wire, or curve_ids is NULL or count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_DynamicArray<std::pair<BRepGraph_EdgeId, TopAbs_Orientation>> aPairs;

    BRepGraph_VertexId aPrevEndVert;
    gp_Pnt             aPrevEndPnt;
    bool               aHasPrev = false;

    for (size_t anI = 0; anI < theCount; ++anI)
    {
      const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveIds[anI]);
      if (!aRawId.IsValid())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve rep id is invalid");
        return OCCTL_NOT_FOUND;
      }
      if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve3D");
        return OCCTL_WRONG_KIND;
      }
      BRepGraph_Curve3DRepId         aCurve3DId(static_cast<uint32_t>(aRawId.Index));
      const occ::handle<Geom_Curve>& aGeomCurve =
        theGraph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;

      const double aU1       = aGeomCurve->FirstParameter();
      const double aU2       = aGeomCurve->LastParameter();
      const gp_Pnt aStartPnt = aGeomCurve->Value(aU1);
      const gp_Pnt anEndPnt  = aGeomCurve->Value(aU2);
      const bool   aIsClosed =
        aGeomCurve->IsClosed() || aStartPnt.Distance(anEndPnt) < Precision::Confusion();

      BRepGraph_VertexId aStartVert;
      if (!aHasPrev || aPrevEndPnt.Distance(aStartPnt) >= Precision::Confusion())
      {
        aStartVert = theGraph->graph.Editor().Vertices().Add(aStartPnt, Precision::Confusion());
      }
      else
      {
        aStartVert = aPrevEndVert;
      }

      BRepGraph_VertexId anEndVert;
      if (aIsClosed)
      {
        anEndVert = aStartVert;
      }
      else
      {
        anEndVert = theGraph->graph.Editor().Vertices().Add(anEndPnt, Precision::Confusion());
      }

      const BRepGraph_EdgeId anEdgeId =
        theGraph->graph.Editor().Edges().Add(aStartVert,
                                             anEndVert,
                                             aGeomCurve,
                                             aU1,
                                             aU2,
                                             Precision::Confusion());

      aPairs.Append(std::make_pair(anEdgeId, TopAbs_FORWARD));

      aPrevEndVert = anEndVert;
      aPrevEndPnt  = anEndPnt;
      aHasPrev     = true;
    }

    const BRepGraph_WireId aWireId = theGraph->graph.Editor().Wires().Add(aPairs);
    *theOutWire                    = OcctL::Topo::PackNodeId(aWireId);
    return OCCTL_OK;
  });
}

} // extern "C"
