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

#include <occtl/occtl_viz.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/GeomMath.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <AIS_BRepGraph.hxx>
#include <AIS_BRepGraphOwner.hxx>
#include <AIS_InteractiveContext.hxx>
#include <BRepGraph_UIDsView.hxx>
#include <Graphic3d_Camera.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <Standard_ErrorHandler.hxx>
#include <TCollection_AsciiString.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <Image_AlienPixMap.hxx>
#include <Image_Format.hxx>
#include <Image_PixMap.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_ColorRGBA.hxx>

#include <algorithm>
#include <memory>

struct occtl_viz_driver
{
  occ::handle<Aspect_DisplayConnection> display;
  occ::handle<OpenGl_GraphicDriver>     driver;
};

struct occtl_viz_viewer
{
  occtl_viz_driver*                   owner = nullptr;
  occ::handle<V3d_Viewer>             viewer;
  occ::handle<AIS_InteractiveContext> context;
};

struct occtl_viz_view
{
  occtl_viz_viewer*                 owner = nullptr;
  occ::handle<V3d_View>             view;
  occ::handle<Aspect_NeutralWindow> window;
  int                               width  = 0;
  int                               height = 0;
};

struct occtl_viz_presentable
{
  occtl_viz_viewer*          owner = nullptr;
  occtl_graph_t*             graph = nullptr;
  occ::handle<AIS_BRepGraph> ais;
};

namespace
{

bool IsDisplayModeValid(const occtl_viz_display_mode_t theMode)
{
  return theMode == OCCTL_VIZ_DISPLAY_WIREFRAME || theMode == OCCTL_VIZ_DISPLAY_SHADED
         || theMode == OCCTL_VIZ_DISPLAY_SHADED_WITH_EDGES
         || theMode == OCCTL_VIZ_DISPLAY_MESH_DEBUG;
}

bool IsSelectionModeValid(const occtl_viz_selection_mode_t theMode)
{
  return theMode >= OCCTL_VIZ_SELECT_WHOLE && theMode <= OCCTL_VIZ_SELECT_COMPSOLID;
}

bool IsZeroOrOne(const int32_t theValue)
{
  return theValue == 0 || theValue == 1;
}

V3d_TypeOfOrientation ToOcctStandardView(const occtl_viz_standard_view_t theView,
                                         occtl_status_t&                 theStatus)
{
  theStatus = OCCTL_OK;
  switch (theView)
  {
    case OCCTL_VIZ_VIEW_FRONT:
      return V3d_Yneg;
    case OCCTL_VIZ_VIEW_BACK:
      return V3d_Ypos;
    case OCCTL_VIZ_VIEW_LEFT:
      return V3d_Xneg;
    case OCCTL_VIZ_VIEW_RIGHT:
      return V3d_Xpos;
    case OCCTL_VIZ_VIEW_TOP:
      return V3d_Zpos;
    case OCCTL_VIZ_VIEW_BOTTOM:
      return V3d_Zneg;
    case OCCTL_VIZ_VIEW_ISO:
      return V3d_XposYnegZpos;
    default:
      theStatus = OCCTL_OUT_OF_RANGE;
      return V3d_XposYnegZpos;
  }
}

Quantity_Color ToOcctColor(const occtl_color_rgba_t theColor)
{
  return Quantity_Color(static_cast<double>(theColor.r),
                        static_cast<double>(theColor.g),
                        static_cast<double>(theColor.b),
                        Quantity_TOC_RGB);
}

occtl_transform_t ToAbiTransform(const TopLoc_Location& theLocation)
{
  return OcctL::Geom::FromGp(theLocation.Transformation());
}

occtl_viz_selection_mode_t SelectionModeFromNode(const BRepGraph_NodeId& theNode)
{
  switch (theNode.NodeKind)
  {
    case BRepGraph_NodeId::Kind::Face:
      return OCCTL_VIZ_SELECT_FACE;
    case BRepGraph_NodeId::Kind::Edge:
      return OCCTL_VIZ_SELECT_EDGE;
    case BRepGraph_NodeId::Kind::Vertex:
      return OCCTL_VIZ_SELECT_VERTEX;
    case BRepGraph_NodeId::Kind::Wire:
      return OCCTL_VIZ_SELECT_WIRE;
    case BRepGraph_NodeId::Kind::Shell:
      return OCCTL_VIZ_SELECT_SHELL;
    case BRepGraph_NodeId::Kind::Solid:
      return OCCTL_VIZ_SELECT_SOLID;
    case BRepGraph_NodeId::Kind::CoEdge:
      return OCCTL_VIZ_SELECT_COEDGE;
    case BRepGraph_NodeId::Kind::Product:
      return OCCTL_VIZ_SELECT_PRODUCT;
    case BRepGraph_NodeId::Kind::Compound:
      return OCCTL_VIZ_SELECT_COMPOUND;
    case BRepGraph_NodeId::Kind::CompSolid:
      return OCCTL_VIZ_SELECT_COMPSOLID;
    default:
      return OCCTL_VIZ_SELECT_WHOLE;
  }
}

void InitPickResult(occtl_viz_pick_result_t* const thePick)
{
  if (thePick == nullptr)
  {
    return;
  }
  const occtl_viz_pick_result_t anInit = OCCTL_VIZ_PICK_RESULT_INIT;
  *thePick                             = anInit;
}

occtl_status_t FillPick(const occtl_viz_view_t* const             theView,
                        const occ::handle<SelectMgr_EntityOwner>& theOwner,
                        occtl_viz_pick_result_t* const            thePick)
{
  InitPickResult(thePick);
  if (theOwner.IsNull())
  {
    return OCCTL_NOT_FOUND;
  }
  const occ::handle<AIS_BRepGraphOwner> anOwner =
    occ::handle<AIS_BRepGraphOwner>::DownCast(theOwner);
  if (anOwner.IsNull())
  {
    return OCCTL_NOT_FOUND;
  }

  const BRepGraph_NodeId aNode = anOwner->Node();
  thePick->node                = OcctL::Topo::PackNodeId(aNode);
  thePick->ref                 = OcctL::Topo::PackRefId(anOwner->Reference());
  thePick->selection_mode      = SelectionModeFromNode(aNode);
  thePick->usage_transform     = ToAbiTransform(anOwner->UsageLocation());

  if (theView != nullptr && theView->owner != nullptr)
  {
    const occ::handle<AIS_BRepGraph> aPrs =
      occ::handle<AIS_BRepGraph>::DownCast(anOwner->Selectable());
    if (!aPrs.IsNull())
    {
      const BRepGraph* const aGraph = aPrs->Graph();
      if (aGraph != nullptr)
      {
        const BRepGraph_UID aUid = aGraph->UIDs().Of(aNode);
        thePick->uid             = OcctL::Topo::PackUID(aUid);
      }
    }
  }
  return OCCTL_OK;
}

} // namespace

extern "C"
{

OCCTL_API void OCCTL_CALL
  occtl_viz_driver_options_init(occtl_viz_driver_options_t* const theOptions)
{
  if (theOptions == nullptr)
  {
    return;
  }
  const occtl_viz_driver_options_t anInit = OCCTL_VIZ_DRIVER_OPTIONS_INIT;
  *theOptions                             = anInit;
}

OCCTL_API void OCCTL_CALL occtl_viz_view_options_init(occtl_viz_view_options_t* const theOptions)
{
  if (theOptions == nullptr)
  {
    return;
  }
  const occtl_viz_view_options_t anInit = OCCTL_VIZ_VIEW_OPTIONS_INIT;
  *theOptions                           = anInit;
}

OCCTL_API void OCCTL_CALL occtl_viz_pick_result_init(occtl_viz_pick_result_t* const thePick)
{
  InitPickResult(thePick);
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_driver_create(const occtl_viz_driver_options_t* const theOptions,
                          occtl_viz_driver_t** const              theOutDriver)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theOutDriver == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_driver_create: out_driver is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutDriver = nullptr;
    if (theOptions != nullptr && theOptions->struct_version != OCCTL_VIZ_DRIVER_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_viz_driver_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOptions != nullptr && theOptions->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_driver_options_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    std::unique_ptr<occtl_viz_driver_t> aDriver(new occtl_viz_driver_t{});
    aDriver->display = new Aspect_DisplayConnection();
    aDriver->driver  = new OpenGl_GraphicDriver(aDriver->display, false);
    if (theOptions != nullptr)
    {
      aDriver->driver->EnableVBO(theOptions->enable_vbo != 0);
      aDriver->driver->SetVerticalSync(theOptions->enable_vsync != 0);
    }
    *theOutDriver = aDriver.release();
    return OCCTL_OK;
  });
}

OCCTL_API void OCCTL_CALL occtl_viz_driver_free(occtl_viz_driver_t* const theDriver)
{
  delete theDriver;
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_viewer_create(occtl_viz_driver_t* const  theDriver,
                                                            occtl_viz_viewer_t** const theOutViewer)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theDriver == nullptr || theOutViewer == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_viewer_create: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutViewer = nullptr;
    std::unique_ptr<occtl_viz_viewer_t> aViewer(new occtl_viz_viewer_t{});
    aViewer->owner   = theDriver;
    aViewer->viewer  = new V3d_Viewer(theDriver->driver);
    aViewer->context = new AIS_InteractiveContext(aViewer->viewer);
    *theOutViewer    = aViewer.release();
    return OCCTL_OK;
  });
}

OCCTL_API void OCCTL_CALL occtl_viz_viewer_free(occtl_viz_viewer_t* const theViewer)
{
  delete theViewer;
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_create(occtl_viz_viewer_t* const             theViewer,
                        const occtl_viz_view_options_t* const theOptions,
                        occtl_viz_view_t** const              theOutView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theViewer == nullptr || theOutView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_create: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutView                                    = nullptr;
    const occtl_viz_view_options_t        aDefault = OCCTL_VIZ_VIEW_OPTIONS_INIT;
    const occtl_viz_view_options_t* const anOptions =
      theOptions != nullptr ? theOptions : &aDefault;
    if (anOptions->struct_version != OCCTL_VIZ_VIEW_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_viz_view_options_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (anOptions->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_options_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsZeroOrOne(anOptions->offscreen))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_options_t::offscreen must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (anOptions->width <= 0 || anOptions->height <= 0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_viz_view_create: width/height must be positive");
      return OCCTL_OUT_OF_RANGE;
    }
    if (anOptions->native_handle == nullptr && anOptions->offscreen == 0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_viz_view_create: native_handle is required when offscreen is 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    std::unique_ptr<occtl_viz_view_t> aView(new occtl_viz_view_t{});
    aView->owner  = theViewer;
    aView->width  = anOptions->width;
    aView->height = anOptions->height;
    aView->view   = theViewer->viewer->CreateView();
    aView->window = new Aspect_NeutralWindow();
    aView->window->SetSize(anOptions->width, anOptions->height);
    if (anOptions->native_handle != nullptr)
    {
      aView->window->SetNativeHandle(reinterpret_cast<Aspect_Drawable>(anOptions->native_handle));
    }
    if (anOptions->native_handle != nullptr || anOptions->offscreen != 0)
    {
      aView->view->SetWindow(aView->window);
      aView->view->MustBeResized();
    }
    *theOutView = aView.release();
    return OCCTL_OK;
  });
}

OCCTL_API void OCCTL_CALL occtl_viz_view_free(occtl_viz_view_t* const theView)
{
  delete theView;
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_create(occtl_viz_viewer_t* const       theViewer,
                               occtl_graph_t* const            theGraph,
                               const occtl_node_id_t           theRoot,
                               occtl_viz_presentable_t** const theOutPresentable)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theViewer == nullptr || theGraph == nullptr || theOutPresentable == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_presentable_create: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutPresentable           = nullptr;
    const BRepGraph_NodeId aRoot = OcctL::Topo::UnpackNodeId(theRoot);
    if (!aRoot.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "occtl_viz_presentable_create: root NodeId is invalid");
      return OCCTL_NOT_FOUND;
    }

    std::unique_ptr<occtl_viz_presentable_t> aPresentable(new occtl_viz_presentable_t{});
    aPresentable->owner = theViewer;
    aPresentable->graph = theGraph;
    aPresentable->ais   = new AIS_BRepGraph(theGraph->graph, aRoot);
    aPresentable->ais->SetDisplayMode(AIS_BRepGraph::DM_ShadedWithBRepEdges);
    *theOutPresentable = aPresentable.release();
    return OCCTL_OK;
  });
}

OCCTL_API void OCCTL_CALL occtl_viz_presentable_free(occtl_viz_presentable_t* const thePresentable)
{
  delete thePresentable;
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_set_display_mode(occtl_viz_presentable_t* const thePresentable,
                                         const occtl_viz_display_mode_t theMode)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (thePresentable == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_viz_presentable_set_display_mode: presentable is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsDisplayModeValid(theMode))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_viz_presentable_set_display_mode: invalid mode");
      return OCCTL_OUT_OF_RANGE;
    }
    thePresentable->ais->SetDisplayMode(static_cast<int>(theMode));
    thePresentable->ais->SetToUpdate(static_cast<int>(theMode));
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_synchronize(occtl_viz_presentable_t* const thePresentable,
                                    int32_t* const                 theOutChanged)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (thePresentable == nullptr || theOutChanged == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_presentable_synchronize: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutChanged = thePresentable->ais->SynchronizeCache() ? 1 : 0;
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_invalidate(occtl_viz_presentable_t* const thePresentable)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (thePresentable == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_viz_presentable_invalidate: presentable is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    thePresentable->ais->InvalidateCache();
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_set_visible_nodes(occtl_viz_presentable_t* const thePresentable,
                                          const occtl_node_id_t* const   theNodes,
                                          const size_t                   theNbNodes)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (thePresentable == nullptr || (theNodes == nullptr && theNbNodes != 0))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_viz_presentable_set_visible_nodes: invalid arguments");
      return OCCTL_INVALID_ARGUMENT;
    }
    NCollection_Map<BRepGraph_NodeId> aMask;
    for (size_t anIndex = 0; anIndex < theNbNodes; ++anIndex)
    {
      const BRepGraph_NodeId aNode = OcctL::Topo::UnpackNodeId(theNodes[anIndex]);
      if (!aNode.IsValid())
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_NOT_FOUND,
          "occtl_viz_presentable_set_visible_nodes: invalid NodeId");
        return OCCTL_NOT_FOUND;
      }
      aMask.Add(aNode);
    }
    thePresentable->ais->SetVisibleSubtreeMask(aMask, true);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_clear_visible_nodes(occtl_viz_presentable_t* const thePresentable)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (thePresentable == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_viz_presentable_clear_visible_nodes: presentable is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    thePresentable->ais->ClearVisibleSubtreeMask();
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_display(occtl_viz_view_t* const        theView,
                         occtl_viz_presentable_t* const thePresentable)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr || thePresentable == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_display: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->owner->context->Display(thePresentable->ais, false);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_erase(occtl_viz_view_t* const        theView,
                       occtl_viz_presentable_t* const thePresentable)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr || thePresentable == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_erase: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->owner->context->Erase(thePresentable->ais, false);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_resize(occtl_viz_view_t* const theView,
                                                          const int32_t           theWidth,
                                                          const int32_t           theHeight)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_resize: view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theWidth <= 0 || theHeight <= 0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_viz_view_resize: width/height must be positive");
      return OCCTL_OUT_OF_RANGE;
    }
    theView->width  = theWidth;
    theView->height = theHeight;
    theView->window->SetSize(theWidth, theHeight);
    theView->view->MustBeResized();
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_redraw(occtl_viz_view_t* const theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_redraw: view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->view->Redraw();
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_fit_all(occtl_viz_view_t* const theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_fit_all: view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->view->FitAll(0.01, false);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_set_background(occtl_viz_view_t* const  theView,
                                                                  const occtl_color_rgba_t theColor)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_set_background: view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->view->SetBackgroundColor(ToOcctColor(theColor));
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_set_camera(occtl_viz_view_t* const         theView,
                            const occtl_viz_camera_t* const theCamera)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr || theCamera == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_set_camera: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->view->SetEye(theCamera->eye.x, theCamera->eye.y, theCamera->eye.z);
    theView->view->SetAt(theCamera->center.x, theCamera->center.y, theCamera->center.z);
    theView->view->SetUp(theCamera->up.x, theCamera->up.y, theCamera->up.z);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_get_camera(const occtl_viz_view_t* const theView,
                            occtl_viz_camera_t* const     theOutCamera)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr || theOutCamera == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_get_camera: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->view->Eye(theOutCamera->eye.x, theOutCamera->eye.y, theOutCamera->eye.z);
    theView->view->At(theOutCamera->center.x, theOutCamera->center.y, theOutCamera->center.z);
    theView->view->Up(theOutCamera->up.x, theOutCamera->up.y, theOutCamera->up.z);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_set_standard_view(occtl_viz_view_t* const         theView,
                                   const occtl_viz_standard_view_t theStandardView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_set_standard_view: view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t              aStatus       = OCCTL_OK;
    const V3d_TypeOfOrientation anOrientation = ToOcctStandardView(theStandardView, aStatus);
    if (aStatus != OCCTL_OK)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_viz_view_set_standard_view: invalid standard view");
      return aStatus;
    }
    theView->view->SetProj(anOrientation, false);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_pan(occtl_viz_view_t* const theView,
                                                       const int32_t           theDX,
                                                       const int32_t           theDY)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_pan: view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->view->Pan(theDX, theDY, 1.0, false);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_zoom(occtl_viz_view_t* const theView,
                                                        const int32_t           theX1,
                                                        const int32_t           theY1,
                                                        const int32_t           theX2,
                                                        const int32_t           theY2)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_zoom: view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->view->Zoom(theX1, theY1, theX2, theY2);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_orbit_start(occtl_viz_view_t* const theView,
                                                               const int32_t           theX,
                                                               const int32_t           theY)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_orbit_start: view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->view->StartRotation(theX, theY);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_orbit_update(occtl_viz_view_t* const theView,
                                                                const int32_t           theX,
                                                                const int32_t           theY)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_orbit_update: view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    theView->view->Rotation(theX, theY);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_activate_selection(occtl_viz_view_t* const          theView,
                                    occtl_viz_presentable_t* const   thePresentable,
                                    const occtl_viz_selection_mode_t theMode)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr || thePresentable == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_activate_selection: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsSelectionModeValid(theMode))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE,
                                             "occtl_viz_view_activate_selection: invalid mode");
      return OCCTL_OUT_OF_RANGE;
    }
    theView->owner->context->Activate(thePresentable->ais, static_cast<int>(theMode), true);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_pick(occtl_viz_view_t* const        theView,
                                                        const int32_t                  theX,
                                                        const int32_t                  theY,
                                                        const int32_t                  theSelect,
                                                        occtl_viz_pick_result_t* const theOutPick)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr || theOutPick == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_pick: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutPick->struct_version != OCCTL_VIZ_PICK_RESULT_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "occtl_viz_view_pick: unsupported pick result version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOutPick->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_viz_view_pick: pick result p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    InitPickResult(theOutPick);
    theView->owner->context->MoveTo(theX, theY, theView->view, false);
    if (!theView->owner->context->HasDetected())
    {
      return OCCTL_NOT_FOUND;
    }
    const occ::handle<SelectMgr_EntityOwner> anOwner = theView->owner->context->DetectedOwner();
    if (theSelect != 0)
    {
      theView->owner->context->SelectDetected(AIS_SelectionScheme_Replace);
    }
    const occtl_status_t aStatus = FillPick(theView, anOwner, theOutPick);
    if (aStatus != OCCTL_OK)
    {
      OcctL::Core::ErrorState::Current().Set(
        aStatus,
        "occtl_viz_view_pick: detected owner is not a BRepGraph owner");
    }
    return aStatus;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_read_pixels_rgba(occtl_viz_view_t* const theView,
                                                                    uint8_t* const theOutRgba,
                                                                    const size_t   theCap,
                                                                    size_t* const  theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_read_pixels_rgba: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    const size_t aRequired =
      static_cast<size_t>(theView->width) * static_cast<size_t>(theView->height) * 4u;
    *theOutCount = aRequired;
    if (theOutRgba == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aRequired)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_BUFFER_TOO_SMALL,
        "occtl_viz_view_read_pixels_rgba: output buffer too small");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    Image_PixMap anImage;
    if (!anImage.InitTrash(Image_Format_RGBA,
                           static_cast<size_t>(theView->width),
                           static_cast<size_t>(theView->height)))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_MEMORY,
        "occtl_viz_view_read_pixels_rgba: image allocation failed");
      return OCCTL_OUT_OF_MEMORY;
    }
    if (!theView->view->ToPixMap(anImage, theView->width, theView->height))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_UNSUPPORTED,
        "occtl_viz_view_read_pixels_rgba: offscreen rendering failed");
      return OCCTL_UNSUPPORTED;
    }
    for (int aRow = 0; aRow < theView->height; ++aRow)
    {
      std::memcpy(theOutRgba + static_cast<size_t>(aRow) * static_cast<size_t>(theView->width) * 4u,
                  anImage.Row(static_cast<size_t>(aRow)),
                  static_cast<size_t>(theView->width) * 4u);
    }
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_dump_image(occtl_viz_view_t* const theView,
                                                              const char* const       thePath)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theView == nullptr || thePath == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_viz_view_dump_image: NULL argument");
      return OCCTL_INVALID_ARGUMENT;
    }
    Image_AlienPixMap anImage;
    if (!theView->view->ToPixMap(anImage, theView->width, theView->height))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_UNSUPPORTED,
        "occtl_viz_view_dump_image: offscreen rendering failed");
      return OCCTL_UNSUPPORTED;
    }
    if (!anImage.Save(TCollection_AsciiString(thePath)))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_IO_ERROR,
                                             "occtl_viz_view_dump_image: image save failed");
      return OCCTL_IO_ERROR;
    }
    return OCCTL_OK;
  });
}

} // extern "C"
