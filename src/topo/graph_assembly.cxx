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
#include "../geom/GeomMath.hxx"

#include <BRepGraph_ChildExplorer.hxx>
#include <BRepGraph_EditorView.hxx>
#include <BRepGraph_RefsIterator.hxx>
#include <BRepGraph_TopoView.hxx>

#include <BRepGraphInc_Definition.hxx>
#include <BRepGraphInc_Instance.hxx>

#include <TopLoc_Location.hxx>
#include <gp_Trsf.hxx>
#include <Precision.hxx>

#include <TCollection_AsciiString.hxx>
#include <cmath>

namespace
{

bool isFiniteValue(const double theValue)
{
  return !std::isnan(theValue) && !Precision::IsInfinite(theValue);
}

occtl_status_t validateTransform(const occtl_transform_t& theTransform, const char* const theLabel)
{
  for (double aValue : theTransform.m)
  {
    if (!isFiniteValue(aValue))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                      + ": transform contains non-finite values"));
      return OCCTL_INVALID_ARGUMENT;
    }
  }
  return OCCTL_OK;
}

occtl_status_t findUniqueOccurrenceRef(const occtl_graph_t* const   theGraph,
                                       const BRepGraph_OccurrenceId theOccurrenceId,
                                       BRepGraph_OccurrenceRefId&   theOutRefId)
{
  bool aFound = false;
  for (BRepGraph_OccurrenceRefIterator anIt(theGraph->graph); anIt.More(); anIt.Next())
  {
    const BRepGraphInc::OccurrenceRef& aRef = anIt.Current();
    if (aRef.OccurrenceDefId != theOccurrenceId)
    {
      continue;
    }

    if (aFound)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "theOccurrence has more than one active occurrence reference");
      return OCCTL_INVALID_ARGUMENT;
    }

    theOutRefId = anIt.CurrentId();
    aFound      = true;
  }

  if (!aFound)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                           "theOccurrence has no active occurrence reference");
    return OCCTL_NOT_FOUND;
  }

  return OCCTL_OK;
}

occtl_status_t linkProductToTopologyImpl(occtl_graph_t* const    theGraph,
                                         const occtl_node_id_t   theProduct,
                                         const occtl_node_id_t   theRoot,
                                         const occtl_transform_t thePlacement,
                                         occtl_node_id_t* const  theOutOccurrence)
{
  if (theGraph == nullptr || theOutOccurrence == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           theGraph == nullptr ? "theGraph is NULL"
                                                               : "theOutOccurrence is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  *theOutOccurrence = OCCTL_NODE_ID_INVALID;

  BRepGraph_ProductId aProductId;
  if (const occtl_status_t aStatus =
        OcctL::Topo::ToTypedId(theGraph, theProduct, BRepGraph_NodeId::Kind::Product, aProductId))
  {
    return aStatus;
  }
  if (const occtl_status_t aStatus = validateTransform(thePlacement, "thePlacement"))
  {
    return aStatus;
  }

  const BRepGraph_NodeId aRootId = OcctL::Topo::UnpackNodeId(theRoot);
  if (!aRootId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theRoot is invalid or removed");
    return OCCTL_NOT_FOUND;
  }

  const TopLoc_Location aLoc(OcctL::Geom::ToGpTrsf(thePlacement));

  const BRepGraph_ProductId aTempId =
    theGraph->graph.Editor().Products().LinkProductToTopology(aRootId, aLoc);

  if (!aTempId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR, "LinkProductToTopology failed");
    return OCCTL_ERROR;
  }

  const BRepGraphInc::ProductDef& aTempDef = theGraph->graph.Topo().Products().Definition(aTempId);
  if (aTempDef.OccurrenceRefIds.Length() == 0)
  {
    theGraph->graph.Editor().Gen().RemoveNode(BRepGraph_NodeId(aTempId));
    OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR,
                                           "LinkProductToTopology created no occurrence");
    return OCCTL_ERROR;
  }

  const BRepGraph_OccurrenceRefId    aFirstRefId = aTempDef.OccurrenceRefIds.Value(0);
  const BRepGraphInc::OccurrenceRef& aFirstRef =
    theGraph->graph.Refs().Occurrences().Entry(aFirstRefId);
  const BRepGraph_OccurrenceId anOccurrenceId = aFirstRef.OccurrenceDefId;
  if (!anOccurrenceId.IsValid())
  {
    theGraph->graph.Editor().Gen().RemoveNode(BRepGraph_NodeId(aTempId));
    OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR,
                                           "LinkProductToTopology created invalid occurrence");
    return OCCTL_ERROR;
  }

  BRepGraph_MutGuard<BRepGraphInc::ProductDef> aMut =
    theGraph->graph.Editor().Products().Mut(aProductId);
  BRepGraphInc::ProductDef& aDef = aMut.Internal();

  for (int anI = 0; anI < aTempDef.OccurrenceRefIds.Length(); ++anI)
  {
    const BRepGraph_OccurrenceRefId aRefId = aTempDef.OccurrenceRefIds.Value(anI);
    aDef.OccurrenceRefIds.Append(aRefId);
  }

  const_cast<BRepGraphInc::ProductDef&>(aTempDef).OccurrenceRefIds.Clear();
  theGraph->graph.Editor().Gen().RemoveNode(BRepGraph_NodeId(aTempId));

  *theOutOccurrence = OcctL::Topo::PackNodeId(anOccurrenceId);
  return OCCTL_OK;
}

occtl_status_t linkProductsImpl(occtl_graph_t* const    theGraph,
                                const occtl_node_id_t   theParentProduct,
                                const occtl_node_id_t   theChildProduct,
                                const occtl_transform_t thePlacement,
                                const occtl_node_id_t   theParentOccurrence,
                                occtl_node_id_t* const  theOutOccurrence)
{
  if (theGraph == nullptr || theOutOccurrence == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           theGraph == nullptr ? "theGraph is NULL"
                                                               : "theOutOccurrence is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  *theOutOccurrence = OCCTL_NODE_ID_INVALID;

  BRepGraph_ProductId aParentId;
  if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                            theParentProduct,
                                                            BRepGraph_NodeId::Kind::Product,
                                                            aParentId))
  {
    return aStatus;
  }

  BRepGraph_ProductId aChildId;
  if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                            theChildProduct,
                                                            BRepGraph_NodeId::Kind::Product,
                                                            aChildId))
  {
    return aStatus;
  }
  if (const occtl_status_t aStatus = validateTransform(thePlacement, "thePlacement"))
  {
    return aStatus;
  }

  const TopLoc_Location  aLoc(OcctL::Geom::ToGpTrsf(thePlacement));
  const BRepGraph_NodeId aParentOccId = OcctL::Topo::UnpackNodeId(theParentOccurrence);

  const BRepGraph_OccurrenceId anOccId = theGraph->graph.Editor().Products().LinkProducts(
    aParentId,
    aChildId,
    aLoc,
    aParentOccId.IsValid() ? BRepGraph_OccurrenceId(aParentOccId) : BRepGraph_OccurrenceId());

  if (!anOccId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR, "LinkProducts failed");
    return OCCTL_ERROR;
  }

  *theOutOccurrence = OcctL::Topo::PackNodeId(anOccId);
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_make_product_info_init(occtl_topo_make_product_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_product(occtl_graph_t* const                        theGraph,
                          const occtl_topo_make_product_info_t* const theInfo,
                          occtl_node_id_t* const                      theOutProduct)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutProduct == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph, theInfo, or theOutProduct is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->struct_version != OCCTL_TOPO_MAKE_PRODUCT_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_TOPO_MAKE_PRODUCT_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = validateTransform(theInfo->placement, "info->placement"))
    {
      return aStatus;
    }

    BRepGraph_ProductId aProductId;

    const BRepGraph_NodeId aRootId = OcctL::Topo::UnpackNodeId(theInfo->root);
    if (theInfo->root.bits != 0u && !aRootId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "info->root is invalid or removed");
      return OCCTL_NOT_FOUND;
    }
    if (aRootId.IsValid())
    {
      const TopLoc_Location aLoc(OcctL::Geom::ToGpTrsf(theInfo->placement));

      aProductId = theGraph->graph.Editor().Products().LinkProductToTopology(aRootId, aLoc);
    }
    else
    {
      aProductId = theGraph->graph.Editor().Products().CreateEmptyProduct();
    }

    if (!aProductId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR, "make_product failed");
      return OCCTL_ERROR;
    }

    *theOutProduct = OcctL::Topo::PackNodeId(aProductId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_link_product(occtl_graph_t* const    theGraph,
                                                            const occtl_node_id_t   theProduct,
                                                            const occtl_node_id_t   theRoot,
                                                            const occtl_transform_t thePlacement)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    occtl_node_id_t anOccurrence = OCCTL_NODE_ID_INVALID;
    return linkProductToTopologyImpl(theGraph, theProduct, theRoot, thePlacement, &anOccurrence);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_link_product_occurrence(occtl_graph_t* const    theGraph,
                                     const occtl_node_id_t   theProduct,
                                     const occtl_node_id_t   theRoot,
                                     const occtl_transform_t thePlacement,
                                     occtl_node_id_t* const  theOutOccurrence)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return linkProductToTopologyImpl(theGraph, theProduct, theRoot, thePlacement, theOutOccurrence);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_link_products(occtl_graph_t* const    theGraph,
                           const occtl_node_id_t   theParentProduct,
                           const occtl_node_id_t   theChildProduct,
                           const occtl_transform_t thePlacement,
                           const occtl_node_id_t   theParentOccurrence)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    occtl_node_id_t anOccurrence = OCCTL_NODE_ID_INVALID;
    return linkProductsImpl(theGraph,
                            theParentProduct,
                            theChildProduct,
                            thePlacement,
                            theParentOccurrence,
                            &anOccurrence);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_link_products_occurrence(occtl_graph_t* const    theGraph,
                                      const occtl_node_id_t   theParentProduct,
                                      const occtl_node_id_t   theChildProduct,
                                      const occtl_transform_t thePlacement,
                                      const occtl_node_id_t   theParentOccurrence,
                                      occtl_node_id_t* const  theOutOccurrence)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return linkProductsImpl(theGraph,
                            theParentProduct,
                            theChildProduct,
                            thePlacement,
                            theParentOccurrence,
                            theOutOccurrence);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_remove_occurrence(occtl_graph_t* const theGraph, const occtl_ref_id_t theOccurrenceRef)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RefId aRefId = OcctL::Topo::UnpackRefId(theOccurrenceRef);
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "theOccurrenceRef is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    if (aRefId.RefKind != BRepGraph_RefId::Kind::Occurrence)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "theOccurrenceRef is not an occurrence ref");
      return OCCTL_WRONG_KIND;
    }

    const bool anOk = theGraph->graph.Editor().Gen().RemoveRef(aRefId);
    if (!anOk)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "theOccurrenceRef is already removed");
      return OCCTL_NOT_FOUND;
    }

    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_occurrence_set_transform(occtl_graph_t* const    theGraph,
                                      const occtl_node_id_t   theOccurrence,
                                      const occtl_transform_t theTransform)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_OccurrenceId anOccurrenceId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theOccurrence,
                                                              BRepGraph_NodeId::Kind::Occurrence,
                                                              anOccurrenceId))
    {
      return aStatus;
    }

    BRepGraph_OccurrenceRefId aRefId;
    if (const occtl_status_t aStatus = findUniqueOccurrenceRef(theGraph, anOccurrenceId, aRefId))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus = validateTransform(theTransform, "theTransform"))
    {
      return aStatus;
    }

    const TopLoc_Location aLoc(OcctL::Geom::ToGpTrsf(theTransform));
    theGraph->graph.Editor().Occurrences().SetRefLocalLocation(aRefId, aLoc);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_occurrence_transform(const occtl_graph_t* const theGraph,
                                  const occtl_node_id_t      theOccurrence,
                                  occtl_transform_t* const   theOutTransform)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutTransform == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "theOutTransform is NULL"
                                                      : "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_OccurrenceId anOccurrenceId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theOccurrence,
                                                              BRepGraph_NodeId::Kind::Occurrence,
                                                              anOccurrenceId))
    {
      return aStatus;
    }

    BRepGraph_OccurrenceRefId aRefId;
    if (const occtl_status_t aStatus = findUniqueOccurrenceRef(theGraph, anOccurrenceId, aRefId))
    {
      return aStatus;
    }

    *theOutTransform =
      OcctL::Geom::FromGp(theGraph->graph.Refs().LocalLocation(aRefId).Transformation());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_occurrence_world_transform(const occtl_graph_t* const theGraph,
                                        const occtl_node_id_t      theRoot,
                                        const occtl_node_id_t      theOccurrence,
                                        occtl_transform_t* const   theOutTransform)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutTransform == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "theOutTransform is NULL"
                                                      : "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aRootId = OcctL::Topo::UnpackNodeId(theRoot);
    if (!aRootId.IsValid() || theGraph->graph.Topo().Gen().IsRemoved(aRootId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theRoot is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    BRepGraph_OccurrenceId anOccurrenceId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theOccurrence,
                                                              BRepGraph_NodeId::Kind::Occurrence,
                                                              anOccurrenceId))
    {
      return aStatus;
    }

    BRepGraph_ChildExplorer::Config aConfig;
    aConfig.TargetKind            = BRepGraph_NodeId::Kind::Occurrence;
    aConfig.AccumulateLocation    = true;
    aConfig.AccumulateOrientation = false;

    for (BRepGraph_ChildExplorer anIt(theGraph->graph, aRootId, aConfig); anIt.More(); anIt.Next())
    {
      const BRepGraphInc::NodeInstance anInst = anIt.Current();
      if (anInst.DefId == BRepGraph_NodeId(anOccurrenceId))
      {
        *theOutTransform = OcctL::Geom::FromGp(anInst.Location.Transformation());
        return OCCTL_OK;
      }
    }

    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                           "theOccurrence is not reachable from theRoot");
    return OCCTL_NOT_FOUND;
  });
}

} // extern "C"
