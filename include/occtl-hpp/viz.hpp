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

/**
 * @file
 * @brief C++ veneer for graph-native visualization.
 *
 * Provides RAII handles (Driver, Viewer, View, Presentable), idiomatic option
 * structs, camera and pick-result wrappers, and interaction helpers over the
 * viz C ABI.  Failures translate to occtl::Error via check().
 */

#ifndef OCCTL_HPP_VIZ_HPP
#define OCCTL_HPP_VIZ_HPP

#include <occtl/occtl_viz.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace occtl::viz
{

/// @brief Display mode for graph presentables.
enum class DisplayMode
{
  Wireframe       = OCCTL_VIZ_DISPLAY_WIREFRAME,
  Shaded          = OCCTL_VIZ_DISPLAY_SHADED,
  ShadedWithEdges = OCCTL_VIZ_DISPLAY_SHADED_WITH_EDGES,
  MeshDebug       = OCCTL_VIZ_DISPLAY_MESH_DEBUG
};

/// @brief Graph-native selection mode for picking.
enum class SelectionMode
{
  Whole     = OCCTL_VIZ_SELECT_WHOLE,
  Face      = OCCTL_VIZ_SELECT_FACE,
  Edge      = OCCTL_VIZ_SELECT_EDGE,
  Vertex    = OCCTL_VIZ_SELECT_VERTEX,
  Wire      = OCCTL_VIZ_SELECT_WIRE,
  Shell     = OCCTL_VIZ_SELECT_SHELL,
  Solid     = OCCTL_VIZ_SELECT_SOLID,
  CoEdge    = OCCTL_VIZ_SELECT_COEDGE,
  Product   = OCCTL_VIZ_SELECT_PRODUCT,
  Compound  = OCCTL_VIZ_SELECT_COMPOUND,
  CompSolid = OCCTL_VIZ_SELECT_COMPSOLID
};

/// @brief Standard camera orientation presets.
enum class StandardView
{
  Front  = OCCTL_VIZ_VIEW_FRONT,
  Back   = OCCTL_VIZ_VIEW_BACK,
  Left   = OCCTL_VIZ_VIEW_LEFT,
  Right  = OCCTL_VIZ_VIEW_RIGHT,
  Top    = OCCTL_VIZ_VIEW_TOP,
  Bottom = OCCTL_VIZ_VIEW_BOTTOM,
  Iso    = OCCTL_VIZ_VIEW_ISO
};

/// @brief Options for creating a visualization driver.
struct DriverOptions
{
  bool enable_vbo   = true; ///< Enable VBOs before view creation.
  bool enable_vsync = true; ///< Request vertical sync.

  [[nodiscard]] ::occtl_viz_driver_options_t to_c() const noexcept
  {
    ::occtl_viz_driver_options_t anOptions = OCCTL_VIZ_DRIVER_OPTIONS_INIT;
    anOptions.enable_vbo                   = enable_vbo ? 1 : 0;
    anOptions.enable_vsync                 = enable_vsync ? 1 : 0;
    return anOptions;
  }
};

/// @brief Options for creating a native or offscreen view.
struct ViewOptions
{
  int   width         = 640;     ///< View width in pixels.
  int   height        = 480;     ///< View height in pixels.
  void* native_handle = nullptr; ///< Borrowed native window/view handle; nullptr for offscreen.
  bool  offscreen     = true;    ///< Create an offscreen-capable view when true.

  [[nodiscard]] ::occtl_viz_view_options_t to_c() const noexcept
  {
    ::occtl_viz_view_options_t anOptions = OCCTL_VIZ_VIEW_OPTIONS_INIT;
    anOptions.width                      = width;
    anOptions.height                     = height;
    anOptions.native_handle              = native_handle;
    anOptions.offscreen                  = offscreen ? 1 : 0;
    return anOptions;
  }
};

/// @brief Camera placement (eye, target, up) for a viz view.
struct Camera
{
  Point3  eye;    ///< Camera eye point.
  Point3  center; ///< Camera target point.
  Vector3 up;     ///< Camera up vector.

  /// @brief Converts to the C value type for passing to the ABI.
  [[nodiscard]] ::occtl_viz_camera_t to_c() const noexcept
  {
    ::occtl_viz_camera_t aCamera;
    aCamera.eye    = eye.c_type();
    aCamera.center = center.c_type();
    aCamera.up     = up.c_type();
    return aCamera;
  }

  /// @brief Constructs from the C value type received from the ABI.
  static Camera from_c(const ::occtl_viz_camera_t& theC) noexcept
  {
    return Camera{Point3(theC.eye), Point3(theC.center), Vector3(theC.up)};
  }
};

/// @brief Graph identity returned by View::pick().
struct PickResult
{
  UID           uid;             ///< Persistent identity of the picked node.
  NodeId        node;            ///< Session-local picked node ID.
  RefId         ref;             ///< Picked reference, or invalid when none.
  SelectionMode selection_mode;  ///< Selection mode that produced the owner.
  Transform     usage_transform; ///< Accumulated usage transform.

  /// @brief Constructs from the C value type received from the ABI.
  static PickResult from_c(const ::occtl_viz_pick_result_t& theC) noexcept
  {
    PickResult aResult;
    aResult.uid             = UID(theC.uid);
    aResult.node            = NodeId(theC.node);
    aResult.ref             = RefId(theC.ref);
    aResult.selection_mode  = static_cast<SelectionMode>(theC.selection_mode);
    aResult.usage_transform = Transform(theC.usage_transform);
    return aResult;
  }
};

/// @brief RAII wrapper for the process-local visualization driver.
///
/// A @c Driver must outlive any @c Viewer created from it.
class Driver
{
public:
  explicit Driver(const DriverOptions& theOptions = DriverOptions{})
  {
    ::occtl_viz_driver_options_t anOptions = theOptions.to_c();
    check(::occtl_viz_driver_create(&anOptions, &myHandle));
  }

  ~Driver() noexcept { ::occtl_viz_driver_free(myHandle); }

  Driver(const Driver&)            = delete;
  Driver& operator=(const Driver&) = delete;

  Driver(Driver&& theOther) noexcept
      : myHandle(theOther.myHandle)
  {
    theOther.myHandle = nullptr;
  }

  Driver& operator=(Driver&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_viz_driver_free(myHandle);
      myHandle          = theOther.myHandle;
      theOther.myHandle = nullptr;
    }
    return *this;
  }

  ::occtl_viz_driver_t* get() const noexcept { return myHandle; }

private:
  ::occtl_viz_driver_t* myHandle = nullptr;
};

/// @brief RAII wrapper for a viewer bound to a @c Driver.
///
/// A @c Viewer must outlive any @c View or @c Presentable created from it.
class Viewer
{
public:
  explicit Viewer(Driver& theDriver)
  {
    check(::occtl_viz_viewer_create(theDriver.get(), &myHandle));
  }

  ~Viewer() noexcept { ::occtl_viz_viewer_free(myHandle); }

  Viewer(const Viewer&)            = delete;
  Viewer& operator=(const Viewer&) = delete;

  Viewer(Viewer&& theOther) noexcept
      : myHandle(theOther.myHandle)
  {
    theOther.myHandle = nullptr;
  }

  Viewer& operator=(Viewer&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_viz_viewer_free(myHandle);
      myHandle          = theOther.myHandle;
      theOther.myHandle = nullptr;
    }
    return *this;
  }

  ::occtl_viz_viewer_t* get() const noexcept { return myHandle; }

private:
  ::occtl_viz_viewer_t* myHandle = nullptr;
};

/// @brief RAII wrapper for a display object bound to a graph root.
///
/// A @c Presentable borrows its source @c Graph and must not outlive it.
class Presentable
{
public:
  /// @brief Creates a display object for a graph root.
  Presentable(Viewer& theViewer, Graph& theGraph, const NodeId theRoot)
  {
    check(
      ::occtl_viz_presentable_create(theViewer.get(), theGraph.get(), theRoot.get(), &myHandle));
  }

  ~Presentable() noexcept { ::occtl_viz_presentable_free(myHandle); }

  Presentable(const Presentable&)            = delete;
  Presentable& operator=(const Presentable&) = delete;

  Presentable(Presentable&& theOther) noexcept
      : myHandle(theOther.myHandle)
  {
    theOther.myHandle = nullptr;
  }

  Presentable& operator=(Presentable&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_viz_presentable_free(myHandle);
      myHandle          = theOther.myHandle;
      theOther.myHandle = nullptr;
    }
    return *this;
  }

  /// @brief Sets the display mode.
  void set_display_mode(const DisplayMode theMode)
  {
    check(
      ::occtl_viz_presentable_set_display_mode(myHandle,
                                               static_cast<::occtl_viz_display_mode_t>(theMode)));
  }

  /// @brief Synchronizes presentation after graph or metadata changes.
  /// @return true when the synchronized state differs from the cached state.
  bool synchronize()
  {
    int32_t aChanged = 0;
    check(::occtl_viz_presentable_synchronize(myHandle, &aChanged));
    return aChanged != 0;
  }

  /// @brief Invalidates cached presentation state.
  void invalidate() { check(::occtl_viz_presentable_invalidate(myHandle)); }

  /// @brief Restricts display to a visible node mask.
  void set_visible_nodes(const std::vector<NodeId>& theNodes)
  {
    std::vector<::occtl_node_id_t> aIds;
    aIds.reserve(theNodes.size());
    for (const NodeId& aNode : theNodes)
      aIds.push_back(aNode.get());
    check(::occtl_viz_presentable_set_visible_nodes(myHandle, aIds.data(), aIds.size()));
  }

  /// @brief Clears a visible node mask (display everything).
  void clear_visible_nodes() { check(::occtl_viz_presentable_clear_visible_nodes(myHandle)); }

  ::occtl_viz_presentable_t* get() const noexcept { return myHandle; }

private:
  ::occtl_viz_presentable_t* myHandle = nullptr;
};

/// @brief RAII wrapper for a native or offscreen view.
///
/// A @c View borrows its parent @c Viewer and must not outlive it.
class View
{
public:
  explicit View(Viewer& theViewer, const ViewOptions& theOptions = ViewOptions{})
  {
    ::occtl_viz_view_options_t anOptions = theOptions.to_c();
    check(::occtl_viz_view_create(theViewer.get(), &anOptions, &myHandle));
  }

  ~View() noexcept { ::occtl_viz_view_free(myHandle); }

  View(const View&)            = delete;
  View& operator=(const View&) = delete;

  View(View&& theOther) noexcept
      : myHandle(theOther.myHandle)
  {
    theOther.myHandle = nullptr;
  }

  View& operator=(View&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_viz_view_free(myHandle);
      myHandle          = theOther.myHandle;
      theOther.myHandle = nullptr;
    }
    return *this;
  }

  /// @brief Displays a presentable in this view.
  void display(Presentable& thePresentable)
  {
    check(::occtl_viz_view_display(myHandle, thePresentable.get()));
  }

  /// @brief Erases a presentable from this view.
  void erase(Presentable& thePresentable)
  {
    check(::occtl_viz_view_erase(myHandle, thePresentable.get()));
  }

  /// @brief Resizes the view.
  void resize(const int32_t theWidth, const int32_t theHeight)
  {
    check(::occtl_viz_view_resize(myHandle, theWidth, theHeight));
  }

  /// @brief Redraws the view.
  void redraw() { check(::occtl_viz_view_redraw(myHandle)); }

  /// @brief Fits all displayed content into view.
  void fit_all() { check(::occtl_viz_view_fit_all(myHandle)); }

  /// @brief Sets the view background colour.
  void set_background(const ::occtl_color_rgba_t& theColor)
  {
    check(::occtl_viz_view_set_background(myHandle, theColor));
  }

  /// @brief Sets the camera.
  void set_camera(const Camera& theCamera)
  {
    ::occtl_viz_camera_t aCamera = theCamera.to_c();
    check(::occtl_viz_view_set_camera(myHandle, &aCamera));
  }

  /// @brief Returns the current camera.
  [[nodiscard]] Camera get_camera() const
  {
    ::occtl_viz_camera_t aCamera{};
    check(::occtl_viz_view_get_camera(myHandle, &aCamera));
    return Camera::from_c(aCamera);
  }

  /// @brief Sets a standard camera orientation.
  void set_standard_view(const StandardView theView)
  {
    check(::occtl_viz_view_set_standard_view(myHandle,
                                             static_cast<::occtl_viz_standard_view_t>(theView)));
  }

  /// @brief Pans the camera by screen-space pixels.
  void pan(const int32_t theDx, const int32_t theDy)
  {
    check(::occtl_viz_view_pan(myHandle, theDx, theDy));
  }

  /// @brief Zooms using two screen-space points.
  void zoom(const int32_t theX1, const int32_t theY1, const int32_t theX2, const int32_t theY2)
  {
    check(::occtl_viz_view_zoom(myHandle, theX1, theY1, theX2, theY2));
  }

  /// @brief Starts an orbit interaction at a screen point.
  void orbit_start(const int32_t theX, const int32_t theY)
  {
    check(::occtl_viz_view_orbit_start(myHandle, theX, theY));
  }

  /// @brief Updates an orbit interaction at a screen point.
  void orbit_update(const int32_t theX, const int32_t theY)
  {
    check(::occtl_viz_view_orbit_update(myHandle, theX, theY));
  }

  /// @brief Activates graph-native selection for a presentable.
  void activate_selection(Presentable& thePresentable, const SelectionMode theMode)
  {
    check(::occtl_viz_view_activate_selection(myHandle,
                                              thePresentable.get(),
                                              static_cast<::occtl_viz_selection_mode_t>(theMode)));
  }

  /// @brief Picks or selects at a screen point and returns graph identity.
  void pick(const int32_t theX, const int32_t theY, const bool theSelect, PickResult& theResult)
  {
    ::occtl_viz_pick_result_t aRaw = OCCTL_VIZ_PICK_RESULT_INIT;
    check(::occtl_viz_view_pick(myHandle, theX, theY, theSelect ? 1 : 0, &aRaw));
    theResult = PickResult::from_c(aRaw);
  }

  /// @brief Reads view pixels as tightly packed RGBA bytes (two-call buffer pattern).
  [[nodiscard]] std::vector<std::uint8_t> read_pixels_rgba()
  {
    size_t aCount = 0;
    check(::occtl_viz_view_read_pixels_rgba(myHandle, nullptr, 0, &aCount));
    std::vector<std::uint8_t> aPixels(aCount);
    check(::occtl_viz_view_read_pixels_rgba(myHandle, aPixels.data(), aPixels.size(), &aCount));
    return aPixels;
  }

  /// @brief Dumps a view image to a PNG path when OCCT image codecs are available.
  void dump_image(const std::string& thePath)
  {
    check(::occtl_viz_view_dump_image(myHandle, thePath.c_str()));
  }

  ::occtl_viz_view_t* get() const noexcept { return myHandle; }

private:
  ::occtl_viz_view_t* myHandle = nullptr;
};

} // namespace occtl::viz

#endif // OCCTL_HPP_VIZ_HPP
