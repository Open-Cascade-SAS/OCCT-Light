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

#include <occtl/occtl_prim.h>

#include <BRepGraphAlgo_SameParameter.hxx>
#include <BRepGraphAlgo_Sewing.hxx>
#include <BRepGraphCheck_Analyzer.hxx>
#include <BRepGraphCheck_CheckView.hxx>
#include <BRepGraphCheck_Issue.hxx>
#include <BRepGraph_DefsIterator.hxx>
#include <BRepGraph_Iterator.hxx>
#include <BRepGraph_TopoView.hxx>
#include <NCollection_DynamicArray.hxx>
#include <NCollection_IndexedDataMap.hxx>
#include <NCollection_IndexedMap.hxx>
#include <NCollection_LinearVector.hxx>
#include <NCollection_List.hxx>
#include <Precision.hxx>

#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgo.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Defeaturing.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepAlgoAPI_Splitter.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepGProp.hxx>
#include <BRepGraph_LayerHistory.hxx>
#include <BRepGraph_LayerRegistry.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_UIDsView.hxx>
#include <BRepLib.hxx>
#include <BRepOffsetAPI_DraftAngle.hxx>
#include <BRepOffsetAPI_MakeFilling.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepOffsetAPI_NormalProjection.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepOffset_MakeOffset.hxx>
#include <BRepOffset_Mode.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRepTools_History.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomAbs_JoinType.hxx>
#include <GeomAbs_Shape.hxx>
#include <Geom_Surface.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <HLRBRep_PolyHLRToShape.hxx>
#include <NCollection_HArray1.hxx>
#include <Standard_ErrorHandler.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <ShapeFix_Wire.hxx>
#include <TopTools_ShapeMapHasher.hxx>

#include <occtl/occtl_topo_algo.h>

#include "../geom/GeomMath.hxx"
#include "../geom/RepLookup.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

#include <TCollection_AsciiString.hxx>

namespace
{

bool IsFiniteValue(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

BRepGraphAlgo_Sewing::Options ToOcctSewOptions(const occtl_topo_sew_options_t* const theOpts)
{
  BRepGraphAlgo_Sewing::Options aOut;
  if (theOpts == nullptr)
  {
    return aOut;
  }
  aOut.Tolerance           = theOpts->tolerance;
  aOut.Cutting             = theOpts->cutting != 0;
  aOut.SameParameterMode   = theOpts->same_parameter_mode != 0;
  aOut.NonManifoldMode     = theOpts->non_manifold_mode != 0;
  aOut.Parallel            = theOpts->parallel != 0;
  aOut.HistoryMode         = theOpts->history_mode != 0;
  aOut.FaceAnalysis        = theOpts->face_analysis != 0;
  aOut.FloatingEdgesMode   = theOpts->floating_edges_mode != 0;
  aOut.LocalTolerancesMode = theOpts->local_tolerances_mode != 0;
  if (theOpts->min_tolerance > 0.0)
  {
    aOut.MinTolerance = theOpts->min_tolerance;
  }
  if (theOpts->max_tolerance > 0.0)
  {
    aOut.MaxTolerance = theOpts->max_tolerance;
  }
  return aOut;
}

void FillSewResult(occtl_topo_sew_result_t* const      theOutResult,
                   const BRepGraphAlgo_Sewing::Result& theResult)
{
  if (theOutResult == nullptr)
  {
    return;
  }
  theOutResult->is_done                     = theResult.IsDone ? 1 : 0;
  theOutResult->free_edge_count_before      = theResult.NbFreeEdgesBefore;
  theOutResult->free_edge_count_after       = theResult.NbFreeEdgesAfter;
  theOutResult->sewn_edge_count             = theResult.NbSewnEdges;
  theOutResult->multiple_edge_count         = theResult.NbMultipleEdges;
  theOutResult->degenerated_edge_count      = theResult.NbDegeneratedEdges;
  theOutResult->deleted_face_count          = theResult.NbDeletedFaces;
  theOutResult->rejected_by_tolerance_count = theResult.NbRejectedByTolerance;
}

occtl_topo_check_severity_t ToAbiSeverity(const BRepGraphCheck_Issue::Severity theSev)
{
  switch (theSev)
  {
    case BRepGraphCheck_Issue::Severity::Warning:
      return OCCTL_TOPO_CHECK_WARNING;
    case BRepGraphCheck_Issue::Severity::Error:
      return OCCTL_TOPO_CHECK_ERROR;
    case BRepGraphCheck_Issue::Severity::Fatal:
      return OCCTL_TOPO_CHECK_FATAL;
  }
  return OCCTL_TOPO_CHECK_ERROR;
}

occtl_status_t ResolveShape(const occtl_graph_t* const theGraph,
                            const occtl_node_id_t      theRoot,
                            const char* const          theLabel,
                            BRepGraph_NodeId&          theOutNode,
                            TopoDS_Shape&              theOutShape)
{
  theOutNode = OcctL::Topo::UnpackNodeId(theRoot);
  if (!theOutNode.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " NodeId is invalid or removed"));
    return OCCTL_NOT_FOUND;
  }

  theOutShape = theGraph->graph.Shapes().Shape(theOutNode);
  if (theOutShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " could not be reconstructed as TopoDS shape"));
    return OCCTL_NOT_FOUND;
  }
  return OCCTL_OK;
}

occtl_status_t AddCopiedResult(
  const TopoDS_Shape&                                                                 theShape,
  const char* const                                                                   theLabel,
  occtl_graph_t**                                                                     theOutGraph,
  occtl_node_id_t*                                                                    theOutRoot,
  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>* const theOutAdded =
    nullptr)
{
  if (theShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_GEOMETRY_INVALID,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel) + " result shape is null"));
    return OCCTL_GEOMETRY_INVALID;
  }

  occtl_graph*                   aNewGraph = new occtl_graph();
  BRepGraph::ShapesView::Options anBuildOpts;
  anBuildOpts.CreateAutoProduct = false;
  anBuildOpts.TrackAddedNodes   = true;
  const BRepGraph::ShapesView::Result aBuildRes =
    aNewGraph->graph.Shapes().Add(theShape, anBuildOpts);
  if (!aBuildRes.Ok)
  {
    delete aNewGraph;
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " result topology could not be merged into graph"));
    return OCCTL_TOPOLOGY_INVALID;
  }

  *theOutGraph = aNewGraph;
  *theOutRoot  = OcctL::Topo::PackNodeId(aBuildRes.TopologyRoot);
  if (theOutAdded != nullptr)
  {
    *theOutAdded = aBuildRes.AddedNodes;
  }
  return OCCTL_OK;
}

occtl_topo_hlr_result_t EmptyHlrResult()
{
  occtl_topo_hlr_result_t aResult = OCCTL_TOPO_HLR_RESULT_INIT;
  return aResult;
}

bool ShapeHasEdges(const TopoDS_Shape& theShape)
{
  if (theShape.IsNull())
  {
    return false;
  }
  for (TopExp_Explorer anExp(theShape, TopAbs_EDGE); anExp.More(); anExp.Next())
  {
    return true;
  }
  return false;
}

bool DirectionIsFiniteNonZero(const occtl_direction3_t& theDirection)
{
  const gp_Vec aVec(theDirection.x, theDirection.y, theDirection.z);
  const double aSquare = aVec.SquareMagnitude();
  return IsFiniteValue(theDirection.x) && IsFiniteValue(theDirection.y)
         && IsFiniteValue(theDirection.z) && IsFiniteValue(aSquare)
         && aSquare > Precision::SquareConfusion();
}

bool VectorIsFiniteNonZero(const occtl_vector3_t& theVector)
{
  const gp_Vec aVec(theVector.x, theVector.y, theVector.z);
  const double aSquare = aVec.SquareMagnitude();
  return IsFiniteValue(theVector.x) && IsFiniteValue(theVector.y) && IsFiniteValue(theVector.z)
         && IsFiniteValue(aSquare) && aSquare > Precision::SquareConfusion();
}

bool PointIsFinite(const occtl_point3_t& thePoint)
{
  return IsFiniteValue(thePoint.x) && IsFiniteValue(thePoint.y) && IsFiniteValue(thePoint.z);
}

bool ProjectionFrameIsValid(const occtl_axis3_placement_t& theFrame)
{
  if (!PointIsFinite(theFrame.location) || !DirectionIsFiniteNonZero(theFrame.x_dir)
      || !DirectionIsFiniteNonZero(theFrame.z_dir))
  {
    return false;
  }

  const gp_Vec anX(theFrame.x_dir.x, theFrame.x_dir.y, theFrame.x_dir.z);
  const gp_Vec aZ(theFrame.z_dir.x, theFrame.z_dir.y, theFrame.z_dir.z);
  return anX.Crossed(aZ).SquareMagnitude() > Precision::SquareConfusion();
}

occtl_status_t AddHlrCategory(occtl_graph_t&    theGraph,
                              TopoDS_Shape      theShape,
                              const char* const theLabel,
                              occtl_node_id_t&  theOutRoot,
                              int&              theOutCount)
{
  theOutRoot = OCCTL_NODE_ID_INVALID;
  if (!ShapeHasEdges(theShape))
  {
    return OCCTL_OK;
  }

  BRepLib::BuildCurves3d(theShape);

  BRepGraph::ShapesView::Options aBuildOpts;
  aBuildOpts.CreateAutoProduct = false;
  const BRepGraph::ShapesView::Result aBuildResult =
    theGraph.graph.Shapes().Add(theShape, aBuildOpts);
  if (!aBuildResult.Ok || !aBuildResult.TopologyRoot.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " HLR category could not be merged into graph"));
    return OCCTL_TOPOLOGY_INVALID;
  }

  theOutRoot = OcctL::Topo::PackNodeId(aBuildResult.TopologyRoot);
  ++theOutCount;
  return OCCTL_OK;
}

template <typename HlrToShapeT>
occtl_status_t ExtractHlrCategories(HlrToShapeT&                    theHlrShapes,
                                    const occtl_topo_hlr_options_t& theOpts,
                                    occtl_graph_t&                  theResultGraph,
                                    occtl_topo_hlr_result_t&        theResult,
                                    int&                            theCategoryCount)
{
  if (const occtl_status_t aStatus = AddHlrCategory(theResultGraph,
                                                    theHlrShapes.VCompound(),
                                                    "visible sharp",
                                                    theResult.visible_sharp,
                                                    theCategoryCount))
  {
    return aStatus;
  }
  if (theOpts.include_smooth != 0)
  {
    if (const occtl_status_t aStatus = AddHlrCategory(theResultGraph,
                                                      theHlrShapes.Rg1LineVCompound(),
                                                      "visible smooth",
                                                      theResult.visible_smooth,
                                                      theCategoryCount))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus = AddHlrCategory(theResultGraph,
                                                      theHlrShapes.RgNLineVCompound(),
                                                      "visible seam",
                                                      theResult.visible_seam,
                                                      theCategoryCount))
    {
      return aStatus;
    }
  }
  if (theOpts.include_outline != 0)
  {
    if (const occtl_status_t aStatus = AddHlrCategory(theResultGraph,
                                                      theHlrShapes.OutLineVCompound(),
                                                      "visible outline",
                                                      theResult.visible_outline,
                                                      theCategoryCount))
    {
      return aStatus;
    }
  }
  if (theOpts.include_hidden != 0)
  {
    if (const occtl_status_t aStatus = AddHlrCategory(theResultGraph,
                                                      theHlrShapes.HCompound(),
                                                      "hidden sharp",
                                                      theResult.hidden_sharp,
                                                      theCategoryCount))
    {
      return aStatus;
    }
    if (theOpts.include_smooth != 0)
    {
      if (const occtl_status_t aStatus = AddHlrCategory(theResultGraph,
                                                        theHlrShapes.Rg1LineHCompound(),
                                                        "hidden smooth",
                                                        theResult.hidden_smooth,
                                                        theCategoryCount))
      {
        return aStatus;
      }
      if (const occtl_status_t aStatus = AddHlrCategory(theResultGraph,
                                                        theHlrShapes.RgNLineHCompound(),
                                                        "hidden seam",
                                                        theResult.hidden_seam,
                                                        theCategoryCount))
      {
        return aStatus;
      }
    }
    if (theOpts.include_outline != 0)
    {
      if (const occtl_status_t aStatus = AddHlrCategory(theResultGraph,
                                                        theHlrShapes.OutLineHCompound(),
                                                        "hidden outline",
                                                        theResult.hidden_outline,
                                                        theCategoryCount))
      {
        return aStatus;
      }
    }
  }
  return OCCTL_OK;
}

HLRAlgo_Projector MakeHlrProjector(const gp_Ax2&                   theProjectionFrame,
                                   const occtl_topo_hlr_options_t& theOpts)
{
  if (theOpts.focus > 0.0)
  {
    return HLRAlgo_Projector(theProjectionFrame, theOpts.focus);
  }
  return HLRAlgo_Projector(theProjectionFrame);
}

occtl_status_t RunBRepHlr(const TopoDS_Shape&             theRootShape,
                          const gp_Ax2&                   theProjectionFrame,
                          const occtl_topo_hlr_options_t& theOpts,
                          occtl_graph_t&                  theResultGraph,
                          occtl_topo_hlr_result_t&        theResult,
                          int&                            theCategoryCount)
{
  occ::handle<HLRBRep_Algo> anAlgo = new HLRBRep_Algo();
  anAlgo->Add(theRootShape);
  anAlgo->Projector(MakeHlrProjector(theProjectionFrame, theOpts));
  anAlgo->Update();
  anAlgo->Hide();

  HLRBRep_HLRToShape anHlrShapes(anAlgo);
  return ExtractHlrCategories(anHlrShapes, theOpts, theResultGraph, theResult, theCategoryCount);
}

occtl_status_t RunPolyHlr(const TopoDS_Shape&             theRootShape,
                          const gp_Ax2&                   theProjectionFrame,
                          const occtl_topo_hlr_options_t& theOpts,
                          occtl_graph_t&                  theResultGraph,
                          occtl_topo_hlr_result_t&        theResult,
                          int&                            theCategoryCount)
{
  occ::handle<HLRBRep_PolyAlgo> anAlgo = new HLRBRep_PolyAlgo();
  anAlgo->Projector(MakeHlrProjector(theProjectionFrame, theOpts));
  anAlgo->Load(theRootShape);
  anAlgo->Update();

  HLRBRep_PolyHLRToShape anHlrShapes;
  anHlrShapes.Update(anAlgo);
  return ExtractHlrCategories(anHlrShapes, theOpts, theResultGraph, theResult, theCategoryCount);
}

GeomAbs_Shape ToGeomAbsContinuity(const occtl_topo_filling_continuity_t theContinuity)
{
  switch (theContinuity)
  {
    case OCCTL_TOPO_FILLING_C0:
      return GeomAbs_C0;
    case OCCTL_TOPO_FILLING_G1:
      return GeomAbs_G1;
    case OCCTL_TOPO_FILLING_G2:
      return GeomAbs_G2;
    case OCCTL_TOPO_FILLING_CONTINUITY_RESERVED_FUTURE:
      break;
  }
  return GeomAbs_C0;
}

GeomAbs_JoinType ToGeomAbsJoin(const occtl_offset_join_type_t theJoin)
{
  switch (theJoin)
  {
    case OCCTL_OFFSET_JOIN_TANGENT:
      return GeomAbs_Tangent;
    case OCCTL_OFFSET_JOIN_INTERSECTION:
      return GeomAbs_Intersection;
    case OCCTL_OFFSET_JOIN_ARC:
      return GeomAbs_Arc;
    case OCCTL_OFFSET_JOIN_RESERVED_FUTURE:
      break;
  }
  return GeomAbs_Arc;
}

GeomAbs_JoinType ToWireOffsetJoin(const occtl_topo_wire_offset_2d_join_t theJoin)
{
  switch (theJoin)
  {
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_TANGENT:
      return GeomAbs_Tangent;
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION:
      return GeomAbs_Intersection;
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC:
      return GeomAbs_Arc;
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_RESERVED_FUTURE:
      break;
  }
  return GeomAbs_Arc;
}

bool IsValidWireOffsetJoin(const occtl_topo_wire_offset_2d_join_t theJoin)
{
  return theJoin == OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC
         || theJoin == OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_TANGENT
         || theJoin == OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION;
}

bool IsValidBrakeSide(const occtl_prim_brake_side_t theSide)
{
  return theSide == OCCTL_PRIM_BRAKE_SIDE_LEFT || theSide == OCCTL_PRIM_BRAKE_SIDE_RIGHT;
}

bool FirstWire(const TopoDS_Shape& theShape, TopoDS_Wire& theOutWire)
{
  if (theShape.IsNull())
  {
    return false;
  }
  if (theShape.ShapeType() == TopAbs_WIRE)
  {
    theOutWire = TopoDS::Wire(theShape);
    return true;
  }

  TopExp_Explorer anExplorer(theShape, TopAbs_WIRE);
  if (!anExplorer.More())
  {
    return false;
  }
  theOutWire = TopoDS::Wire(anExplorer.Current());
  return true;
}

bool FirstFace(const TopoDS_Shape& theShape, TopoDS_Face& theOutFace)
{
  if (theShape.IsNull())
  {
    return false;
  }
  if (theShape.ShapeType() == TopAbs_FACE)
  {
    theOutFace = TopoDS::Face(theShape);
    return true;
  }

  TopExp_Explorer anExplorer(theShape, TopAbs_FACE);
  if (!anExplorer.More())
  {
    return false;
  }
  theOutFace = TopoDS::Face(anExplorer.Current());
  return true;
}

bool LineShapeToWire(const TopoDS_Shape& theShape, TopoDS_Wire& theOutWire)
{
  if (theShape.IsNull())
  {
    return false;
  }
  if (theShape.ShapeType() == TopAbs_WIRE)
  {
    theOutWire = TopoDS::Wire(theShape);
    return true;
  }
  if (theShape.ShapeType() == TopAbs_EDGE)
  {
    BRepBuilderAPI_MakeWire aWireMaker(TopoDS::Edge(theShape));
    if (!aWireMaker.IsDone())
    {
      return false;
    }
    theOutWire = aWireMaker.Wire();
    return !theOutWire.IsNull();
  }
  return false;
}

NCollection_LinearVector<TopoDS_Vertex> OrderedWireVertices(const TopoDS_Wire& theWire)
{
  NCollection_LinearVector<TopoDS_Vertex> aVertices;
  BRepTools_WireExplorer                  anExplorer(theWire);
  for (; anExplorer.More(); anExplorer.Next())
  {
    const TopoDS_Edge anEdge = anExplorer.Current();
    TopoDS_Vertex     aFirst;
    TopoDS_Vertex     aLast;
    TopExp::Vertices(anEdge, aFirst, aLast, true);
    if (anEdge.Orientation() == TopAbs_REVERSED)
    {
      std::swap(aFirst, aLast);
    }

    if (aVertices.IsEmpty())
    {
      aVertices.Append(aFirst);
    }
    aVertices.Append(aLast);
  }
  return aVertices;
}

bool FindThicknessMatchedVertex(const NCollection_LinearVector<TopoDS_Vertex>& theCandidates,
                                const gp_Pnt&                                  thePoint,
                                const double                                   theThickness,
                                TopoDS_Vertex&                                 theOutVertex)
{
  double anError = std::numeric_limits<double>::max();
  for (size_t aCandidateIndex = 0; aCandidateIndex < theCandidates.Size(); ++aCandidateIndex)
  {
    const TopoDS_Vertex& aCandidate      = theCandidates.Value(aCandidateIndex);
    const gp_Pnt         aCandidatePoint = BRep_Tool::Pnt(aCandidate);
    const double         aDistance       = thePoint.Distance(aCandidatePoint);
    const double         aCandidateError = std::abs(aDistance - theThickness);
    if (aCandidateError < anError)
    {
      anError      = aCandidateError;
      theOutVertex = aCandidate;
    }
  }
  return !theOutVertex.IsNull() && anError <= std::max(1.0e-5, theThickness * 1.0e-3);
}

occtl_status_t ComputeSplitPlaneEnvelope(const TopoDS_Shape& theShape,
                                         const gp_Pnt&       thePoint,
                                         double&             theOutExtent)
{
  Bnd_Box aBox;
  BRepBndLib::Add(theShape, aBox);
  if (aBox.IsVoid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "split root has no finite bounding box");
    return OCCTL_GEOMETRY_INVALID;
  }

  double aXMin = 0.0;
  double aYMin = 0.0;
  double aZMin = 0.0;
  double aXMax = 0.0;
  double aYMax = 0.0;
  double aZMax = 0.0;
  aBox.Get(aXMin, aYMin, aZMin, aXMax, aYMax, aZMax);
  if (!IsFiniteValue(aXMin) || !IsFiniteValue(aYMin) || !IsFiniteValue(aZMin)
      || !IsFiniteValue(aXMax) || !IsFiniteValue(aYMax) || !IsFiniteValue(aZMax))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "split root bounding box is not finite");
    return OCCTL_GEOMETRY_INVALID;
  }

  const gp_Pnt aMin(aXMin, aYMin, aZMin);
  const gp_Pnt aMax(aXMax, aYMax, aZMax);
  const double aDiag        = aMin.Distance(aMax);
  double       aMaxDistance = aDiag;
  const gp_Pnt aCorners[8]  = {gp_Pnt(aXMin, aYMin, aZMin),
                               gp_Pnt(aXMin, aYMin, aZMax),
                               gp_Pnt(aXMin, aYMax, aZMin),
                               gp_Pnt(aXMin, aYMax, aZMax),
                               gp_Pnt(aXMax, aYMin, aZMin),
                               gp_Pnt(aXMax, aYMin, aZMax),
                               gp_Pnt(aXMax, aYMax, aZMin),
                               gp_Pnt(aXMax, aYMax, aZMax)};
  for (const gp_Pnt& aCorner : aCorners)
  {
    aMaxDistance = std::max(aMaxDistance, thePoint.Distance(aCorner));
  }

  theOutExtent = std::max(1.0, aMaxDistance * 2.0);
  return OCCTL_OK;
}

occtl_status_t ComputeProjectionDistance(const TopoDS_Shape& theSource,
                                         const TopoDS_Shape& theTarget,
                                         double&             theOutDistance)
{
  Bnd_Box aBox;
  BRepBndLib::Add(theSource, aBox);
  BRepBndLib::Add(theTarget, aBox);
  if (aBox.IsVoid())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_GEOMETRY_INVALID,
      "projection source and target have no finite bounding box");
    return OCCTL_GEOMETRY_INVALID;
  }

  double aXMin = 0.0;
  double aYMin = 0.0;
  double aZMin = 0.0;
  double aXMax = 0.0;
  double aYMax = 0.0;
  double aZMax = 0.0;
  aBox.Get(aXMin, aYMin, aZMin, aXMax, aYMax, aZMax);
  if (!IsFiniteValue(aXMin) || !IsFiniteValue(aYMin) || !IsFiniteValue(aZMin)
      || !IsFiniteValue(aXMax) || !IsFiniteValue(aYMax) || !IsFiniteValue(aZMax))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_GEOMETRY_INVALID,
      "projection source or target bounding box is not finite");
    return OCCTL_GEOMETRY_INVALID;
  }

  const gp_Pnt aMin(aXMin, aYMin, aZMin);
  const gp_Pnt aMax(aXMax, aYMax, aZMax);
  theOutDistance = std::max(1.0, aMin.Distance(aMax) * 2.0);
  return OCCTL_OK;
}

occtl_status_t ProjectionTargetBoundary(const TopoDS_Shape& theTarget, TopoDS_Shape& theOutBoundary)
{
  if (theTarget.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                           "target could not be reconstructed as TopoDS shape");
    return OCCTL_NOT_FOUND;
  }

  if (theTarget.ShapeType() == TopAbs_FACE || theTarget.ShapeType() == TopAbs_SHELL)
  {
    theOutBoundary = theTarget;
    return OCCTL_OK;
  }

  TopoDS_Compound aCompound;
  BRep_Builder    aBuilder;
  aBuilder.MakeCompound(aCompound);

  size_t aFaceCount = 0;
  for (TopExp_Explorer anExp(theTarget, TopAbs_FACE); anExp.More(); anExp.Next())
  {
    aBuilder.Add(aCompound, anExp.Current());
    ++aFaceCount;
  }
  if (aFaceCount == 0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "target has no boundary faces");
    return OCCTL_WRONG_KIND;
  }

  theOutBoundary = aCompound;
  return OCCTL_OK;
}

TopoDS_Face MakeSplitPlaneFace(const gp_Pnt& thePoint,
                               const gp_Dir& theNormal,
                               const double  theExtent)
{
  const gp_Pln            aPlane(thePoint, theNormal);
  BRepBuilderAPI_MakeFace aMakeFace(aPlane, -theExtent, theExtent, -theExtent, theExtent);
  return aMakeFace.Face();
}

bool IsFiniteNonZeroDirection(const occtl_direction3_t& theDirection)
{
  const gp_Vec aVec(theDirection.x, theDirection.y, theDirection.z);
  const double aSq = aVec.SquareMagnitude();
  return IsFiniteValue(theDirection.x) && IsFiniteValue(theDirection.y)
         && IsFiniteValue(theDirection.z) && IsFiniteValue(aSq)
         && aSq > Precision::SquareConfusion();
}

bool IsFinitePoint(const occtl_point3_t& thePoint)
{
  return IsFiniteValue(thePoint.x) && IsFiniteValue(thePoint.y) && IsFiniteValue(thePoint.z);
}

bool IsNonParallel(const occtl_direction3_t& theA, const occtl_direction3_t& theB)
{
  gp_Vec aVecA(theA.x, theA.y, theA.z);
  gp_Vec aVecB(theB.x, theB.y, theB.z);
  if (aVecA.SquareMagnitude() <= Precision::SquareConfusion()
      || aVecB.SquareMagnitude() <= Precision::SquareConfusion())
  {
    return false;
  }
  aVecA.Normalize();
  aVecB.Normalize();
  return std::abs(aVecA.Dot(aVecB)) < 1.0 - Precision::Angular();
}

double EdgeLength(const TopoDS_Edge& theEdge)
{
  GProp_GProps aProps;
  BRepGProp::LinearProperties(theEdge, aProps);
  return aProps.Mass();
}

bool TrialFilletRadius(const TopoDS_Shape&                          theRootShape,
                       const NCollection_LinearVector<TopoDS_Edge>& theEdges,
                       const double                                 theRadius)
{
  try
  {
    OCC_CATCH_SIGNALS;
    BRepFilletAPI_MakeFillet aFillet(theRootShape);
    for (size_t anEdgeIndex = 0; anEdgeIndex < theEdges.Size(); ++anEdgeIndex)
    {
      const TopoDS_Edge& anEdge = theEdges.Value(anEdgeIndex);
      aFillet.Add(theRadius, anEdge);
    }
    aFillet.Build();
    return aFillet.IsDone() && !aFillet.Shape().IsNull();
  }
  catch (...)
  {
    return false;
  }
}

occtl_status_t ResolveSelectedRootEdges(const occtl_graph_t* const             theGraph,
                                        const TopoDS_Shape&                    theRootShape,
                                        const occtl_node_id_t*                 theEdges,
                                        const size_t                           theEdgeCount,
                                        NCollection_LinearVector<TopoDS_Edge>& theOutEdges)
{
  NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> aRootEdges;
  TopExp::MapShapes(theRootShape, TopAbs_EDGE, aRootEdges);

  theOutEdges.Clear();
  for (size_t anIdx = 0; anIdx < theEdgeCount; ++anIdx)
  {
    BRepGraph_NodeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdges[anIdx], BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    const TopoDS_Shape anEdgeShape = theGraph->graph.Shapes().Shape(anEdgeId);
    if (anEdgeShape.IsNull() || anEdgeShape.ShapeType() != TopAbs_EDGE)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "selected edge could not be reconstructed as TopoDS_Edge");
      return OCCTL_NOT_FOUND;
    }

    if (!aRootEdges.Contains(anEdgeShape))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "selected edge is not under root");
      return OCCTL_NOT_FOUND;
    }
    theOutEdges.Append(TopoDS::Edge(anEdgeShape));
  }

  return OCCTL_OK;
}

bool FaceMidNormal(const TopoDS_Face& theFace, gp_Dir& theOutNormal)
{
  BRepAdaptor_Surface anAdaptor(theFace);
  const double        aUMin = anAdaptor.FirstUParameter();
  const double        aUMax = anAdaptor.LastUParameter();
  const double        aVMin = anAdaptor.FirstVParameter();
  const double        aVMax = anAdaptor.LastVParameter();

  gp_Pnt aPoint;
  gp_Vec aDU;
  gp_Vec aDV;
  anAdaptor.D1((aUMin + aUMax) * 0.5, (aVMin + aVMax) * 0.5, aPoint, aDU, aDV);

  gp_Vec aNormal = aDU.Crossed(aDV);
  if (aNormal.SquareMagnitude() <= Precision::SquareConfusion())
  {
    return false;
  }
  if (theFace.Orientation() == TopAbs_REVERSED)
  {
    aNormal.Reverse();
  }
  theOutNormal = gp_Dir(aNormal);
  return true;
}

double ShapeBoundingExtent(const TopoDS_Shape& theShape)
{
  Bnd_Box aBox;
  BRepBndLib::Add(theShape, aBox);
  if (aBox.IsVoid())
  {
    return 1.0;
  }

  double aXMin = 0.0;
  double aYMin = 0.0;
  double aZMin = 0.0;
  double aXMax = 0.0;
  double aYMax = 0.0;
  double aZMax = 0.0;
  aBox.Get(aXMin, aYMin, aZMin, aXMax, aYMax, aZMax);
  if (!IsFiniteValue(aXMin) || !IsFiniteValue(aYMin) || !IsFiniteValue(aZMin)
      || !IsFiniteValue(aXMax) || !IsFiniteValue(aYMax) || !IsFiniteValue(aZMax))
  {
    return 1.0;
  }
  return std::max(1.0, gp_Pnt(aXMin, aYMin, aZMin).Distance(gp_Pnt(aXMax, aYMax, aZMax)));
}

gp_Pnt ShapeBoundingCenter(const TopoDS_Shape& theShape)
{
  Bnd_Box aBox;
  BRepBndLib::Add(theShape, aBox);
  if (aBox.IsVoid())
  {
    return gp_Pnt(0.0, 0.0, 0.0);
  }

  double aXMin = 0.0;
  double aYMin = 0.0;
  double aZMin = 0.0;
  double aXMax = 0.0;
  double aYMax = 0.0;
  double aZMax = 0.0;
  aBox.Get(aXMin, aYMin, aZMin, aXMax, aYMax, aZMax);
  return gp_Pnt((aXMin + aXMax) * 0.5, (aYMin + aYMax) * 0.5, (aZMin + aZMax) * 0.5);
}

bool FaceNormalAtPoint(const TopoDS_Face& theFace, const gp_Pnt& thePoint, gp_Dir& theOutNormal)
{
  const occ::handle<Geom_Surface> aSurface = BRep_Tool::Surface(theFace);
  if (aSurface.IsNull())
  {
    return false;
  }

  double aUMin = 0.0;
  double aUMax = 0.0;
  double aVMin = 0.0;
  double aVMax = 0.0;
  BRepTools::UVBounds(theFace, aUMin, aUMax, aVMin, aVMax);
  GeomAPI_ProjectPointOnSurf aProjector(thePoint, aSurface, aUMin, aUMax, aVMin, aVMax);
  if (!aProjector.IsDone() || aProjector.NbPoints() < 1)
  {
    return FaceMidNormal(theFace, theOutNormal);
  }

  double aU = 0.0;
  double aV = 0.0;
  aProjector.LowerDistanceParameters(aU, aV);

  gp_Pnt aPoint;
  gp_Vec aDU;
  gp_Vec aDV;
  aSurface->D1(aU, aV, aPoint, aDU, aDV);
  gp_Vec aNormal = aDU.Crossed(aDV);
  if (aNormal.SquareMagnitude() <= Precision::SquareConfusion())
  {
    return FaceMidNormal(theFace, theOutNormal);
  }
  if (theFace.Orientation() == TopAbs_REVERSED)
  {
    aNormal.Reverse();
  }
  theOutNormal = gp_Dir(aNormal);
  return true;
}

bool IntersectFaceNearest(IntCurvesFace_ShapeIntersector& theIntersector,
                          const TopoDS_Face&              theTargetFace,
                          const gp_Pnt&                   thePoint,
                          const gp_Dir&                   theDirection,
                          const double                    theExtent,
                          gp_Pnt&                         theOutPoint,
                          gp_Dir&                         theOutNormal)
{
  theIntersector.Perform(gp_Lin(thePoint, theDirection), -theExtent, theExtent);
  if (!theIntersector.IsDone() || theIntersector.NbPnt() < 1)
  {
    return false;
  }

  int    aBestIndex = 1;
  double aBestAbs   = std::abs(theIntersector.WParameter(1));
  for (int anIdx = 2; anIdx <= theIntersector.NbPnt(); ++anIdx)
  {
    const double aParam = std::abs(theIntersector.WParameter(anIdx));
    if (aParam < aBestAbs)
    {
      aBestAbs   = aParam;
      aBestIndex = anIdx;
    }
  }

  theOutPoint = theIntersector.Pnt(aBestIndex);
  return FaceNormalAtPoint(theTargetFace, theOutPoint, theOutNormal);
}

gp_Pnt LocalSurfaceOffset(const gp_Pnt& theOrigin,
                          const gp_Dir& theXDirection,
                          const gp_Dir& theNormal,
                          const gp_Vec& theOffset)
{
  gp_Ax3 aFrame(theOrigin, theNormal, theXDirection);
  gp_Vec aWorldOffset(gp_Vec(aFrame.XDirection()) * theOffset.X());
  aWorldOffset += gp_Vec(aFrame.YDirection()) * theOffset.Y();
  aWorldOffset += gp_Vec(aFrame.Direction()) * theOffset.Z();
  return theOrigin.Translated(aWorldOffset);
}

occtl_status_t InterpolateWrappedEdge(const NCollection_LinearVector<gp_Pnt>& thePoints,
                                      const double                            theTolerance,
                                      TopoDS_Edge&                            theOutEdge)
{
  if (thePoints.Size() < 2)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "wrapped edge needs at least two points");
    return OCCTL_GEOMETRY_INVALID;
  }

  occ::handle<NCollection_HArray1<gp_Pnt>> aPoints =
    new NCollection_HArray1<gp_Pnt>(1, static_cast<int>(thePoints.Size()));
  for (int anIdx = 1; anIdx <= aPoints->Length(); ++anIdx)
  {
    aPoints->SetValue(anIdx, thePoints.Value(static_cast<size_t>(anIdx - 1)));
  }

  GeomAPI_Interpolate anInterpolator(aPoints, false, theTolerance);
  anInterpolator.Perform();
  if (!anInterpolator.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "OCCT failed to interpolate wrapped edge");
    return OCCTL_GEOMETRY_INVALID;
  }

  BRepBuilderAPI_MakeEdge anEdgeMaker(anInterpolator.Curve());
  if (!anEdgeMaker.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "OCCT failed to build wrapped edge");
    return OCCTL_GEOMETRY_INVALID;
  }

  theOutEdge = anEdgeMaker.Edge();
  return OCCTL_OK;
}

occtl_status_t WrapEdgeFromStart(const TopoDS_Edge&                       theEdge,
                                 const TopoDS_Face&                       theTargetFace,
                                 IntCurvesFace_ShapeIntersector&          theIntersector,
                                 const gp_Pnt&                            theTargetCenter,
                                 const gp_Dir&                            theSurfaceXDirection,
                                 const gp_Pnt&                            theStartPoint,
                                 const gp_Dir&                            theStartNormal,
                                 const occtl_topo_wrap_on_face_options_t& theOpts,
                                 const double                             theExtent,
                                 TopoDS_Edge&                             theOutEdge,
                                 gp_Pnt&                                  theOutEndPoint,
                                 gp_Dir&                                  theOutEndNormal)
{
  BRepAdaptor_Curve aCurve(theEdge);
  const double      aFirst        = aCurve.FirstParameter();
  const double      aLast         = aCurve.LastParameter();
  const double      aSourceLength = EdgeLength(theEdge);
  if (!IsFiniteValue(aFirst) || !IsFiniteValue(aLast) || aLast <= aFirst || aSourceLength <= 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "source edge range or length is invalid");
    return OCCTL_GEOMETRY_INVALID;
  }

  int         aSubdivisions = std::max(1, theOpts.initial_subdivisions);
  TopoDS_Edge aBestEdge;
  gp_Pnt      aBestEndPoint  = theStartPoint;
  gp_Dir      aBestEndNormal = theStartNormal;
  for (int aRefinement = 0; aRefinement <= theOpts.max_refinements; ++aRefinement)
  {
    NCollection_LinearVector<gp_Pnt> aWrappedPoints;
    aWrappedPoints.Append(theStartPoint);

    gp_Pnt aCurrentPoint   = theStartPoint;
    gp_Dir aCurrentNormal  = theStartNormal;
    gp_Pnt aPreviousPlanar = aCurve.Value(aFirst);
    for (int anIdx = 1; anIdx <= aSubdivisions; ++anIdx)
    {
      const double aParam =
        aFirst + (aLast - aFirst) * (static_cast<double>(anIdx) / aSubdivisions);
      const gp_Pnt aPlanar = aCurve.Value(aParam);
      const gp_Vec anOffset(aPreviousPlanar, aPlanar);
      const gp_Pnt aWorldPoint =
        LocalSurfaceOffset(aCurrentPoint, theSurfaceXDirection, aCurrentNormal, anOffset);
      gp_Vec aRayVector(theTargetCenter, aWorldPoint);
      if (aRayVector.SquareMagnitude() <= Precision::SquareConfusion())
      {
        aRayVector = gp_Vec(aCurrentNormal);
      }

      gp_Pnt aNextPoint;
      gp_Dir aNextNormal;
      if (!IntersectFaceNearest(theIntersector,
                                theTargetFace,
                                aWorldPoint,
                                gp_Dir(aRayVector),
                                theExtent,
                                aNextPoint,
                                aNextNormal))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "wrapped point ray did not intersect target face");
        return OCCTL_GEOMETRY_INVALID;
      }

      aWrappedPoints.Append(aNextPoint);
      aCurrentPoint   = aNextPoint;
      aCurrentNormal  = aNextNormal;
      aPreviousPlanar = aPlanar;
    }

    TopoDS_Edge aCandidate;
    if (const occtl_status_t aStatus =
          InterpolateWrappedEdge(aWrappedPoints, theOpts.tolerance, aCandidate))
    {
      return aStatus;
    }

    aBestEdge                 = aCandidate;
    aBestEndPoint             = aCurrentPoint;
    aBestEndNormal            = aCurrentNormal;
    const double aLengthError = std::abs(EdgeLength(aCandidate) - aSourceLength);
    if (aLengthError <= theOpts.tolerance)
    {
      theOutEdge      = aBestEdge;
      theOutEndPoint  = aBestEndPoint;
      theOutEndNormal = aBestEndNormal;
      return OCCTL_OK;
    }
    aSubdivisions *= 2;
  }

  theOutEdge      = aBestEdge;
  theOutEndPoint  = aBestEndPoint;
  theOutEndNormal = aBestEndNormal;
  return OCCTL_OK;
}

occtl_status_t FixWrappedWire(const NCollection_LinearVector<TopoDS_Edge>& theEdges,
                              const double                                 thePrecision,
                              TopoDS_Wire&                                 theOutWire)
{
  BRepBuilderAPI_MakeWire aWireMaker;
  for (size_t anEdgeIndex = 0; anEdgeIndex < theEdges.Size(); ++anEdgeIndex)
  {
    const TopoDS_Edge& anEdge = theEdges.Value(anEdgeIndex);
    aWireMaker.Add(anEdge);
  }
  aWireMaker.Build();
  if (!aWireMaker.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "OCCT failed to build wrapped wire");
    return OCCTL_GEOMETRY_INVALID;
  }

  ShapeFix_Wire aFixer;
  aFixer.SetPrecision(thePrecision);
  aFixer.Load(aWireMaker.Wire());
  aFixer.FixReorder();
  aFixer.FixConnected();
  theOutWire = aFixer.Wire();
  if (theOutWire.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "OCCT failed to fix wrapped wire");
    return OCCTL_GEOMETRY_INVALID;
  }
  return OCCTL_OK;
}

occtl_status_t ResolveTypedShape(const occtl_graph_t* const   theGraph,
                                 const occtl_node_id_t        theNodeId,
                                 const BRepGraph_NodeId::Kind theKind,
                                 const TopAbs_ShapeEnum       theShapeType,
                                 const char* const            theLabel,
                                 TopoDS_Shape&                theOutShape)
{
  BRepGraph_NodeId aNodeId;
  if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph, theNodeId, theKind, aNodeId))
  {
    return aStatus;
  }

  theOutShape = theGraph->graph.Shapes().Shape(aNodeId);
  if (theOutShape.IsNull() || theOutShape.ShapeType() != theShapeType)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " could not be reconstructed as TopoDS shape"));
    return OCCTL_NOT_FOUND;
  }
  return OCCTL_OK;
}

void BindShapeAndSubshapes(
  const occtl_graph_t* const                                                    theGraph,
  const TopoDS_Shape&                                                           theRoot,
  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>& theOutMap)
{
  auto bind = [&](const TopoDS_Shape& theSub) {
    if (theSub.IsNull() || theOutMap.IsBound(theSub))
    {
      return;
    }
    const BRepGraph_NodeId aNodeId = theGraph->graph.Shapes().FindNode(theSub);
    if (aNodeId.IsValid())
    {
      theOutMap.Bind(theSub, aNodeId);
    }
  };

  bind(theRoot);
  static const TopAbs_ShapeEnum aKinds[] = {TopAbs_COMPOUND,
                                            TopAbs_COMPSOLID,
                                            TopAbs_SOLID,
                                            TopAbs_SHELL,
                                            TopAbs_FACE,
                                            TopAbs_WIRE,
                                            TopAbs_EDGE,
                                            TopAbs_VERTEX};
  for (const TopAbs_ShapeEnum aKind : aKinds)
  {
    for (TopExp_Explorer anExp(theRoot, aKind); anExp.More(); anExp.Next())
    {
      bind(anExp.Current());
    }
  }
}

void FillHistory(
  const occtl_graph_t* const theInputGraph,
  occtl_graph_t* const       theOutputGraph,
  const NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>& theInputs,
  const NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>& theOutputs,
  const occ::handle<BRepTools_History>&                                               theSource,
  const TCollection_AsciiString&                                                      theOpLabel)
{
  theOutputGraph->graph.LayerRegistry().Ensure<BRepGraph_LayerHistory>()->Absorb(
    theInputGraph->graph,
    theOutputGraph->graph,
    theInputs,
    theOutputs,
    theSource,
    theOpLabel);
}

occtl_status_t AddFeatureSelectionFaces(const occtl_graph_t* const theGraph,
                                        const occtl_node_id_t      theSelection,
                                        BRepAlgoAPI_Defeaturing&   theDefeaturing,
                                        size_t&                    theFaceCount)
{
  BRepGraph_NodeId aNodeId;
  TopoDS_Shape     aShape;
  if (const occtl_status_t aStatus =
        ResolveShape(theGraph, theSelection, "selected feature", aNodeId, aShape))
  {
    return aStatus;
  }

  switch (aShape.ShapeType())
  {
    case TopAbs_FACE:
      theDefeaturing.AddFaceToRemove(aShape);
      ++theFaceCount;
      return OCCTL_OK;
    case TopAbs_SHELL:
    case TopAbs_SOLID:
    case TopAbs_COMPSOLID:
    case TopAbs_COMPOUND:
      for (TopExp_Explorer anExp(aShape, TopAbs_FACE); anExp.More(); anExp.Next())
      {
        theDefeaturing.AddFaceToRemove(anExp.Current());
        ++theFaceCount;
      }
      return OCCTL_OK;
    default:
      break;
  }

  OcctL::Core::ErrorState::Current().Set(
    OCCTL_WRONG_KIND,
    "selected feature must be a Face, Shell, Solid, CompSolid or Compound");
  return OCCTL_WRONG_KIND;
}

occtl_status_t SetFeatureSelectionOffset(const occtl_graph_t* const theGraph,
                                         const occtl_node_id_t      theSelection,
                                         const double               theOffset,
                                         BRepOffset_MakeOffset&     theOffsetMaker,
                                         size_t&                    theFaceCount)
{
  BRepGraph_NodeId aNodeId;
  TopoDS_Shape     aShape;
  if (const occtl_status_t aStatus =
        ResolveShape(theGraph, theSelection, "selected feature", aNodeId, aShape))
  {
    return aStatus;
  }

  switch (aShape.ShapeType())
  {
    case TopAbs_FACE:
      theOffsetMaker.SetOffsetOnFace(TopoDS::Face(aShape), theOffset);
      ++theFaceCount;
      return OCCTL_OK;
    case TopAbs_SHELL:
    case TopAbs_SOLID:
    case TopAbs_COMPSOLID:
    case TopAbs_COMPOUND:
      for (TopExp_Explorer anExp(aShape, TopAbs_FACE); anExp.More(); anExp.Next())
      {
        theOffsetMaker.SetOffsetOnFace(TopoDS::Face(anExp.Current()), theOffset);
        ++theFaceCount;
      }
      return OCCTL_OK;
    default:
      break;
  }

  OcctL::Core::ErrorState::Current().Set(
    OCCTL_WRONG_KIND,
    "selected feature must be a Face, Shell, Solid, CompSolid or Compound");
  return OCCTL_WRONG_KIND;
}

static occtl_status_t blendEdgesImpl(const occtl_graph_t*                   theGraph,
                                     const occtl_topo_edge_blend_options_t* theOpts,
                                     occtl_graph_t**                        theOutGraph,
                                     occtl_node_id_t*                       theOutRoot)
{
  if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "graph, options, out_graph or out_root is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->struct_version != OCCTL_TOPO_EDGE_BLEND_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_topo_edge_blend_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->edge_count == 0 || theOpts->edges == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "selected edge list is empty or NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->chamfer_mode == 0 && theOpts->radius <= 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "fillet radius must be strictly positive");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->chamfer_mode != 0
      && (theOpts->chamfer_dist1 <= 0.0 || theOpts->chamfer_dist2 <= 0.0))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "chamfer distances must be strictly positive");
    return OCCTL_INVALID_ARGUMENT;
  }

  *theOutGraph = nullptr;
  *theOutRoot  = OCCTL_NODE_ID_INVALID;

  BRepGraph_NodeId aRootId;
  TopoDS_Shape     aRootShape;
  if (const occtl_status_t aStatus =
        ResolveShape(theGraph, theOpts->root, "root", aRootId, aRootShape))
  {
    return aStatus;
  }

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anInputNodes;
  NCollection_List<TopoDS_Shape>                                               anInputs;
  BindShapeAndSubshapes(theGraph, aRootShape, anInputNodes);
  anInputs.Append(aRootShape);

  if (theOpts->chamfer_mode != 0)
  {
    NCollection_IndexedDataMap<TopoDS_Shape,
                               NCollection_List<TopoDS_Shape>,
                               TopTools_ShapeMapHasher>
      anEdgeFaceMap;
    TopExp::MapShapesAndAncestors(aRootShape, TopAbs_EDGE, TopAbs_FACE, anEdgeFaceMap);

    BRepFilletAPI_MakeChamfer aChamfer(aRootShape);
    for (size_t anIdx = 0; anIdx < theOpts->edge_count; ++anIdx)
    {
      BRepGraph_NodeId anEdgeId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theOpts->edges[anIdx],
                                                                BRepGraph_NodeId::Kind::Edge,
                                                                anEdgeId))
      {
        return aStatus;
      }

      const TopoDS_Shape anEdgeShape = theGraph->graph.Shapes().Shape(anEdgeId);
      if (anEdgeShape.IsNull() || anEdgeShape.ShapeType() != TopAbs_EDGE)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_NOT_FOUND,
          "selected edge could not be reconstructed as TopoDS_Edge");
        return OCCTL_NOT_FOUND;
      }

      const TopoDS_Edge anEdge = TopoDS::Edge(anEdgeShape);
      if (theOpts->chamfer_dist1 == theOpts->chamfer_dist2 || !anEdgeFaceMap.Contains(anEdge))
      {
        aChamfer.Add(theOpts->chamfer_dist1, anEdge);
      }
      else
      {
        const TopoDS_Face aFace = TopoDS::Face(anEdgeFaceMap.FindFromKey(anEdge).First());
        aChamfer.Add(theOpts->chamfer_dist1, theOpts->chamfer_dist2, anEdge, aFace);
      }
    }
    aChamfer.Build();
    if (!aChamfer.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "selected-edge chamfer failed to produce a valid result");
      return OCCTL_GEOMETRY_INVALID;
    }

    NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anOutputNodes;
    if (const occtl_status_t aStatus = AddCopiedResult(aChamfer.Shape(),
                                                       "selected-edge chamfer",
                                                       theOutGraph,
                                                       theOutRoot,
                                                       &anOutputNodes))
    {
      return aStatus;
    }

    occ::handle<BRepTools_History> aSourceHistory = new BRepTools_History(anInputs, aChamfer);
    FillHistory(theGraph,
                *theOutGraph,
                anInputNodes,
                anOutputNodes,
                aSourceHistory,
                TCollection_AsciiString("selected-edge chamfer"));
    return OCCTL_OK;
  }

  BRepFilletAPI_MakeFillet aFillet(aRootShape);
  for (size_t anIdx = 0; anIdx < theOpts->edge_count; ++anIdx)
  {
    BRepGraph_NodeId anEdgeId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theOpts->edges[anIdx],
                                                              BRepGraph_NodeId::Kind::Edge,
                                                              anEdgeId))
    {
      return aStatus;
    }

    const TopoDS_Shape anEdgeShape = theGraph->graph.Shapes().Shape(anEdgeId);
    if (anEdgeShape.IsNull() || anEdgeShape.ShapeType() != TopAbs_EDGE)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "selected edge could not be reconstructed as TopoDS_Edge");
      return OCCTL_NOT_FOUND;
    }
    aFillet.Add(theOpts->radius, TopoDS::Edge(anEdgeShape));
  }
  aFillet.Build();
  if (!aFillet.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "selected-edge fillet failed to produce a valid result");
    return OCCTL_GEOMETRY_INVALID;
  }

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anOutputNodes;
  if (const occtl_status_t aStatus = AddCopiedResult(aFillet.Shape(),
                                                     "selected-edge fillet",
                                                     theOutGraph,
                                                     theOutRoot,
                                                     &anOutputNodes))
  {
    return aStatus;
  }

  occ::handle<BRepTools_History> aSourceHistory = new BRepTools_History(anInputs, aFillet);
  FillHistory(theGraph,
              *theOutGraph,
              anInputNodes,
              anOutputNodes,
              aSourceHistory,
              TCollection_AsciiString("topology algorithm"));
  return OCCTL_OK;
}

static occtl_status_t draftFacesImpl(const occtl_graph_t*                    theGraph,
                                     const occtl_topo_draft_faces_options_t* theOpts,
                                     occtl_graph_t**                         theOutGraph,
                                     occtl_node_id_t*                        theOutRoot)
{
  if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "graph, options, out_graph or out_root is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->struct_version != OCCTL_TOPO_DRAFT_FACES_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_topo_draft_faces_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->faces == nullptr || theOpts->face_count == 0 || theOpts->angle == 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "selected feature list is empty or NULL, or angle is zero");
    return OCCTL_INVALID_ARGUMENT;
  }

  *theOutGraph = nullptr;
  *theOutRoot  = OCCTL_NODE_ID_INVALID;

  BRepGraph_NodeId aRootId;
  TopoDS_Shape     aRootShape;
  if (const occtl_status_t aStatus =
        ResolveShape(theGraph, theOpts->root, "root", aRootId, aRootShape))
  {
    return aStatus;
  }

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anInputNodes;
  NCollection_List<TopoDS_Shape>                                               anInputs;
  BindShapeAndSubshapes(theGraph, aRootShape, anInputNodes);
  anInputs.Append(aRootShape);

  BRepOffsetAPI_DraftAngle aDraft(aRootShape);
  const gp_Dir             aPullDirection = OcctL::Geom::ToGp(theOpts->pull_direction);
  const gp_Pln             aNeutralPlane(OcctL::Geom::ToGp(theOpts->neutral_point),
                                         OcctL::Geom::ToGp(theOpts->neutral_normal));
  for (size_t anIdx = 0; anIdx < theOpts->face_count; ++anIdx)
  {
    TopoDS_Shape aFaceShape;
    if (const occtl_status_t aStatus = ResolveTypedShape(theGraph,
                                                         theOpts->faces[anIdx],
                                                         BRepGraph_NodeId::Kind::Face,
                                                         TopAbs_FACE,
                                                         "selected face",
                                                         aFaceShape))
    {
      return aStatus;
    }

    aDraft.Add(TopoDS::Face(aFaceShape),
               aPullDirection,
               theOpts->angle,
               aNeutralPlane,
               theOpts->keep_inside != 0);
    if (!aDraft.AddDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "draft failed while adding a selected face");
      return OCCTL_GEOMETRY_INVALID;
    }
  }

  aDraft.Build();
  if (!aDraft.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "draft failed to produce a valid result");
    return OCCTL_GEOMETRY_INVALID;
  }

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anOutputNodes;
  if (const occtl_status_t aStatus =
        AddCopiedResult(aDraft.Shape(), "draft", theOutGraph, theOutRoot, &anOutputNodes))
  {
    return aStatus;
  }

  occ::handle<BRepTools_History> aSourceHistory = new BRepTools_History(anInputs, aDraft);
  FillHistory(theGraph,
              *theOutGraph,
              anInputNodes,
              anOutputNodes,
              aSourceHistory,
              TCollection_AsciiString("topology algorithm"));
  return OCCTL_OK;
}

static occtl_status_t removeFeaturesImpl(const occtl_graph_t*                  theGraph,
                                         const occtl_topo_defeature_options_t* theOpts,
                                         occtl_graph_t**                       theOutGraph,
                                         occtl_node_id_t*                      theOutRoot)
{
  if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "graph, options, out_graph or out_root is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->struct_version != OCCTL_TOPO_DEFEATURE_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_topo_defeature_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->selections == nullptr || theOpts->selection_count == 0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "selected feature list is empty or NULL");
    return OCCTL_INVALID_ARGUMENT;
  }

  *theOutGraph = nullptr;
  *theOutRoot  = OCCTL_NODE_ID_INVALID;

  BRepGraph_NodeId aRootId;
  TopoDS_Shape     aRootShape;
  if (const occtl_status_t aStatus =
        ResolveShape(theGraph, theOpts->root, "root", aRootId, aRootShape))
  {
    return aStatus;
  }

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anInputNodes;
  BindShapeAndSubshapes(theGraph, aRootShape, anInputNodes);

  BRepAlgoAPI_Defeaturing aDefeaturing;
  aDefeaturing.SetShape(aRootShape);
  aDefeaturing.SetRunParallel(theOpts->parallel != 0);
  aDefeaturing.SetToFillHistory(true);
  size_t aFaceCount = 0;
  for (size_t anIdx = 0; anIdx < theOpts->selection_count; ++anIdx)
  {
    if (const occtl_status_t aStatus =
          AddFeatureSelectionFaces(theGraph, theOpts->selections[anIdx], aDefeaturing, aFaceCount))
    {
      return aStatus;
    }
  }
  if (aFaceCount == 0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                           "selected features did not contain removable faces");
    return OCCTL_WRONG_KIND;
  }

  aDefeaturing.Build();
  if (!aDefeaturing.IsDone() || aDefeaturing.Shape().IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "defeaturing failed to produce a valid result");
    return OCCTL_GEOMETRY_INVALID;
  }

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anOutputNodes;
  if (const occtl_status_t aStatus = AddCopiedResult(aDefeaturing.Shape(),
                                                     "defeaturing",
                                                     theOutGraph,
                                                     theOutRoot,
                                                     &anOutputNodes))
  {
    return aStatus;
  }
  FillHistory(theGraph,
              *theOutGraph,
              anInputNodes,
              anOutputNodes,
              aDefeaturing.History(),
              TCollection_AsciiString("defeaturing"));
  return OCCTL_OK;
}

static occtl_status_t offsetFeaturesImpl(const occtl_graph_t*                        theGraph,
                                         const occtl_topo_offset_features_options_t* theOpts,
                                         occtl_graph_t**                             theOutGraph,
                                         occtl_node_id_t*                            theOutRoot)
{
  if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "graph, options, out_graph or out_root is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->struct_version != OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_topo_offset_features_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->selections == nullptr || theOpts->selection_count == 0 || theOpts->tolerance <= 0.0
      || (theOpts->base_offset == 0.0 && theOpts->selection_offset == 0.0))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "selected feature list or offset options are invalid");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->join != OCCTL_OFFSET_JOIN_ARC && theOpts->join != OCCTL_OFFSET_JOIN_TANGENT
      && theOpts->join != OCCTL_OFFSET_JOIN_INTERSECTION)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "unsupported offset join type");
    return OCCTL_INVALID_ARGUMENT;
  }

  *theOutGraph = nullptr;
  *theOutRoot  = OCCTL_NODE_ID_INVALID;

  BRepGraph_NodeId aRootId;
  TopoDS_Shape     aRootShape;
  if (const occtl_status_t aStatus =
        ResolveShape(theGraph, theOpts->root, "root", aRootId, aRootShape))
  {
    return aStatus;
  }

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anInputNodes;
  BindShapeAndSubshapes(theGraph, aRootShape, anInputNodes);

  BRepOffset_MakeOffset anOffsetMaker;
  anOffsetMaker.Initialize(aRootShape,
                           theOpts->base_offset,
                           theOpts->tolerance,
                           BRepOffset_Skin,
                           theOpts->intersection != 0,
                           theOpts->self_intersection != 0,
                           ToGeomAbsJoin(theOpts->join),
                           false,
                           theOpts->remove_internal_edges != 0);

  size_t aFaceCount = 0;
  for (size_t anIdx = 0; anIdx < theOpts->selection_count; ++anIdx)
  {
    if (const occtl_status_t aStatus = SetFeatureSelectionOffset(theGraph,
                                                                 theOpts->selections[anIdx],
                                                                 theOpts->selection_offset,
                                                                 anOffsetMaker,
                                                                 aFaceCount))
    {
      return aStatus;
    }
  }
  if (aFaceCount == 0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                           "selected features did not contain offsettable faces");
    return OCCTL_WRONG_KIND;
  }

  anOffsetMaker.MakeOffsetShape();
  if (!anOffsetMaker.IsDone() || anOffsetMaker.Shape().IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_GEOMETRY_INVALID,
      "selected-feature offset failed to produce a valid result");
    return OCCTL_GEOMETRY_INVALID;
  }

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anOutputNodes;
  if (const occtl_status_t aStatus = AddCopiedResult(anOffsetMaker.Shape(),
                                                     "selected-feature offset",
                                                     theOutGraph,
                                                     theOutRoot,
                                                     &anOutputNodes))
  {
    return aStatus;
  }

  NCollection_List<TopoDS_Shape> anInputs;
  anInputs.Append(aRootShape);
  occ::handle<BRepTools_History> aSourceHistory = new BRepTools_History(anInputs, anOffsetMaker);
  FillHistory(theGraph,
              *theOutGraph,
              anInputNodes,
              anOutputNodes,
              aSourceHistory,
              TCollection_AsciiString("topology algorithm"));
  return OCCTL_OK;
}

static occtl_status_t makeFillingImpl(const occtl_graph_t*                theGraph,
                                      const occtl_topo_filling_options_t* theOpts,
                                      occtl_graph_t**                     theOutGraph,
                                      occtl_node_id_t*                    theOutRoot)
{
  if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "graph, options, out_graph or out_root is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->struct_version != OCCTL_TOPO_FILLING_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_topo_filling_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->edges == nullptr || theOpts->edge_count < 2 || theOpts->degree < 1
      || theOpts->point_count_on_curve < 2 || theOpts->iteration_count < 1
      || theOpts->max_degree < 1 || theOpts->max_segments < 1 || theOpts->tolerance_2d <= 0.0
      || theOpts->tolerance_3d <= 0.0 || theOpts->angular_tolerance <= 0.0
      || theOpts->curvature_tolerance <= 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "filling edge list or solver parameters are invalid");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->continuity != OCCTL_TOPO_FILLING_C0 && theOpts->continuity != OCCTL_TOPO_FILLING_G1
      && theOpts->continuity != OCCTL_TOPO_FILLING_G2)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "unsupported filling continuity");
    return OCCTL_INVALID_ARGUMENT;
  }

  *theOutGraph = nullptr;
  *theOutRoot  = OCCTL_NODE_ID_INVALID;

  BRepOffsetAPI_MakeFilling aFilling(theOpts->degree,
                                     theOpts->point_count_on_curve,
                                     theOpts->iteration_count,
                                     theOpts->anisotropic != 0,
                                     theOpts->tolerance_2d,
                                     theOpts->tolerance_3d,
                                     theOpts->angular_tolerance,
                                     theOpts->curvature_tolerance,
                                     theOpts->max_degree,
                                     theOpts->max_segments);
  aFilling.SetConstrParam(theOpts->tolerance_2d,
                          theOpts->tolerance_3d,
                          theOpts->angular_tolerance,
                          theOpts->curvature_tolerance);
  aFilling.SetResolParam(theOpts->degree,
                         theOpts->point_count_on_curve,
                         theOpts->iteration_count,
                         theOpts->anisotropic != 0);
  aFilling.SetApproxParam(theOpts->max_degree, theOpts->max_segments);

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anInputNodes;
  NCollection_List<TopoDS_Shape>                                               anInputs;
  const GeomAbs_Shape aContinuity = ToGeomAbsContinuity(theOpts->continuity);
  for (size_t anIdx = 0; anIdx < theOpts->edge_count; ++anIdx)
  {
    BRepGraph_NodeId anEdgeId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theOpts->edges[anIdx],
                                                              BRepGraph_NodeId::Kind::Edge,
                                                              anEdgeId))
    {
      return aStatus;
    }

    const TopoDS_Shape anEdgeShape = theGraph->graph.Shapes().Shape(anEdgeId);
    if (anEdgeShape.IsNull() || anEdgeShape.ShapeType() != TopAbs_EDGE)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "filling edge could not be reconstructed as TopoDS_Edge");
      return OCCTL_NOT_FOUND;
    }
    if (!anInputNodes.IsBound(anEdgeShape))
    {
      anInputNodes.Bind(anEdgeShape, anEdgeId);
      anInputs.Append(anEdgeShape);
    }
    aFilling.Add(TopoDS::Edge(anEdgeShape), aContinuity, true);
  }

  aFilling.Build();
  if (!aFilling.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "filling failed to produce a valid result");
    return OCCTL_GEOMETRY_INVALID;
  }

  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anOutputNodes;
  if (const occtl_status_t aStatus =
        AddCopiedResult(aFilling.Shape(), "filling", theOutGraph, theOutRoot, &anOutputNodes))
  {
    return aStatus;
  }

  occ::handle<BRepTools_History> aSourceHistory = new BRepTools_History(anInputs, aFilling);
  FillHistory(theGraph,
              *theOutGraph,
              anInputNodes,
              anOutputNodes,
              aSourceHistory,
              TCollection_AsciiString("topology algorithm"));
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_sew_options_init(occtl_topo_sew_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_sew_options_t aInit = OCCTL_TOPO_SEW_OPTIONS_INIT;
  *theOpts                             = aInit;
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_sew_result_init(occtl_topo_sew_result_t* const theResult)
{
  if (theResult == nullptr)
  {
    return;
  }
  const occtl_topo_sew_result_t aInit = OCCTL_TOPO_SEW_RESULT_INIT;
  *theResult                          = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_sew(occtl_graph_t* const                  theGraph,
                                                   const occtl_topo_sew_options_t* const theOpts,
                                                   occtl_topo_sew_result_t* const theOutResult)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts != nullptr && theOpts->struct_version != OCCTL_TOPO_SEW_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_sew_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOutResult != nullptr && theOutResult->struct_version != OCCTL_TOPO_SEW_RESULT_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_topo_sew_result_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    const BRepGraphAlgo_Sewing::Options aOpts = ToOcctSewOptions(theOpts);
    const BRepGraphAlgo_Sewing::Result aRes = BRepGraphAlgo_Sewing::Perform(theGraph->graph, aOpts);
    FillSewResult(theOutResult, aRes);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_same_parameter_options_init(occtl_topo_same_parameter_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_same_parameter_options_t aInit = OCCTL_TOPO_SAME_PARAMETER_OPTIONS_INIT;
  *theOpts                                        = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_recompute_same_parameter(occtl_graph_t* const                             theGraph,
                                      const occtl_topo_same_parameter_options_t* const theOpts,
                                      uint32_t* const                                  theOutC0,
                                      uint32_t* const                                  theOutApprox)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts != nullptr
        && theOpts->struct_version != OCCTL_TOPO_SAME_PARAMETER_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_same_parameter_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }

    BRepGraphAlgo_SameParameter::Options aOcctOpts;
    if (theOpts != nullptr)
    {
      aOcctOpts.Tolerance   = theOpts->tolerance;
      aOcctOpts.HistoryMode = theOpts->history_mode != 0;
    }

    NCollection_IndexedMap<BRepGraph_EdgeId> aEdges;
    for (BRepGraph_EdgeIterator anIt(theGraph->graph); anIt.More(); anIt.Next())
    {
      aEdges.Add(anIt.CurrentId());
    }

    const BRepGraphAlgo_SameParameter::Result aRes =
      BRepGraphAlgo_SameParameter::Perform(theGraph->graph, aEdges, aOcctOpts);
    if (theOutC0 != nullptr)
    {
      *theOutC0 = static_cast<uint32_t>(aRes.NbC0Fallbacks);
    }
    if (theOutApprox != nullptr)
    {
      *theOutApprox = static_cast<uint32_t>(aRes.NbApproxFallbacks);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_check(const occtl_graph_t* const      theGraph,
                                                     occtl_topo_check_issue_t* const theOutIssues,
                                                     const size_t                    theCap,
                                                     size_t* const                   theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_count is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraphCheck_Analyzer anAnalyzer(theGraph->graph);
    anAnalyzer.Perform();
    const BRepGraphCheck_CheckView aView = anAnalyzer.View();

    // Two-pass: first gather all issues across all invalid nodes into a flat
    // buffer so we can size the caller's buffer.  Each node's Issues() call
    // combines def-level + context issues; deduplication is OCCT's
    // responsibility.
    NCollection_LinearVector<BRepGraphCheck_Issue>   anAll;
    const NCollection_DynamicArray<BRepGraph_NodeId> anInvalid = aView.InvalidNodes();
    for (size_t i = 0; i < anInvalid.Size(); ++i)
    {
      const NCollection_DynamicArray<BRepGraphCheck_Issue> aOne = aView.Issues(anInvalid[i]);
      for (size_t j = 0; j < aOne.Size(); ++j)
      {
        anAll.Append(aOne[j]);
      }
    }

    const size_t aTotal = anAll.Size();
    *theOutCount        = aTotal;

    if (theOutIssues == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aTotal)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "occtl_topo_check: cap < total issue count");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (size_t i = 0; i < aTotal; ++i)
    {
      const BRepGraphCheck_Issue& anIssue = anAll[i];
      occtl_topo_check_issue_t&   anOut   = theOutIssues[i];
      anOut.node_id                       = OcctL::Topo::PackNodeId(anIssue.NodeId);
      anOut.context_node_id               = OcctL::Topo::PackNodeId(anIssue.ContextNodeId);
      anOut.status_bit                    = anIssue.StatusBit;
      anOut.severity                      = ToAbiSeverity(anIssue.IssueSeverity);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_fillet_options_init(occtl_topo_fillet_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_fillet_options_t anInit = OCCTL_TOPO_FILLET_OPTIONS_INIT;
  *theOpts                                 = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_fillet(occtl_graph_t* const                     theGraph,
                    const occtl_topo_fillet_options_t* const theOpts,
                    occtl_graph_t** const                    theOutGraph,
                    occtl_node_id_t* const                   theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, opts, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_FILLET_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_fillet_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    TopoDS_Compound aCompound;
    BRep_Builder    aBuilder;
    aBuilder.MakeCompound(aCompound);

    int32_t anAdded = 0;
    for (BRepGraph_SolidIterator anIt(theGraph->graph); anIt.More(); anIt.Next())
    {
      const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(anIt.CurrentId());
      if (!aShape.IsNull())
      {
        aBuilder.Add(aCompound, aShape);
        ++anAdded;
      }
    }
    for (BRepGraph_ShellIterator anIt(theGraph->graph); anIt.More(); anIt.Next())
    {
      const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(anIt.CurrentId());
      if (!aShape.IsNull())
      {
        aBuilder.Add(aCompound, aShape);
        ++anAdded;
      }
    }

    if (anAdded == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "no solids or shells found in graph");
      return OCCTL_GEOMETRY_INVALID;
    }

    TopoDS_Shape aResult;
    if (theOpts->chamfer_mode != 0)
    {
      NCollection_IndexedDataMap<TopoDS_Shape,
                                 NCollection_List<TopoDS_Shape>,
                                 TopTools_ShapeMapHasher>
        anEdgeFaceMap;
      TopExp::MapShapesAndAncestors(aCompound, TopAbs_EDGE, TopAbs_FACE, anEdgeFaceMap);

      BRepFilletAPI_MakeChamfer aChamfer(aCompound);
      for (TopExp_Explorer anExp(aCompound, TopAbs_EDGE); anExp.More(); anExp.Next())
      {
        const TopoDS_Edge& anEdge = TopoDS::Edge(anExp.Current());
        if (theOpts->chamfer_dist1 == theOpts->chamfer_dist2 || !anEdgeFaceMap.Contains(anEdge))
        {
          aChamfer.Add(theOpts->chamfer_dist1, anEdge);
        }
        else
        {
          const TopoDS_Face aFace = TopoDS::Face(anEdgeFaceMap.FindFromKey(anEdge).First());
          aChamfer.Add(theOpts->chamfer_dist1, theOpts->chamfer_dist2, anEdge, aFace);
        }
      }
      aChamfer.Build();
      if (!aChamfer.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_GEOMETRY_INVALID,
          "chamfer algorithm failed to produce a valid result");
        return OCCTL_GEOMETRY_INVALID;
      }
      aResult = aChamfer.Shape();
    }
    else
    {
      BRepFilletAPI_MakeFillet aFillet(aCompound);
      for (TopExp_Explorer anExp(aCompound, TopAbs_EDGE); anExp.More(); anExp.Next())
      {
        aFillet.Add(theOpts->radius, TopoDS::Edge(anExp.Current()));
      }
      aFillet.Build();
      if (!aFillet.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "fillet algorithm failed to produce a valid result");
        return OCCTL_GEOMETRY_INVALID;
      }
      aResult = aFillet.Shape();
    }

    if (aResult.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "fillet/chamfer result shape is null");
      return OCCTL_GEOMETRY_INVALID;
    }

    occtl_graph*                   aNewGraph = new occtl_graph();
    BRepGraph::ShapesView::Options anBuildOpts;
    anBuildOpts.CreateAutoProduct = false;
    anBuildOpts.TrackAddedNodes   = true;
    const BRepGraph::ShapesView::Result aBuildRes =
      aNewGraph->graph.Shapes().Add(aResult, anBuildOpts);
    if (!aBuildRes.Ok)
    {
      delete aNewGraph;
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "fillet/chamfer result topology could not be merged into graph");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutGraph = aNewGraph;
    *theOutRoot  = OcctL::Topo::PackNodeId(aBuildRes.TopologyRoot);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_edge_blend_options_init(occtl_topo_edge_blend_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_edge_blend_options_t anInit = OCCTL_TOPO_EDGE_BLEND_OPTIONS_INIT;
  *theOpts                                     = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_blend_edges(const occtl_graph_t* const                   theGraph,
                         const occtl_topo_edge_blend_options_t* const theOpts,
                         occtl_graph_t** const                        theOutGraph,
                         occtl_node_id_t* const                       theOutRoot)
{
  return OcctL::Core::Guard(
    [&]() -> occtl_status_t { return blendEdgesImpl(theGraph, theOpts, theOutGraph, theOutRoot); });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_max_fillet_radius_options_init(occtl_topo_max_fillet_radius_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_max_fillet_radius_options_t anInit = OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_INIT;
  *theOpts                                            = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_max_fillet_radius(const occtl_graph_t* const                          theGraph,
                               const occtl_topo_max_fillet_radius_options_t* const theOpts,
                               double* const                                       theOutRadius)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutRadius == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options or out_radius is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_max_fillet_radius_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->edge_count == 0 || theOpts->edges == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "selected edge list is empty or NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!(theOpts->min_radius > 0.0) || !IsFiniteValue(theOpts->min_radius))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "min_radius must be positive and finite");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->max_radius > 0.0 && !IsFiniteValue(theOpts->max_radius))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "max_radius must be finite when positive");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!(theOpts->tolerance > 0.0) || !IsFiniteValue(theOpts->tolerance))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tolerance must be positive and finite");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->max_iterations <= 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "max_iterations must be positive");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutRadius = 0.0;

    BRepGraph_NodeId aRootId;
    TopoDS_Shape     aRootShape;
    if (const occtl_status_t aStatus =
          ResolveShape(theGraph, theOpts->root, "root", aRootId, aRootShape))
    {
      return aStatus;
    }

    NCollection_LinearVector<TopoDS_Edge> anEdges;
    if (const occtl_status_t aStatus = ResolveSelectedRootEdges(theGraph,
                                                                aRootShape,
                                                                theOpts->edges,
                                                                theOpts->edge_count,
                                                                anEdges))
    {
      return aStatus;
    }

    double anUpper = theOpts->max_radius;
    if (anUpper <= 0.0)
    {
      anUpper = 0.0;
      for (size_t anEdgeIndex = 0; anEdgeIndex < anEdges.Size(); ++anEdgeIndex)
      {
        const TopoDS_Edge& anEdge  = anEdges.Value(anEdgeIndex);
        const double       aLength = EdgeLength(anEdge);
        if (aLength > 0.0 && IsFiniteValue(aLength))
        {
          const double aCandidate = aLength * 0.5;
          anUpper                 = anUpper > 0.0 ? std::min(anUpper, aCandidate) : aCandidate;
        }
      }

      if (anUpper <= 0.0)
      {
        double anExtent = 0.0;
        if (const occtl_status_t aStatus =
              ComputeSplitPlaneEnvelope(aRootShape, gp_Pnt(0.0, 0.0, 0.0), anExtent))
        {
          return aStatus;
        }
        anUpper = anExtent * 0.25;
      }
    }

    if (!(anUpper > theOpts->min_radius) || !IsFiniteValue(anUpper))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "max_radius must be greater than min_radius");
      return OCCTL_INVALID_ARGUMENT;
    }

    double aLow  = 0.0;
    double aHigh = anUpper;
    if (TrialFilletRadius(aRootShape, anEdges, aHigh))
    {
      aLow = aHigh;
    }
    else if (TrialFilletRadius(aRootShape, anEdges, theOpts->min_radius))
    {
      aLow = theOpts->min_radius;
    }
    else
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT fillet builder failed at the minimum radius");
      return OCCTL_GEOMETRY_INVALID;
    }

    for (int anIter = 0; anIter < theOpts->max_iterations && (aHigh - aLow) > theOpts->tolerance;
         ++anIter)
    {
      const double aMid = 0.5 * (aLow + aHigh);
      if (TrialFilletRadius(aRootShape, anEdges, aMid))
      {
        aLow = aMid;
      }
      else
      {
        aHigh = aMid;
      }
    }

    *theOutRadius = aLow;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_transformed(const occtl_graph_t* const theGraph,
                                                           const occtl_node_id_t      theRoot,
                                                           const occtl_transform_t    theTransform,
                                                           occtl_graph_t** const      theOutGraph,
                                                           occtl_node_id_t* const     theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId aNodeId;
    TopoDS_Shape     aShape;
    if (const occtl_status_t aStatus = ResolveShape(theGraph, theRoot, "root", aNodeId, aShape))
    {
      return aStatus;
    }

    const gp_Trsf            aTrsf = OcctL::Geom::ToGpTrsf(theTransform);
    BRepBuilderAPI_Transform aTransform(aShape, aTrsf, true);
    return AddCopiedResult(aTransform.Shape(), "transform", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_project_on_face_options_init(occtl_topo_project_on_face_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_project_on_face_options_t anInit = OCCTL_TOPO_PROJECT_ON_FACE_OPTIONS_INIT;
  *theOpts                                          = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_project_on_face(const occtl_graph_t* const                        theGraph,
                             const occtl_topo_project_on_face_options_t* const theOpts,
                             occtl_graph_t** const                             theOutGraph,
                             occtl_node_id_t* const                            theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_PROJECT_ON_FACE_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_project_on_face_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->tolerance_3d <= 0.0 || theOpts->tolerance_2d <= 0.0 || theOpts->max_degree < 1
        || theOpts->max_segments < 1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "projection tolerances, max_degree and max_segments must be positive");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId aSourceId = OcctL::Topo::UnpackNodeId(theOpts->source);
    if (!aSourceId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "source NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }
    if (aSourceId.NodeKind != BRepGraph_NodeId::Kind::Edge
        && aSourceId.NodeKind != BRepGraph_NodeId::Kind::Wire)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "source must be an Edge or Wire");
      return OCCTL_WRONG_KIND;
    }

    const TopoDS_Shape aSourceShape = theGraph->graph.Shapes().Shape(aSourceId);
    if (aSourceShape.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "source could not be reconstructed as TopoDS shape");
      return OCCTL_NOT_FOUND;
    }

    TopoDS_Shape aFaceShape;
    if (const occtl_status_t aStatus = ResolveTypedShape(theGraph,
                                                         theOpts->face,
                                                         BRepGraph_NodeId::Kind::Face,
                                                         TopAbs_FACE,
                                                         "target face",
                                                         aFaceShape))
    {
      return aStatus;
    }

    BRepOffsetAPI_NormalProjection aProjection(aFaceShape);
    aProjection.SetParams(theOpts->tolerance_3d,
                          theOpts->tolerance_2d,
                          GeomAbs_C2,
                          theOpts->max_degree,
                          theOpts->max_segments);
    aProjection.SetMaxDistance(theOpts->max_distance);
    aProjection.SetLimit(theOpts->limit_to_face != 0);
    aProjection.Compute3d(theOpts->compute_3d != 0);
    aProjection.Add(aSourceShape);
    aProjection.Build();
    if (!aProjection.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "normal projection failed to produce a valid result");
      return OCCTL_GEOMETRY_INVALID;
    }
    return AddCopiedResult(aProjection.Projection(), "normal projection", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_wrap_on_face_options_init(occtl_topo_wrap_on_face_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_wrap_on_face_options_t anInit = OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_INIT;
  *theOpts                                       = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wrap_on_face(const occtl_graph_t* const                     theGraph,
                          const occtl_topo_wrap_on_face_options_t* const theOpts,
                          occtl_graph_t** const                          theOutGraph,
                          occtl_node_id_t* const                         theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_wrap_on_face_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr || theOpts->tolerance <= 0.0 || theOpts->initial_subdivisions < 1
        || theOpts->max_refinements < 0 || !IsFinitePoint(theOpts->surface_location.location)
        || !IsFiniteNonZeroDirection(theOpts->surface_location.x_dir)
        || !IsFiniteNonZeroDirection(theOpts->surface_location.y_dir)
        || !IsFiniteNonZeroDirection(theOpts->surface_location.z_dir)
        || !IsNonParallel(theOpts->surface_location.x_dir, theOpts->surface_location.z_dir))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "wrap options are invalid");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId aSourceId;
    TopoDS_Shape     aSourceShape;
    if (const occtl_status_t aStatus =
          ResolveShape(theGraph, theOpts->source, "source", aSourceId, aSourceShape))
    {
      return aStatus;
    }
    if (aSourceId.NodeKind != BRepGraph_NodeId::Kind::Edge
        && aSourceId.NodeKind != BRepGraph_NodeId::Kind::Wire
        && aSourceId.NodeKind != BRepGraph_NodeId::Kind::Face)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "source must be an Edge, Wire or Face");
      return OCCTL_WRONG_KIND;
    }

    TopoDS_Shape aTargetShape;
    if (const occtl_status_t aStatus = ResolveTypedShape(theGraph,
                                                         theOpts->target_face,
                                                         BRepGraph_NodeId::Kind::Face,
                                                         TopAbs_FACE,
                                                         "target face",
                                                         aTargetShape))
    {
      return aStatus;
    }
    const TopoDS_Face aTargetFace = TopoDS::Face(aTargetShape);

    const double aExtent = theOpts->intersection_extent > 0.0
                             ? theOpts->intersection_extent
                             : ShapeBoundingExtent(aTargetFace) * 4.0;
    const double aWirePrecision =
      theOpts->wire_fix_tolerance > 0.0 ? theOpts->wire_fix_tolerance : theOpts->tolerance;
    const gp_Pnt aTargetCenter      = ShapeBoundingCenter(aTargetFace);
    const gp_Dir aSurfaceXDirection = OcctL::Geom::ToGp(theOpts->surface_location.x_dir);
    gp_Dir       aSurfaceNormal     = OcctL::Geom::ToGp(theOpts->surface_location.z_dir);
    const gp_Pnt aSurfacePoint      = OcctL::Geom::ToGp(theOpts->surface_location.location);
    FaceNormalAtPoint(aTargetFace, aSurfacePoint, aSurfaceNormal);

    IntCurvesFace_ShapeIntersector anIntersector;
    anIntersector.Load(aTargetFace, theOpts->tolerance);

    auto wrapWire = [&](const TopoDS_Wire& theWire, TopoDS_Wire& theOutWire) -> occtl_status_t {
      NCollection_LinearVector<TopoDS_Edge> aWrappedEdges;
      gp_Pnt                                aCurrentPoint  = aSurfacePoint;
      gp_Dir                                aCurrentNormal = aSurfaceNormal;
      for (BRepTools_WireExplorer anExplorer(theWire); anExplorer.More(); anExplorer.Next())
      {
        const TopoDS_Edge anEdge = anExplorer.Current();
        TopoDS_Edge       aWrappedEdge;
        if (const occtl_status_t aStatus = WrapEdgeFromStart(anEdge,
                                                             aTargetFace,
                                                             anIntersector,
                                                             aTargetCenter,
                                                             aSurfaceXDirection,
                                                             aCurrentPoint,
                                                             aCurrentNormal,
                                                             *theOpts,
                                                             aExtent,
                                                             aWrappedEdge,
                                                             aCurrentPoint,
                                                             aCurrentNormal))
        {
          return aStatus;
        }
        aWrappedEdges.Append(aWrappedEdge);
      }
      if (aWrappedEdges.IsEmpty())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "source wire has no edges");
        return OCCTL_GEOMETRY_INVALID;
      }
      return FixWrappedWire(aWrappedEdges, aWirePrecision, theOutWire);
    };

    TopoDS_Shape aResult;
    if (aSourceId.NodeKind == BRepGraph_NodeId::Kind::Edge)
    {
      TopoDS_Edge aWrappedEdge;
      gp_Pnt      anEndPoint  = aSurfacePoint;
      gp_Dir      anEndNormal = aSurfaceNormal;
      if (const occtl_status_t aStatus = WrapEdgeFromStart(TopoDS::Edge(aSourceShape),
                                                           aTargetFace,
                                                           anIntersector,
                                                           aTargetCenter,
                                                           aSurfaceXDirection,
                                                           aSurfacePoint,
                                                           aSurfaceNormal,
                                                           *theOpts,
                                                           aExtent,
                                                           aWrappedEdge,
                                                           anEndPoint,
                                                           anEndNormal))
      {
        return aStatus;
      }
      aResult = aWrappedEdge;
    }
    else if (aSourceId.NodeKind == BRepGraph_NodeId::Kind::Wire)
    {
      TopoDS_Wire aWrappedWire;
      if (const occtl_status_t aStatus = wrapWire(TopoDS::Wire(aSourceShape), aWrappedWire))
      {
        return aStatus;
      }
      aResult = aWrappedWire;
    }
    else
    {
      const TopoDS_Face aSourceFace = TopoDS::Face(aSourceShape);
      const TopoDS_Wire anOuterWire = BRepTools::OuterWire(aSourceFace);
      TopoDS_Wire       aWrappedOuter;
      if (const occtl_status_t aStatus = wrapWire(anOuterWire, aWrappedOuter))
      {
        return aStatus;
      }

      BRepOffsetAPI_MakeFilling aFilling;
      for (BRepTools_WireExplorer anExplorer(aWrappedOuter); anExplorer.More(); anExplorer.Next())
      {
        aFilling.Add(anExplorer.Current(), GeomAbs_C0);
      }
      aFilling.Add(aSurfacePoint);
      aFilling.Build();
      if (!aFilling.IsDone() || aFilling.Shape().IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "OCCT failed to build wrapped filling face");
        return OCCTL_GEOMETRY_INVALID;
      }

      BRepBuilderAPI_MakeFace aFaceMaker(TopoDS::Face(aFilling.Shape()));
      for (TopExp_Explorer anExp(aSourceFace, TopAbs_WIRE); anExp.More(); anExp.Next())
      {
        const TopoDS_Wire aSourceWire = TopoDS::Wire(anExp.Current());
        if (aSourceWire.IsSame(anOuterWire))
        {
          continue;
        }

        TopoDS_Wire aWrappedHole;
        if (const occtl_status_t aStatus = wrapWire(aSourceWire, aWrappedHole))
        {
          return aStatus;
        }
        aFaceMaker.Add(aWrappedHole);
      }
      aResult = aFaceMaker.Face();
    }

    return AddCopiedResult(aResult, "surface wrap", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_project_face_direction_options_init(
  occtl_topo_project_face_direction_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_project_face_direction_options_t anInit =
    OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_INIT;
  *theOpts = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_project_face_along_direction(
  const occtl_graph_t* const                               theGraph,
  const occtl_topo_project_face_direction_options_t* const theOpts,
  occtl_graph_t** const                                    theOutGraph,
  occtl_node_id_t* const                                   theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_project_face_direction_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (!IsFiniteNonZeroDirection(theOpts->direction) || !IsFiniteValue(theOpts->max_distance))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "projection direction must be finite and non-zero; max_distance must be finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    TopoDS_Shape aSourceShape;
    if (const occtl_status_t aStatus = ResolveTypedShape(theGraph,
                                                         theOpts->source_face,
                                                         BRepGraph_NodeId::Kind::Face,
                                                         TopAbs_FACE,
                                                         "source face",
                                                         aSourceShape))
    {
      return aStatus;
    }

    BRepGraph_NodeId aTargetId;
    TopoDS_Shape     aTargetShape;
    if (const occtl_status_t aStatus =
          ResolveShape(theGraph, theOpts->target, "target", aTargetId, aTargetShape))
    {
      return aStatus;
    }

    TopoDS_Shape aTargetBoundary;
    if (const occtl_status_t aStatus = ProjectionTargetBoundary(aTargetShape, aTargetBoundary))
    {
      return aStatus;
    }

    double aDistance = theOpts->max_distance;
    if (aDistance <= 0.0)
    {
      if (const occtl_status_t aStatus =
            ComputeProjectionDistance(aSourceShape, aTargetShape, aDistance))
      {
        return aStatus;
      }
    }

    const gp_Dir          aDirection = OcctL::Geom::ToGp(theOpts->direction);
    const gp_Vec          aVector(aDirection.XYZ() * aDistance);
    BRepPrimAPI_MakePrism aPrism(aSourceShape, aVector, theOpts->copy_source != 0, false);
    aPrism.Build();
    if (!aPrism.IsDone() || aPrism.Shape().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "projection prism failed to produce a valid result");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepAlgoAPI_Common aCommon(aPrism.Shape(), aTargetBoundary);
    aCommon.Build();
    if (!aCommon.IsDone() || aCommon.Shape().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "directional face projection failed to intersect target boundary");
      return OCCTL_GEOMETRY_INVALID;
    }

    return AddCopiedResult(aCommon.Shape(), "directional face projection", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_face_to_arcs_options_init(occtl_topo_face_to_arcs_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_face_to_arcs_options_t anInit = OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_INIT;
  *theOpts                                       = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_to_arcs(const occtl_graph_t* const                     theGraph,
                          const occtl_topo_face_to_arcs_options_t* const theOpts,
                          occtl_graph_t** const                          theOutGraph,
                          occtl_node_id_t* const                         theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_face_to_arcs_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (!IsFiniteValue(theOpts->angular_tolerance) || theOpts->angular_tolerance <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "angular_tolerance must be positive and finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId aSourceId;
    TopoDS_Shape     aSourceShape;
    if (const occtl_status_t aStatus =
          ResolveShape(theGraph, theOpts->source, "source", aSourceId, aSourceShape))
    {
      return aStatus;
    }
    if (aSourceId.NodeKind != BRepGraph_NodeId::Kind::Face
        && aSourceId.NodeKind != BRepGraph_NodeId::Kind::Wire)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "source must be a Face or Wire");
      return OCCTL_WRONG_KIND;
    }

    TopoDS_Face aFaceToConvert;
    const bool  isWireInput = aSourceId.NodeKind == BRepGraph_NodeId::Kind::Wire;
    if (isWireInput)
    {
      if (aSourceShape.ShapeType() != TopAbs_WIRE)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_NOT_FOUND,
          "source wire could not be reconstructed as TopoDS wire");
        return OCCTL_NOT_FOUND;
      }
      BRepBuilderAPI_MakeFace aMakeFace(TopoDS::Wire(aSourceShape));
      if (!aMakeFace.IsDone() || aMakeFace.Face().IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_GEOMETRY_INVALID,
          "wire could not be converted to an intermediate planar face");
        return OCCTL_GEOMETRY_INVALID;
      }
      aFaceToConvert = aMakeFace.Face();
    }
    else
    {
      if (aSourceShape.ShapeType() != TopAbs_FACE)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_NOT_FOUND,
          "source face could not be reconstructed as TopoDS face");
        return OCCTL_NOT_FOUND;
      }
      aFaceToConvert = TopoDS::Face(aSourceShape);
    }

    const TopoDS_Face aConvertedFace =
      BRepAlgo::ConvertFace(aFaceToConvert, theOpts->angular_tolerance);
    if (aConvertedFace.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "face-to-arcs conversion failed to produce a valid face");
      return OCCTL_GEOMETRY_INVALID;
    }

    if (isWireInput)
    {
      const TopoDS_Wire aConvertedWire = BRepTools::OuterWire(aConvertedFace);
      if (aConvertedWire.IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_GEOMETRY_INVALID,
          "face-to-arcs conversion failed to produce an outer wire");
        return OCCTL_GEOMETRY_INVALID;
      }
      return AddCopiedResult(aConvertedWire, "face-to-arcs wire", theOutGraph, theOutRoot);
    }

    return AddCopiedResult(aConvertedFace, "face-to-arcs face", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_hlr_options_init(occtl_topo_hlr_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_hlr_options_t anInit = OCCTL_TOPO_HLR_OPTIONS_INIT;
  *theOpts                              = anInit;
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_hlr_result_init(occtl_topo_hlr_result_t* const theResult)
{
  if (theResult == nullptr)
  {
    return;
  }
  const occtl_topo_hlr_result_t anInit = OCCTL_TOPO_HLR_RESULT_INIT;
  *theResult                           = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_hlr_projection(const occtl_graph_t* const            theGraph,
                                 const occtl_topo_hlr_options_t* const theOpts,
                                 occtl_topo_hlr_result_t* const        theOutResult)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutResult != nullptr)
    {
      *theOutResult = EmptyHlrResult();
    }
    if (theGraph == nullptr || theOpts == nullptr || theOutResult == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options or out_result is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_HLR_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_hlr_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_topo_hlr_options_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutResult->struct_version != OCCTL_TOPO_HLR_RESULT_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_topo_hlr_result_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOutResult->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_topo_hlr_result_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theOpts->focus))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "focus must be finite");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->mode != OCCTL_TOPO_HLR_BREP && theOpts->mode != OCCTL_TOPO_HLR_POLY)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "HLR mode is not supported");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!ProjectionFrameIsValid(theOpts->projection_frame))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "projection_frame must contain finite non-parallel X and Z directions");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aRootId;
    TopoDS_Shape     aRootShape;
    if (const occtl_status_t aStatus =
          ResolveShape(theGraph, theOpts->root, "HLR root", aRootId, aRootShape))
    {
      return aStatus;
    }

    const gp_Ax2 aProjectionFrame(OcctL::Geom::ToGp(theOpts->projection_frame.location),
                                  gp_Dir(theOpts->projection_frame.z_dir.x,
                                         theOpts->projection_frame.z_dir.y,
                                         theOpts->projection_frame.z_dir.z),
                                  gp_Dir(theOpts->projection_frame.x_dir.x,
                                         theOpts->projection_frame.x_dir.y,
                                         theOpts->projection_frame.x_dir.z));

    std::unique_ptr<occtl_graph> aResultGraph(new occtl_graph());
    occtl_topo_hlr_result_t      aResult        = EmptyHlrResult();
    int                          aCategoryCount = 0;
    occtl_status_t               aStatus        = OCCTL_OK;
    if (theOpts->mode == OCCTL_TOPO_HLR_POLY)
    {
      aStatus =
        RunPolyHlr(aRootShape, aProjectionFrame, *theOpts, *aResultGraph, aResult, aCategoryCount);
    }
    else
    {
      aStatus =
        RunBRepHlr(aRootShape, aProjectionFrame, *theOpts, *aResultGraph, aResult, aCategoryCount);
    }
    if (aStatus != OCCTL_OK)
    {
      return aStatus;
    }

    if (aCategoryCount == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "HLR produced no requested edge categories");
      return OCCTL_GEOMETRY_INVALID;
    }

    aResult.graph = aResultGraph.release();
    *theOutResult = aResult;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_draft_faces_options_init(occtl_topo_draft_faces_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_draft_faces_options_t anInit = OCCTL_TOPO_DRAFT_FACES_OPTIONS_INIT;
  *theOpts                                      = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_draft_faces(const occtl_graph_t* const                    theGraph,
                         const occtl_topo_draft_faces_options_t* const theOpts,
                         occtl_graph_t** const                         theOutGraph,
                         occtl_node_id_t* const                        theOutRoot)
{
  return OcctL::Core::Guard(
    [&]() -> occtl_status_t { return draftFacesImpl(theGraph, theOpts, theOutGraph, theOutRoot); });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_defeature_options_init(occtl_topo_defeature_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_defeature_options_t anInit = OCCTL_TOPO_DEFEATURE_OPTIONS_INIT;
  *theOpts                                    = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_defeature(const occtl_graph_t* const                  theGraph,
                       const occtl_topo_defeature_options_t* const theOpts,
                       occtl_graph_t** const                       theOutGraph,
                       occtl_node_id_t* const                      theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return removeFeaturesImpl(theGraph, theOpts, theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_offset_features_options_init(occtl_topo_offset_features_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_offset_features_options_t anInit = OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT;
  *theOpts                                          = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_offset_features(const occtl_graph_t* const                        theGraph,
                                  const occtl_topo_offset_features_options_t* const theOpts,
                                  occtl_graph_t** const                             theOutGraph,
                                  occtl_node_id_t* const                            theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return offsetFeaturesImpl(theGraph, theOpts, theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_filling_options_init(occtl_topo_filling_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_filling_options_t anInit = OCCTL_TOPO_FILLING_OPTIONS_INIT;
  *theOpts                                  = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_filling(const occtl_graph_t* const                theGraph,
                          const occtl_topo_filling_options_t* const theOpts,
                          occtl_graph_t** const                     theOutGraph,
                          occtl_node_id_t* const                    theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return makeFillingImpl(theGraph, theOpts, theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_filling_patch_options_init(occtl_topo_filling_patch_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_filling_patch_options_t anInit = OCCTL_TOPO_FILLING_PATCH_OPTIONS_INIT;
  *theOpts                                        = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_filling_patch(const occtl_graph_t* const                      theGraph,
                                const occtl_topo_filling_patch_options_t* const theOpts,
                                occtl_graph_t** const                           theOutGraph,
                                occtl_node_id_t* const                          theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_FILLING_PATCH_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_filling_patch_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr || theOpts->edges == nullptr || theOpts->edge_count < 2
        || (theOpts->points == nullptr && theOpts->point_count != 0) || theOpts->degree < 1
        || theOpts->point_count_on_curve < 1 || theOpts->iteration_count < 1
        || theOpts->tolerance_2d <= 0.0 || theOpts->tolerance_3d <= 0.0
        || theOpts->angular_tolerance <= 0.0 || theOpts->curvature_tolerance <= 0.0
        || theOpts->max_degree < 1 || theOpts->max_segments < 1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "filling patch constraints or solver parameters are invalid");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    BRepOffsetAPI_MakeFilling aFilling(theOpts->degree,
                                       theOpts->point_count_on_curve,
                                       theOpts->iteration_count,
                                       theOpts->anisotropic != 0,
                                       theOpts->tolerance_2d,
                                       theOpts->tolerance_3d,
                                       theOpts->angular_tolerance,
                                       theOpts->curvature_tolerance,
                                       theOpts->max_degree,
                                       theOpts->max_segments);
    aFilling.SetConstrParam(theOpts->tolerance_2d,
                            theOpts->tolerance_3d,
                            theOpts->angular_tolerance,
                            theOpts->curvature_tolerance);
    aFilling.SetResolParam(theOpts->degree,
                           theOpts->point_count_on_curve,
                           theOpts->iteration_count,
                           theOpts->anisotropic != 0);
    aFilling.SetApproxParam(theOpts->max_degree, theOpts->max_segments);

    for (size_t anIdx = 0; anIdx < theOpts->edge_count; ++anIdx)
    {
      const occtl_topo_filling_patch_edge_t& aConstraint = theOpts->edges[anIdx];
      const GeomAbs_Shape aContinuity = ToGeomAbsContinuity(aConstraint.continuity);
      if (aConstraint.continuity != OCCTL_TOPO_FILLING_C0
          && aConstraint.continuity != OCCTL_TOPO_FILLING_G1
          && aConstraint.continuity != OCCTL_TOPO_FILLING_G2)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                               "unsupported filling patch continuity");
        return OCCTL_INVALID_ARGUMENT;
      }

      TopoDS_Shape anEdgeShape;
      if (const occtl_status_t aStatus = ResolveTypedShape(theGraph,
                                                           aConstraint.edge,
                                                           BRepGraph_NodeId::Kind::Edge,
                                                           TopAbs_EDGE,
                                                           "filling patch edge",
                                                           anEdgeShape))
      {
        return aStatus;
      }

      if (aConstraint.support_face.bits != 0u)
      {
        TopoDS_Shape aSupportShape;
        if (const occtl_status_t aStatus = ResolveTypedShape(theGraph,
                                                             aConstraint.support_face,
                                                             BRepGraph_NodeId::Kind::Face,
                                                             TopAbs_FACE,
                                                             "filling patch support face",
                                                             aSupportShape))
        {
          return aStatus;
        }
        aFilling.Add(TopoDS::Edge(anEdgeShape),
                     TopoDS::Face(aSupportShape),
                     aContinuity,
                     aConstraint.is_boundary != 0);
      }
      else
      {
        if (aConstraint.continuity != OCCTL_TOPO_FILLING_C0)
        {
          OcctL::Core::ErrorState::Current().Set(
            OCCTL_INVALID_ARGUMENT,
            "G1/G2 filling patch constraints require a support face");
          return OCCTL_INVALID_ARGUMENT;
        }
        aFilling.Add(TopoDS::Edge(anEdgeShape), aContinuity, aConstraint.is_boundary != 0);
      }
    }

    for (size_t aPointIdx = 0; aPointIdx < theOpts->point_count; ++aPointIdx)
    {
      const occtl_point3_t aPoint = theOpts->points[aPointIdx];
      if (!IsFiniteValue(aPoint.x) || !IsFiniteValue(aPoint.y) || !IsFiniteValue(aPoint.z))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                               "filling patch point constraints must be finite");
        return OCCTL_INVALID_ARGUMENT;
      }
      aFilling.Add(OcctL::Geom::ToGp(aPoint));
    }

    aFilling.Build();
    if (!aFilling.IsDone() || aFilling.Shape().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "filling patch failed to produce a valid result");
      return OCCTL_GEOMETRY_INVALID;
    }

    return AddCopiedResult(aFilling.Shape(), "filling patch", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_split_by_plane_options_init(occtl_topo_split_by_plane_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_split_by_plane_options_t anInit = OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_INIT;
  *theOpts                                         = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_split_by_plane(const occtl_graph_t* const                       theGraph,
                                 const occtl_topo_split_by_plane_options_t* const theOpts,
                                 occtl_graph_t** const                            theOutGraph,
                                 occtl_node_id_t* const                           theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_split_by_plane_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->keep != OCCTL_TOPO_SPLIT_KEEP_ALL
        && theOpts->keep != OCCTL_TOPO_SPLIT_KEEP_POSITIVE
        && theOpts->keep != OCCTL_TOPO_SPLIT_KEEP_NEGATIVE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "split keep mode is not supported");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId aRootId;
    TopoDS_Shape     aShape;
    if (const occtl_status_t aStatus =
          ResolveShape(theGraph, theOpts->root, "split root", aRootId, aShape))
    {
      return aStatus;
    }

    const gp_Pnt aPoint  = OcctL::Geom::ToGp(theOpts->point);
    const gp_Dir aNormal = OcctL::Geom::ToGp(theOpts->normal);

    double anExtent = 0.0;
    if (const occtl_status_t aStatus = ComputeSplitPlaneEnvelope(aShape, aPoint, anExtent))
    {
      return aStatus;
    }

    const TopoDS_Face aPlaneFace = MakeSplitPlaneFace(aPoint, aNormal, anExtent);
    if (aPlaneFace.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "split plane face could not be constructed");
      return OCCTL_GEOMETRY_INVALID;
    }

    if (theOpts->keep == OCCTL_TOPO_SPLIT_KEEP_ALL)
    {
      NCollection_List<TopoDS_Shape> anArgs;
      NCollection_List<TopoDS_Shape> aTools;
      anArgs.Append(aShape);
      aTools.Append(aPlaneFace);

      BRepAlgoAPI_Splitter aSplitter;
      aSplitter.SetArguments(anArgs);
      aSplitter.SetTools(aTools);
      aSplitter.Build();
      if (!aSplitter.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "split by plane failed");
        return OCCTL_GEOMETRY_INVALID;
      }

      return AddCopiedResult(aSplitter.Shape(), "split by plane", theOutGraph, theOutRoot);
    }

    const double aSideScale = theOpts->keep == OCCTL_TOPO_SPLIT_KEEP_POSITIVE ? 1.0 : -1.0;
    const gp_Pnt aReference = aPoint.Translated(gp_Vec(aNormal) * (anExtent * aSideScale));
    BRepPrimAPI_MakeHalfSpace aHalfSpaceBuilder(aPlaneFace, aReference);
    const TopoDS_Solid        aHalfSpace = aHalfSpaceBuilder.Solid();
    if (aHalfSpace.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "split half-space could not be constructed");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepAlgoAPI_Common aCommon(aShape, aHalfSpace);
    aCommon.Build();
    if (!aCommon.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "split half-space intersection failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    return AddCopiedResult(aCommon.Shape(), "split by plane", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_section_by_planes_options_init(occtl_topo_section_by_planes_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_section_by_planes_options_t anInit = OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_INIT;
  *theOpts                                            = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_sections_by_planes(const occtl_graph_t* const                          theGraph,
                                     const occtl_topo_section_by_planes_options_t* const theOpts,
                                     occtl_graph_t** const  theOutGraph,
                                     occtl_node_id_t* const theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_section_by_planes_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->planes == nullptr || theOpts->plane_count == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "section plane list is empty");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId aRootId;
    TopoDS_Shape     aShape;
    if (const occtl_status_t aStatus =
          ResolveShape(theGraph, theOpts->root, "section root", aRootId, aShape))
    {
      return aStatus;
    }

    TopoDS_Compound aCompound;
    BRep_Builder    aBuilder;
    aBuilder.MakeCompound(aCompound);
    bool hasSectionShape = false;

    for (size_t aPlaneIdx = 0; aPlaneIdx < theOpts->plane_count; ++aPlaneIdx)
    {
      const gp_Pnt aPoint  = OcctL::Geom::ToGp(theOpts->planes[aPlaneIdx].point);
      const gp_Dir aNormal = OcctL::Geom::ToGp(theOpts->planes[aPlaneIdx].normal);
      const gp_Pln aPlane(aPoint, aNormal);

      BRepAlgoAPI_Section aSection(aShape, aPlane, false);
      aSection.Approximation(theOpts->approximate != 0);
      aSection.ComputePCurveOn1(theOpts->compute_pcurves_on_root != 0);
      aSection.ComputePCurveOn2(theOpts->compute_pcurves_on_plane != 0);
      aSection.Build();
      if (!aSection.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "section by plane failed");
        return OCCTL_GEOMETRY_INVALID;
      }

      const TopoDS_Shape aSectionShape = aSection.Shape();
      if (aSectionShape.IsNull())
      {
        continue;
      }
      if (theOpts->plane_count == 1)
      {
        return AddCopiedResult(aSectionShape, "section by planes", theOutGraph, theOutRoot);
      }
      aBuilder.Add(aCompound, aSectionShape);
      hasSectionShape = true;
    }

    if (!hasSectionShape)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "section by planes produced no result shape");
      return OCCTL_GEOMETRY_INVALID;
    }
    return AddCopiedResult(aCompound, "section by planes", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_face_from_surface_options_init(occtl_prim_face_from_surface_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_prim_face_from_surface_options_t anInit = OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_INIT;
  *theOpts                                            = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_face_from_surface(occtl_graph_t* const                                theGraph,
                                    const occtl_prim_face_from_surface_options_t* const theOpts,
                                    occtl_node_id_t* const                              theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutFace = OCCTL_NODE_ID_INVALID;

    if (theOpts->struct_version != OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_prim_face_from_surface_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_prim_face_from_surface_options_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->surface_id.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "surface_id is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theOpts->tolerance) || theOpts->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tolerance is negative or non-finite");
      return OCCTL_INVALID_ARGUMENT;
    }
    const bool hasOuterWire = OcctL::Topo::UnpackNodeId(theOpts->outer_wire).IsValid();
    if (theOpts->inner_wire_count > 0 && theOpts->inner_wires == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "inner_wires is NULL when count is non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->inner_wire_count == 0 && theOpts->inner_wires != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "inner_wires is non-NULL when count is zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!hasOuterWire && theOpts->inner_wire_count > 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "inner wires require an outer wire");
      return OCCTL_INVALID_ARGUMENT;
    }

    std::unique_ptr<BRepBuilderAPI_MakeFace> aMaker;
    if (hasOuterWire)
    {
      BRepGraph_NodeId anOuterId;
      TopoDS_Shape     anOuterShape;
      if (const occtl_status_t aStatus =
            ResolveShape(theGraph, theOpts->outer_wire, "outer wire", anOuterId, anOuterShape))
      {
        return aStatus;
      }
      if (anOuterId.NodeKind != BRepGraph_NodeId::Kind::Wire
          || anOuterShape.ShapeType() != TopAbs_WIRE)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "outer_wire is not a Wire node");
        return OCCTL_WRONG_KIND;
      }

      const occ::handle<Geom_Surface>& aLocalSurface =
        OcctL::Geom::SurfaceFromRep(theGraph, theOpts->surface_id);
      aMaker.reset(new BRepBuilderAPI_MakeFace(aLocalSurface, TopoDS::Wire(anOuterShape), true));
      for (size_t anIdx = 0; anIdx < theOpts->inner_wire_count; ++anIdx)
      {
        BRepGraph_NodeId anInnerId;
        TopoDS_Shape     anInnerShape;
        if (const occtl_status_t aStatus = ResolveShape(theGraph,
                                                        theOpts->inner_wires[anIdx],
                                                        "inner wire",
                                                        anInnerId,
                                                        anInnerShape))
        {
          return aStatus;
        }
        if (anInnerId.NodeKind != BRepGraph_NodeId::Kind::Wire
            || anInnerShape.ShapeType() != TopAbs_WIRE)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                                 "inner_wires entry is not a Wire node");
          return OCCTL_WRONG_KIND;
        }
        aMaker->Add(TopoDS::Wire(anInnerShape));
      }
    }
    else
    {
      aMaker.reset(new BRepBuilderAPI_MakeFace());
      aMaker->Init(OcctL::Geom::SurfaceFromRep(theGraph, theOpts->surface_id),
                   true,
                   theOpts->tolerance);
    }

    if (!aMaker->IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not build face from surface");
      return OCCTL_GEOMETRY_INVALID;
    }

    const TopoDS_Face aFace = aMaker->Face();
    if (aFace.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "face from surface result is null");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepGraph::ShapesView::Options aBuildOpts;
    aBuildOpts.CreateAutoProduct                  = false;
    const BRepGraph::ShapesView::Result aBuildRes = theGraph->graph.Shapes().Add(aFace, aBuildOpts);
    if (!aBuildRes.Ok || !aBuildRes.TopologyRoot.IsValid()
        || aBuildRes.TopologyRoot.NodeKind != BRepGraph_NodeId::Kind::Face)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "face from surface result could not be ingested as graph Face");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutFace = OcctL::Topo::PackNodeId(aBuildRes.TopologyRoot);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_face_from_point_grid_options_init(
  occtl_prim_face_from_point_grid_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_prim_face_from_point_grid_options_t anInit =
    OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_INIT;
  *theOpts = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_face_from_point_grid(
  occtl_graph_t* const                                   theGraph,
  const occtl_prim_face_from_point_grid_options_t* const theOpts,
  occtl_node_id_t* const                                 theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutFace = OCCTL_NODE_ID_INVALID;

    if (theOpts->struct_version != OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_prim_face_from_point_grid_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_prim_face_from_point_grid_options_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theOpts->tolerance) || theOpts->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tolerance is negative or non-finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    occtl_rep_id_t       aSurfaceId = OCCTL_REP_ID_INVALID;
    const occtl_status_t aSurfaceStatus =
      occtl_surface_create_from_point_grid(theGraph, &aSurfaceId, &theOpts->surface);
    if (aSurfaceStatus != OCCTL_OK)
    {
      return aSurfaceStatus;
    }

    occtl_prim_face_from_surface_options_t aFaceOpts = OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_INIT;
    aFaceOpts.surface_id                             = aSurfaceId;
    aFaceOpts.tolerance                              = theOpts->tolerance;

    const occtl_status_t aFaceStatus =
      occtl_prim_make_face_from_surface(theGraph, &aFaceOpts, theOutFace);
    return aFaceStatus;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_face_from_boundary_curves_options_init(
  occtl_prim_face_from_boundary_curves_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_prim_face_from_boundary_curves_options_t anInit =
    OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_INIT;
  *theOpts = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_face_from_boundary_curves(
  occtl_graph_t* const                                        theGraph,
  const occtl_prim_face_from_boundary_curves_options_t* const theOpts,
  occtl_node_id_t* const                                      theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutFace = OCCTL_NODE_ID_INVALID;

    if (theOpts->struct_version != OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_prim_face_from_boundary_curves_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_prim_face_from_boundary_curves_options_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theOpts->tolerance) || theOpts->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tolerance is negative or non-finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    occtl_rep_id_t       aSurfaceId = OCCTL_REP_ID_INVALID;
    const occtl_status_t aSurfaceStatus =
      occtl_surface_create_from_boundary_curves(theGraph, &aSurfaceId, &theOpts->surface);
    if (aSurfaceStatus != OCCTL_OK)
    {
      return aSurfaceStatus;
    }

    occtl_prim_face_from_surface_options_t aFaceOpts = OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_INIT;
    aFaceOpts.surface_id                             = aSurfaceId;
    aFaceOpts.tolerance                              = theOpts->tolerance;

    const occtl_status_t aFaceStatus =
      occtl_prim_make_face_from_surface(theGraph, &aFaceOpts, theOutFace);
    return aFaceStatus;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_face_from_curve_grid_options_init(
  occtl_prim_face_from_curve_grid_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_prim_face_from_curve_grid_options_t anInit =
    OCCTL_PRIM_FACE_FROM_CURVE_GRID_OPTIONS_INIT;
  *theOpts = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_face_from_curve_grid(
  occtl_graph_t* const                                   theGraph,
  const occtl_prim_face_from_curve_grid_options_t* const theOpts,
  occtl_node_id_t* const                                 theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutFace = OCCTL_NODE_ID_INVALID;

    if (theOpts->struct_version != OCCTL_PRIM_FACE_FROM_CURVE_GRID_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_prim_face_from_curve_grid_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_prim_face_from_curve_grid_options_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theOpts->tolerance) || theOpts->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tolerance is negative or non-finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    occtl_rep_id_t       aSurfaceId = OCCTL_REP_ID_INVALID;
    const occtl_status_t aSurfaceStatus =
      occtl_surface_create_from_curve_grid(theGraph, &aSurfaceId, &theOpts->surface);
    if (aSurfaceStatus != OCCTL_OK)
    {
      return aSurfaceStatus;
    }

    occtl_prim_face_from_surface_options_t aFaceOpts = OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_INIT;
    aFaceOpts.surface_id                             = aSurfaceId;
    aFaceOpts.tolerance                              = theOpts->tolerance;

    const occtl_status_t aFaceStatus =
      occtl_prim_make_face_from_surface(theGraph, &aFaceOpts, theOutFace);
    return aFaceStatus;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_extrude_faces_options_init(occtl_topo_extrude_faces_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_extrude_faces_options_t anInit = OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_INIT;
  *theOpts                                        = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_face_extrusion(const occtl_graph_t* const                      theGraph,
                                 const occtl_topo_extrude_faces_options_t* const theOpts,
                                 occtl_graph_t** const                           theOutGraph,
                                 occtl_node_id_t* const                          theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_extrude_faces_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->faces == nullptr || theOpts->face_count == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "thicken face list is empty");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!(theOpts->thickness > 0.0) || !IsFiniteValue(theOpts->thickness))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "thickness must be positive and finite");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->use_normal != 0 && !IsFiniteNonZeroDirection(theOpts->normal))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "normal is not a finite non-zero direction");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    TopoDS_Compound aCompound;
    BRep_Builder    aBuilder;
    aBuilder.MakeCompound(aCompound);

    TopoDS_Shape aSingleResult;
    for (size_t aFaceIdx = 0; aFaceIdx < theOpts->face_count; ++aFaceIdx)
    {
      TopoDS_Shape aFaceShape;
      if (const occtl_status_t aStatus = ResolveTypedShape(theGraph,
                                                           theOpts->faces[aFaceIdx],
                                                           BRepGraph_NodeId::Kind::Face,
                                                           TopAbs_FACE,
                                                           "thicken face",
                                                           aFaceShape))
      {
        return aStatus;
      }

      const TopoDS_Face aFace = TopoDS::Face(aFaceShape);
      gp_Dir            aDirection;
      if (theOpts->use_normal != 0)
      {
        aDirection = OcctL::Geom::ToGp(theOpts->normal);
      }
      else if (!FaceMidNormal(aFace, aDirection))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "could not evaluate thicken face normal");
        return OCCTL_GEOMETRY_INVALID;
      }

      TopoDS_Shape aBaseShape = aFace;
      if (theOpts->both_sides != 0)
      {
        gp_Trsf aShift;
        aShift.SetTranslation(gp_Vec(aDirection) * (-0.5 * theOpts->thickness));
        BRepBuilderAPI_Transform aTransform(aFace, aShift, true);
        aBaseShape = aTransform.Shape();
        if (aBaseShape.IsNull())
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                                 "thicken centered base transform failed");
          return OCCTL_GEOMETRY_INVALID;
        }
      }

      const gp_Vec          aVector = gp_Vec(aDirection) * theOpts->thickness;
      BRepPrimAPI_MakePrism aMaker(aBaseShape, aVector, theOpts->copy != 0, theOpts->canonize != 0);
      aMaker.Build();
      if (!aMaker.IsDone() || aMaker.Shape().IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "thicken face prism failed");
        return OCCTL_GEOMETRY_INVALID;
      }

      if (theOpts->face_count == 1)
      {
        aSingleResult = aMaker.Shape();
      }
      else
      {
        aBuilder.Add(aCompound, aMaker.Shape());
      }
    }

    if (theOpts->face_count == 1)
    {
      return AddCopiedResult(aSingleResult, "thicken faces", theOutGraph, theOutRoot);
    }
    return AddCopiedResult(aCompound, "thicken faces", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_brake_formed_options_init(occtl_prim_brake_formed_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_prim_brake_formed_options_t anInit = OCCTL_PRIM_BRAKE_FORMED_OPTIONS_INIT;
  *theOpts                                       = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_brake_formed(const occtl_graph_t* const                     theGraph,
                               const occtl_prim_brake_formed_options_t* const theOpts,
                               occtl_graph_t** const                          theOutGraph,
                               occtl_node_id_t* const                         theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_PRIM_BRAKE_FORMED_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_prim_brake_formed_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "options->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theOpts->thickness) || theOpts->thickness <= 0.0
        || !IsFiniteValue(theOpts->tolerance) || theOpts->tolerance <= 0.0
        || theOpts->station_widths == nullptr || theOpts->station_width_count == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "brake-formed dimensions are invalid");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsValidBrakeSide(theOpts->side) || !IsValidWireOffsetJoin(theOpts->join)
        || (theOpts->approximate != 0 && theOpts->approximate != 1))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "brake-formed side, join, or approximate flag is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }
    for (size_t aWidthIdx = 0; aWidthIdx < theOpts->station_width_count; ++aWidthIdx)
    {
      if (!IsFiniteValue(theOpts->station_widths[aWidthIdx])
          || theOpts->station_widths[aWidthIdx] <= 0.0)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                               "station widths must be finite and positive");
        return OCCTL_INVALID_ARGUMENT;
      }
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId aLineNode;
    TopoDS_Shape     aLineShape;
    if (const occtl_status_t aStatus =
          ResolveShape(theGraph, theOpts->line, "brake line", aLineNode, aLineShape))
    {
      return aStatus;
    }
    if (aLineShape.ShapeType() != TopAbs_EDGE && aLineShape.ShapeType() != TopAbs_WIRE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "brake line must be an Edge or Wire");
      return OCCTL_WRONG_KIND;
    }

    TopoDS_Wire aLineWire;
    if (!LineShapeToWire(aLineShape, aLineWire))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "brake line could not be converted to a Wire");
      return OCCTL_GEOMETRY_INVALID;
    }

    const double aSignedThickness =
      theOpts->side == OCCTL_PRIM_BRAKE_SIDE_LEFT ? theOpts->thickness : -theOpts->thickness;
    BRepOffsetAPI_MakeOffset anOffsetMaker(aLineWire, ToWireOffsetJoin(theOpts->join), false);
    anOffsetMaker.SetApprox(theOpts->approximate != 0);
    anOffsetMaker.Perform(aSignedThickness, 0.0);
    if (!anOffsetMaker.IsDone() || anOffsetMaker.Shape().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "brake line offset failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    TopoDS_Wire anOffsetWire;
    if (!FirstWire(anOffsetMaker.Shape(), anOffsetWire))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "brake line offset did not produce a Wire");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepBuilderAPI_MakeFace aPlaneFaceMaker(anOffsetWire);
    if (!aPlaneFaceMaker.IsDone() || aPlaneFaceMaker.Face().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "brake line offset is not faceable");
      return OCCTL_GEOMETRY_INVALID;
    }
    BRepAdaptor_Surface aSurface(aPlaneFaceMaker.Face());
    if (aSurface.GetType() != GeomAbs_Plane)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "brake line offset is not planar");
      return OCCTL_GEOMETRY_INVALID;
    }
    const gp_Dir aWidthDirection = aSurface.Plane().Axis().Direction();

    const NCollection_LinearVector<TopoDS_Vertex> aLineVertices = OrderedWireVertices(aLineWire);
    const NCollection_LinearVector<TopoDS_Vertex> anOffsetVertices =
      OrderedWireVertices(anOffsetWire);
    if (aLineVertices.Size() < 2 || anOffsetVertices.Size() < 2)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "brake line needs at least two stations");
      return OCCTL_GEOMETRY_INVALID;
    }
    if (theOpts->station_width_count != 1 && theOpts->station_width_count != aLineVertices.Size())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "station_width_count must be 1 or equal to line station count");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepOffsetAPI_ThruSections aThru(/* isSolid */ true,
                                     /* ruled */ true,
                                     theOpts->tolerance);
    aThru.CheckCompatibility(true);
    for (size_t aStationIdx = 0; aStationIdx < aLineVertices.Size(); ++aStationIdx)
    {
      const TopoDS_Vertex& aLineVertex = aLineVertices.Value(aStationIdx);
      const gp_Pnt         aLinePoint  = BRep_Tool::Pnt(aLineVertex);

      TopoDS_Vertex anOffsetVertex;
      if (!FindThicknessMatchedVertex(anOffsetVertices,
                                      aLinePoint,
                                      theOpts->thickness,
                                      anOffsetVertex))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "could not match brake station to offset station");
        return OCCTL_GEOMETRY_INVALID;
      }

      BRepBuilderAPI_MakeEdge aStationEdgeMaker(aLineVertex, anOffsetVertex);
      if (!aStationEdgeMaker.IsDone() || aStationEdgeMaker.Edge().IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "could not build brake station edge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const double          aStationWidth = theOpts->station_width_count == 1
                                              ? theOpts->station_widths[0]
                                              : theOpts->station_widths[aStationIdx];
      const gp_Vec          aWidthVector  = gp_Vec(aWidthDirection) * aStationWidth;
      BRepPrimAPI_MakePrism aStationPrism(aStationEdgeMaker.Edge(), aWidthVector, true, true);
      aStationPrism.Build();
      if (!aStationPrism.IsDone() || aStationPrism.Shape().IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "could not build brake station face");
        return OCCTL_GEOMETRY_INVALID;
      }

      TopoDS_Face aStationFace;
      if (!FirstFace(aStationPrism.Shape(), aStationFace))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "brake station did not produce a Face");
        return OCCTL_GEOMETRY_INVALID;
      }
      const TopoDS_Wire aStationWire = BRepTools::OuterWire(aStationFace);
      if (aStationWire.IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "brake station Face has no outer Wire");
        return OCCTL_GEOMETRY_INVALID;
      }
      aThru.AddWire(aStationWire);
    }

    aThru.Build();
    if (!aThru.IsDone() || aThru.Shape().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "brake-formed solid skinning failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    return AddCopiedResult(aThru.Shape(), "brake formed", theOutGraph, theOutRoot);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_mirrored(const occtl_graph_t* const theGraph,
                                                        const occtl_node_id_t      theRoot,
                                                        const occtl_point3_t       thePoint,
                                                        const occtl_direction3_t   theNormal,
                                                        occtl_graph_t** const      theOutGraph,
                                                        occtl_node_id_t* const     theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theRoot);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "root NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(aNodeId);
    if (aShape.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "root could not be reconstructed as TopoDS shape");
      return OCCTL_NOT_FOUND;
    }

    const gp_Pnt aPt   = OcctL::Geom::ToGp(thePoint);
    const gp_Dir aNorm = OcctL::Geom::ToGp(theNormal);
    gp_Trsf      aTrsf;
    aTrsf.SetMirror(gp_Ax2(aPt, aNorm));

    BRepBuilderAPI_Transform aTransform(aShape, aTrsf, true);
    const TopoDS_Shape       aResult = aTransform.Shape();
    if (aResult.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "mirror result shape is null");
      return OCCTL_GEOMETRY_INVALID;
    }

    occtl_graph*                   aNewGraph = new occtl_graph();
    BRepGraph::ShapesView::Options anBuildOpts;
    anBuildOpts.CreateAutoProduct = false;
    anBuildOpts.TrackAddedNodes   = true;
    const BRepGraph::ShapesView::Result aBuildRes =
      aNewGraph->graph.Shapes().Add(aResult, anBuildOpts);
    if (!aBuildRes.Ok)
    {
      delete aNewGraph;
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "mirror result topology could not be merged into graph");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutGraph = aNewGraph;
    *theOutRoot  = OcctL::Topo::PackNodeId(aBuildRes.TopologyRoot);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_linear_pattern_options_init(occtl_topo_linear_pattern_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_linear_pattern_options_t anInit = OCCTL_TOPO_LINEAR_PATTERN_OPTIONS_INIT;
  *theOpts                                         = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_linear_pattern(const occtl_graph_t* const                       theGraph,
                                 const occtl_node_id_t                            theRoot,
                                 const occtl_topo_linear_pattern_options_t* const theOpts,
                                 occtl_graph_t** const                            theOutGraph,
                                 occtl_node_id_t* const                           theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, opts, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_LINEAR_PATTERN_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_linear_pattern_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "opts->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->count < 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "count must be at least 1");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!VectorIsFiniteNonZero(theOpts->direction))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "direction is not a finite non-zero vector");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theOpts->step))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "step must be finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theRoot);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "root NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(aNodeId);
    if (aShape.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "root could not be reconstructed as TopoDS shape");
      return OCCTL_NOT_FOUND;
    }

    const gp_Vec    aDir = OcctL::Geom::ToGp(theOpts->direction);
    TopoDS_Compound aCmp;
    BRep_Builder    aBuilder;
    aBuilder.MakeCompound(aCmp);

    for (int32_t anIdx = 0; anIdx < theOpts->count; ++anIdx)
    {
      if (anIdx == 0)
      {
        aBuilder.Add(aCmp, aShape);
      }
      else
      {
        gp_Trsf aTrsf;
        aTrsf.SetTranslation(aDir * theOpts->step * static_cast<double>(anIdx));
        BRepBuilderAPI_Transform aTransform(aShape, aTrsf, true);
        aBuilder.Add(aCmp, aTransform.Shape());
      }
    }

    occtl_graph*                   aNewGraph = new occtl_graph();
    BRepGraph::ShapesView::Options anBuildOpts;
    anBuildOpts.CreateAutoProduct = false;
    anBuildOpts.TrackAddedNodes   = true;
    const BRepGraph::ShapesView::Result aBuildRes =
      aNewGraph->graph.Shapes().Add(aCmp, anBuildOpts);
    if (!aBuildRes.Ok)
    {
      delete aNewGraph;
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "linear pattern result topology could not be merged into graph");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutGraph = aNewGraph;
    *theOutRoot  = OcctL::Topo::PackNodeId(aBuildRes.TopologyRoot);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_circular_pattern_options_init(occtl_topo_circular_pattern_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_topo_circular_pattern_options_t anInit = OCCTL_TOPO_CIRCULAR_PATTERN_OPTIONS_INIT;
  *theOpts                                           = anInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_circular_pattern(const occtl_graph_t* const                         theGraph,
                                   const occtl_node_id_t                              theRoot,
                                   const occtl_topo_circular_pattern_options_t* const theOpts,
                                   occtl_graph_t** const                              theOutGraph,
                                   occtl_node_id_t* const                             theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOpts == nullptr || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, opts, out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->struct_version != OCCTL_TOPO_CIRCULAR_PATTERN_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_topo_circular_pattern_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOpts->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "opts->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOpts->count < 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "count must be at least 1");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!PointIsFinite(theOpts->axis.location)
        || !DirectionIsFiniteNonZero(theOpts->axis.direction))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "axis location/direction are invalid");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theOpts->angle))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "angle must be finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theRoot);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "root NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(aNodeId);
    if (aShape.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "root could not be reconstructed as TopoDS shape");
      return OCCTL_NOT_FOUND;
    }

    const gp_Ax1    anAxis = OcctL::Geom::ToGpAx1(theOpts->axis);
    TopoDS_Compound aCmp;
    BRep_Builder    aBuilder;
    aBuilder.MakeCompound(aCmp);

    for (int32_t anIdx = 0; anIdx < theOpts->count; ++anIdx)
    {
      if (anIdx == 0)
      {
        aBuilder.Add(aCmp, aShape);
      }
      else
      {
        gp_Trsf aTrsf;
        aTrsf.SetRotation(anAxis, theOpts->angle * static_cast<double>(anIdx));
        BRepBuilderAPI_Transform aTransform(aShape, aTrsf, true);
        aBuilder.Add(aCmp, aTransform.Shape());
      }
    }

    occtl_graph*                   aNewGraph = new occtl_graph();
    BRepGraph::ShapesView::Options anBuildOpts;
    anBuildOpts.CreateAutoProduct = false;
    anBuildOpts.TrackAddedNodes   = true;
    const BRepGraph::ShapesView::Result aBuildRes =
      aNewGraph->graph.Shapes().Add(aCmp, anBuildOpts);
    if (!aBuildRes.Ok)
    {
      delete aNewGraph;
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "circular pattern result topology could not be merged into graph");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutGraph = aNewGraph;
    *theOutRoot  = OcctL::Topo::PackNodeId(aBuildRes.TopologyRoot);
    return OCCTL_OK;
  });
}

} // extern "C"
