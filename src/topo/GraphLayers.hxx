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

#ifndef OCCTL_TOPO_GRAPH_LAYERS_HXX
#define OCCTL_TOPO_GRAPH_LAYERS_HXX

#include <BRepGraph_NodeId.hxx>
#include <TCollection_AsciiString.hxx>
#include <Quantity_ColorRGBA.hxx>

class BRepGraph;

namespace OcctL::Topo
{

void EnsureBuiltinLayers(BRepGraph& theGraph);
void CopyBuiltinLayers(const BRepGraph& theSource, BRepGraph& theTarget);
bool FindBuiltinColor(const BRepGraph&    theGraph,
                      BRepGraph_NodeId    theNode,
                      Quantity_ColorRGBA& theColor);
bool FindBuiltinName(const BRepGraph&         theGraph,
                     BRepGraph_NodeId         theNode,
                     TCollection_AsciiString& theName);
bool FindBuiltinMetadata(const BRepGraph&               theGraph,
                         BRepGraph_NodeId               theNode,
                         const TCollection_AsciiString& theKey,
                         TCollection_AsciiString&       theValue);
bool HasBuiltinTag(const BRepGraph&               theGraph,
                   BRepGraph_NodeId               theNode,
                   const TCollection_AsciiString& theTag);

} // namespace OcctL::Topo

#endif // OCCTL_TOPO_GRAPH_LAYERS_HXX
