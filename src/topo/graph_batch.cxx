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

#include <BRepGraph_EditorView.hxx>

extern "C"
{

//==================================================================================================

struct occtl_batch
{
  occtl_graph_t* graph;
};

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_begin_batch(occtl_graph_t* const  theGraph,
                                                            occtl_batch_t** const theOutBatch)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutBatch == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_batch is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_batch_t* aBatch = new occtl_batch_t;
    aBatch->graph         = theGraph;
    *theOutBatch          = aBatch;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_batch_commit(occtl_batch_t* const theBatch)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theBatch == nullptr)
    {
      return OCCTL_OK;
    }
    delete theBatch;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_batch_abort(occtl_batch_t* const theBatch)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theBatch == nullptr)
    {
      return OCCTL_OK;
    }
    delete theBatch;
    return OCCTL_OK;
  });
}

} // extern "C"
