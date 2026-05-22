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

#ifndef OCCTL_TOPO_GRAPH_DESCENDANT_CACHE_HXX
#define OCCTL_TOPO_GRAPH_DESCENDANT_CACHE_HXX

#include <BRepGraph.hxx>
#include <BRepGraph_NodeId.hxx>

#include <occtl/occtl_topo.h>

#include <NCollection_LinearVector.hxx>

namespace OcctL::Topo
{

bool ComputeDescendantVertices(BRepGraph&                                 theGraph,
                               BRepGraph_NodeId                           theRoot,
                               NCollection_LinearVector<occtl_node_id_t>& theOutVertices);

bool ComputeDescendantEdges(BRepGraph&                                 theGraph,
                            BRepGraph_NodeId                           theRoot,
                            NCollection_LinearVector<occtl_node_id_t>& theOutEdges);

bool ComputeDescendantFaces(BRepGraph&                                 theGraph,
                            BRepGraph_NodeId                           theRoot,
                            NCollection_LinearVector<occtl_node_id_t>& theOutFaces);

bool ComputeDescendantsByKind(BRepGraph&                                 theGraph,
                              BRepGraph_NodeId                           theRoot,
                              occtl_node_kind_t                          theKind,
                              NCollection_LinearVector<occtl_node_id_t>& theOutNodes);

} // namespace OcctL::Topo

#endif // OCCTL_TOPO_GRAPH_DESCENDANT_CACHE_HXX
