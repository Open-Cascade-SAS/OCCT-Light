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

#include <BRepGraphInc_Definition.hxx>
#include <BRepGraph_DefsIterator.hxx>
#include <BRepGraph_Iterator.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_TopoView.hxx>
#include <BRepGraph_WireExplorer.hxx>
#include <NCollection_LinearVector.hxx>
#include <TopAbs_Orientation.hxx>

#include <occtl/occtl_topo.h>

#include <type_traits>
#include <variant>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

struct occtl_node_iter
{
  std::variant<std::monostate,
               BRepGraph_SolidIterator,
               BRepGraph_ShellIterator,
               BRepGraph_FaceIterator,
               BRepGraph_WireIterator,
               BRepGraph_EdgeIterator,
               BRepGraph_VertexIterator,
               BRepGraph_CompoundIterator,
               BRepGraph_CompSolidIterator,
               BRepGraph_CoEdgeIterator,
               BRepGraph_ProductIterator,
               BRepGraph_OccurrenceIterator,
               BRepGraph_RootProductIterator,
               BRepGraph_DefsShellOfSolid,
               BRepGraph_DefsFaceOfShell,
               BRepGraph_DefsWireOfFace,
               BRepGraph_DefsCoEdgeOfWire,
               BRepGraph_DefsEdgeOfWire,
               BRepGraph_DefsVertexOfEdge,
               BRepGraph_DefsOccurrenceOfProduct,
               BRepGraph_WireExplorer>
    impl;
};

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_node_iter_next(occtl_node_iter_t* const theIter,
                                                         occtl_node_id_t* const   theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theIter == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theIter ? "out_id is NULL" : "iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    return std::visit(
      [&](auto& aImpl) -> occtl_status_t {
        if constexpr (std::is_same_v<std::decay_t<decltype(aImpl)>, std::monostate>)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INTERNAL,
                                                 "iterator is uninitialised (monostate)");
          return OCCTL_INTERNAL;
        }
        else
        {
          if (!aImpl.More())
          {
            *theOutId = OCCTL_NODE_ID_INVALID;
            return OCCTL_NOT_FOUND;
          }
          if constexpr (std::is_same_v<std::decay_t<decltype(aImpl)>, BRepGraph_RootProductIterator>
                        || std::is_same_v<std::decay_t<decltype(aImpl)>, BRepGraph_WireExplorer>)
          {
            *theOutId = OcctL::Topo::PackNodeId(aImpl.Current());
          }
          else
          {
            *theOutId = OcctL::Topo::PackNodeId(aImpl.CurrentId());
          }
          aImpl.Next();
          return OCCTL_OK;
        }
      },
      theIter->impl);
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_node_iter_free(occtl_node_iter_t* const theIter)
{
  delete theIter;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_solid_iter_create(const occtl_graph_t* const theGraph,
                                occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_SolidIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_shell_iter_create(const occtl_graph_t* const theGraph,
                                occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_ShellIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_face_iter_create(const occtl_graph_t* const theGraph,
                               occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_FaceIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_wire_iter_create(const occtl_graph_t* const theGraph,
                               occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_WireIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_edge_iter_create(const occtl_graph_t* const theGraph,
                               occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_EdgeIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_vertex_iter_create(const occtl_graph_t* const theGraph,
                                 occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_VertexIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_compound_iter_create(const occtl_graph_t* const theGraph,
                                   occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_CompoundIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_compsolid_iter_create(const occtl_graph_t* const theGraph,
                                    occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_CompSolidIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_coedge_iter_create(const occtl_graph_t* const theGraph,
                                 occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_CoEdgeIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_product_iter_create(const occtl_graph_t* const theGraph,
                                  occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_ProductIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_occurrence_iter_create(const occtl_graph_t* const theGraph,
                                     occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_OccurrenceIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_root_product_iter_create(const occtl_graph_t* const theGraph,
                                       occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_RootProductIterator>(theGraph->graph);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_shells_of_solid_iter_create(const occtl_graph_t* const theGraph,
                                         const occtl_node_id_t      theSolid,
                                         occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_SolidId aSolidId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theSolid, BRepGraph_NodeId::Kind::Solid, aSolidId))
    {
      return aErr;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_DefsShellOfSolid>(theGraph->graph, aSolidId);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_faces_of_shell_iter_create(const occtl_graph_t* const theGraph,
                                        const occtl_node_id_t      theShell,
                                        occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_ShellId aShellId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theShell, BRepGraph_NodeId::Kind::Shell, aShellId))
    {
      return aErr;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_DefsFaceOfShell>(theGraph->graph, aShellId);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wires_of_face_iter_create(const occtl_graph_t* const theGraph,
                                       const occtl_node_id_t      theFace,
                                       occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aErr;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_DefsWireOfFace>(theGraph->graph, aFaceId);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedges_of_wire_iter_create(const occtl_graph_t* const theGraph,
                                         const occtl_node_id_t      theWire,
                                         occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_WireId aWireId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aErr;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_DefsCoEdgeOfWire>(theGraph->graph, aWireId);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edges_of_wire_iter_create(const occtl_graph_t* const theGraph,
                                       const occtl_node_id_t      theWire,
                                       occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_WireId aWireId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aErr;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_DefsEdgeOfWire>(theGraph->graph, aWireId);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_vertices_of_edge_iter_create(const occtl_graph_t* const theGraph,
                                          const occtl_node_id_t      theEdge,
                                          occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aErr;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_DefsVertexOfEdge>(theGraph->graph, anEdgeId);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_occurrences_of_product_iter_create(const occtl_graph_t* const theGraph,
                                                const occtl_node_id_t      theProduct,
                                                occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_ProductId aProductId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theProduct, BRepGraph_NodeId::Kind::Product, aProductId))
    {
      return aErr;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_DefsOccurrenceOfProduct>(theGraph->graph, aProductId);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_solid_shell_count(const occtl_graph_t* const theGraph,
                               const occtl_node_id_t      theSolid,
                               uint32_t* const            theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_SolidId aSolidId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theSolid, BRepGraph_NodeId::Kind::Solid, aSolidId))
    {
      return aErr;
    }
    *theOutCount = static_cast<uint32_t>(
      theGraph->graph.Topo().Solids().Definition(aSolidId).ShellRefIds.Size());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_edge_count(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t      theWire,
                                                               uint32_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_WireId aWireId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aErr;
    }
    *theOutCount = BRepGraph_Tool::Wire::NbDistinctEdges(theGraph->graph, aWireId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_vertex_count(const occtl_graph_t* const theGraph,
                               const occtl_node_id_t      theEdge,
                               uint32_t* const            theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aErr;
    }
    const BRepGraphInc::EdgeDef& aDef = theGraph->graph.Topo().Edges().Definition(anEdgeId);
    *theOutCount = 2u + static_cast<uint32_t>(aDef.InternalVertexRefIds.Size());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_product_occurrence_count(const occtl_graph_t* const theGraph,
                                      const occtl_node_id_t      theProduct,
                                      uint32_t* const            theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_ProductId aProductId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theProduct, BRepGraph_NodeId::Kind::Product, aProductId))
    {
      return aErr;
    }
    *theOutCount = static_cast<uint32_t>(
      theGraph->graph.Topo().Products().Definition(aProductId).OccurrenceRefIds.Size());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wire_explorer_create(const occtl_graph_t* const theGraph,
                                  const occtl_node_id_t      theWire,
                                  occtl_node_iter_t** const  theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_iter is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_WireId aWireId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aErr;
    }
    occtl_node_iter* anIter = new occtl_node_iter;
    anIter->impl.template emplace<BRepGraph_WireExplorer>(theGraph->graph, aWireId);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wire_order_edges(const occtl_graph_t* const   theGraph,
                              const occtl_node_id_t        theWire,
                              occtl_oriented_node_t* const theOutBuf,
                              const size_t                 theCap,
                              size_t* const                theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aErr;
    }

    NCollection_LinearVector<occtl_oriented_node_t> anEdges;
    BRepGraph_WireExplorer                          anExplorer(theGraph->graph, aWireId);
    anEdges.Reserve(static_cast<size_t>(anExplorer.NbEdges()));
    for (; anExplorer.More(); anExplorer.Next())
    {
      const BRepGraph_CoEdgeId aCoEdgeId = anExplorer.CurrentCoEdgeId();
      const BRepGraph_EdgeId anEdgeId = BRepGraph_Tool::CoEdge::EdgeOf(theGraph->graph, aCoEdgeId);
      const TopAbs_Orientation anOrientation =
        BRepGraph_Tool::CoEdge::Orientation(theGraph->graph, aCoEdgeId);
      anEdges.Append(
        {OcctL::Topo::PackNodeId(anEdgeId), OcctL::Topo::FromOcctOrientation(anOrientation)});
    }

    *theOutCount = anEdges.Size();
    if (theOutBuf != nullptr && theCap < anEdges.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "out_buf capacity is too small");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    if (theOutBuf != nullptr)
    {
      for (size_t anI = 0; anI < anEdges.Size(); ++anI)
      {
        theOutBuf[anI] = anEdges.Value(anI);
      }
    }
    return OCCTL_OK;
  });
}

} // extern "C"
