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

//! @file split.cxx
//! @brief Boolean Split: partition each object using the tools as cutters.
//!
//! BRepAlgoAPI_Splitter inherits from BRepAlgoAPI_BuilderAlgo directly and
//! does not have a "type of operation" setter.  The generic Preflight
//! + RunAndCommit pattern still drives it: SetArguments are the splittees,
//! SetTools are the cutters, the result root is a Compound of partitions.

#include "BoolMath.hxx"

#include "../core/Guard.hxx"

#include <occtl/occtl_bool.h>

#include <BRepAlgoAPI_Splitter.hxx>

extern "C"
{

OCCTL_API occtl_status_t OCCTL_CALL occtl_bool_split(occtl_graph_t* const              graph,
                                                     const occtl_node_id_t* const      objects,
                                                     const size_t                      n_objects,
                                                     const occtl_node_id_t* const      tools,
                                                     const size_t                      n_tools,
                                                     const occtl_bool_options_t* const opts,
                                                     occtl_node_id_t* const            out_root)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    NCollection_List<TopoDS_Shape>                                               anObjList;
    NCollection_List<TopoDS_Shape>                                               aToolList;
    NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anInputsMap;
    BRepAlgoAPI_Splitter                                                         anAlgo;

    if (const occtl_status_t aStatus = OcctL::Bool::Preflight(graph,
                                                              objects,
                                                              n_objects,
                                                              tools,
                                                              n_tools,
                                                              opts,
                                                              out_root,
                                                              anObjList,
                                                              aToolList,
                                                              anInputsMap,
                                                              anAlgo))
    {
      return aStatus;
    }

    return OcctL::Bool::RunAndCommit(graph, opts, out_root, "Split", anInputsMap, anAlgo);
  });
}

} // extern "C"
