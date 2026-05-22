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

#include "GraphHandle.hxx"
#include "IdConvert.hxx"
#include "TopoMath.hxx"

#include <BRepGraph.hxx>
#include <BRepGraphInc_Instance.hxx>
#include <BRepGraph_ChildExplorer.hxx>
#include <BRepGraph_ParentExplorer.hxx>
#include <BRepGraph_TopoView.hxx>

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include "../geom/GeomMath.hxx"

#include <TopAbs_Orientation.hxx>
#include <TopLoc_Location.hxx>
#include <gp_Trsf.hxx>

#include <TCollection_AsciiString.hxx>
#include <type_traits>
#include <variant>

struct occtl_topo_explorer_iter
{
  std::variant<std::monostate, BRepGraph_ChildExplorer, BRepGraph_ParentExplorer> impl;
};

namespace
{

occtl_status_t validateTraversalMode(const occtl_topo_explorer_traversal_t theMode)
{
  if (theMode != OCCTL_TOPO_EXPLORER_RECURSIVE && theMode != OCCTL_TOPO_EXPLORER_DIRECT_CHILDREN)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE, "explorer mode is not valid");
    return OCCTL_OUT_OF_RANGE;
  }
  return OCCTL_OK;
}

occtl_status_t parseKindFilter(const occtl_node_kind_t                theAbiKind,
                               const char* const                      theFieldName,
                               std::optional<BRepGraph_NodeId::Kind>& theOutKind)
{
  if (theAbiKind == OCCTL_KIND_INVALID)
  {
    theOutKind.reset();
    return OCCTL_OK;
  }

  BRepGraph_NodeId::Kind anOcctKind;
  if (!OcctL::Topo::TryToOcctNodeKind(theAbiKind, anOcctKind))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_OUT_OF_RANGE,
      static_cast<std::string_view>(TCollection_AsciiString(theFieldName) + " is not valid"));
    return OCCTL_OUT_OF_RANGE;
  }

  theOutKind = anOcctKind;
  return OCCTL_OK;
}

//! Converts and validates BRepGraph_ChildExplorer::Config from ABI config.
occtl_status_t ConfigFromABI(const occtl_topo_child_explorer_config_t* theABI,
                             BRepGraph_ChildExplorer::Config&          theConfig)
{
  if (theABI->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "config->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (const occtl_status_t aModeStatus = validateTraversalMode(theABI->mode))
  {
    return aModeStatus;
  }

  const BRepGraph_ChildExplorer::TraversalMode aMode =
    (theABI->mode == OCCTL_TOPO_EXPLORER_DIRECT_CHILDREN)
      ? BRepGraph_ChildExplorer::TraversalMode::DirectChildren
      : BRepGraph_ChildExplorer::TraversalMode::Recursive;

  std::optional<BRepGraph_NodeId::Kind> aTargetKind;
  if (const occtl_status_t aTargetStatus =
        parseKindFilter(theABI->target_kind, "config->target_kind", aTargetKind))
  {
    return aTargetStatus;
  }

  std::optional<BRepGraph_NodeId::Kind> aAvoidKind;
  if (const occtl_status_t aAvoidStatus =
        parseKindFilter(theABI->avoid_kind, "config->avoid_kind", aAvoidKind))
  {
    return aAvoidStatus;
  }

  theConfig = {
    aMode,
    aTargetKind,
    aAvoidKind,
    theABI->emit_avoid_kind != 0,
    theABI->accumulate_location != 0,
    theABI->accumulate_orientation != 0,
    TopLoc_Location(), // StartLoc — identity
    TopAbs_FORWARD     // StartOri — forward
  };

  return OCCTL_OK;
}

//! Converts and validates BRepGraph_ParentExplorer::Config from ABI config.
occtl_status_t ConfigFromABI(const occtl_topo_parent_explorer_config_t* theABI,
                             BRepGraph_ParentExplorer::Config&          theConfig)
{
  if (theABI->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "config->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (const occtl_status_t aModeStatus = validateTraversalMode(theABI->mode))
  {
    return aModeStatus;
  }

  const BRepGraph_ParentExplorer::TraversalMode aMode =
    (theABI->mode == OCCTL_TOPO_EXPLORER_DIRECT_CHILDREN)
      ? BRepGraph_ParentExplorer::TraversalMode::DirectParents
      : BRepGraph_ParentExplorer::TraversalMode::Recursive;

  std::optional<BRepGraph_NodeId::Kind> aTargetKind;
  if (const occtl_status_t aTargetStatus =
        parseKindFilter(theABI->target_kind, "config->target_kind", aTargetKind))
  {
    return aTargetStatus;
  }

  std::optional<BRepGraph_NodeId::Kind> aAvoidKind;
  if (const occtl_status_t aAvoidStatus =
        parseKindFilter(theABI->avoid_kind, "config->avoid_kind", aAvoidKind))
  {
    return aAvoidStatus;
  }

  theConfig = {aMode, aTargetKind, aAvoidKind, theABI->emit_avoid_kind != 0};

  return OCCTL_OK;
}

} // anonymous namespace

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_child_explorer_create(const occtl_graph_t* const                      theGraph,
                                   const occtl_node_id_t                           theRoot,
                                   const occtl_topo_child_explorer_config_t* const theConfigABI,
                                   occtl_topo_explorer_iter_t** const              theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aRootId = OcctL::Topo::UnpackNodeId(theRoot);
    if (!aRootId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "root NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    BRepGraph_ChildExplorer::Config aConfig{};
    if (theConfigABI != nullptr)
    {
      if (theConfigABI->struct_version != OCCTL_TOPO_CHILD_EXPLORER_CONFIG_VERSION_1)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                               "unsupported child explorer config version");
        return OCCTL_VERSION_MISMATCH;
      }
      if (const occtl_status_t aCfgStatus = ConfigFromABI(theConfigABI, aConfig))
      {
        return aCfgStatus;
      }
    }

    occtl_topo_explorer_iter* anIter = new occtl_topo_explorer_iter;
    anIter->impl.template emplace<BRepGraph_ChildExplorer>(theGraph->graph, aRootId, aConfig);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_parent_explorer_create(const occtl_graph_t* const                       theGraph,
                                    const occtl_node_id_t                            theNode,
                                    const occtl_topo_parent_explorer_config_t* const theConfigABI,
                                    occtl_topo_explorer_iter_t** const               theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theNode);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    BRepGraph_ParentExplorer::Config aConfig{};
    if (theConfigABI != nullptr)
    {
      if (theConfigABI->struct_version != OCCTL_TOPO_PARENT_EXPLORER_CONFIG_VERSION_1)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                               "unsupported parent explorer config version");
        return OCCTL_VERSION_MISMATCH;
      }
      if (const occtl_status_t aCfgStatus = ConfigFromABI(theConfigABI, aConfig))
      {
        return aCfgStatus;
      }
    }

    occtl_topo_explorer_iter* anIter = new occtl_topo_explorer_iter;
    anIter->impl.template emplace<BRepGraph_ParentExplorer>(theGraph->graph, aNodeId, aConfig);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_explorer_iter_next(occtl_topo_explorer_iter_t* const theIter,
                                occtl_node_id_t* const            theOutNode,
                                occtl_transform_t* const          theOutTransform,
                                occtl_orientation_t* const        theOutOrientation)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theIter == nullptr || theOutNode == nullptr || theOutTransform == nullptr
        || theOutOrientation == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theIter ? "an out-param is NULL" : "iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    return std::visit(
      [&](auto& aImpl) -> occtl_status_t {
        if constexpr (std::is_same_v<std::decay_t<decltype(aImpl)>, std::monostate>)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INTERNAL,
                                                 "explorer iterator is uninitialised (monostate)");
          return OCCTL_INTERNAL;
        }
        else
        {
          if (!aImpl.More())
          {
            *theOutNode        = OCCTL_NODE_ID_INVALID;
            *theOutTransform   = OcctL::Geom::FromGp(gp_Trsf()); // identity
            *theOutOrientation = OCCTL_ORIENTATION_FORWARD;
            return OCCTL_NOT_FOUND;
          }

          const BRepGraphInc::NodeInstance aInst = aImpl.Current();
          *theOutNode                            = OcctL::Topo::PackNodeId(aInst.DefId);
          *theOutTransform = OcctL::Geom::FromGp(aInst.Location.Transformation());
          *theOutOrientation =
            static_cast<occtl_orientation_t>(static_cast<int>(aInst.Orientation));
          aImpl.Next();
          return OCCTL_OK;
        }
      },
      theIter->impl);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_explorer_iter_free(occtl_topo_explorer_iter_t* const theIter)
{
  delete theIter;
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_child_explorer_config_init(occtl_topo_child_explorer_config_t* const theConfig)
{
  if (theConfig != nullptr)
  {
    *theConfig = OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_parent_explorer_config_init(occtl_topo_parent_explorer_config_t* const theConfig)
{
  if (theConfig != nullptr)
  {
    *theConfig = OCCTL_TOPO_PARENT_EXPLORER_CONFIG_INIT;
  }
}

} // extern "C"
