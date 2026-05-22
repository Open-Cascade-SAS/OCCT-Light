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
 * @file occtl_viz.h
 * @brief OCCT-Light: graph-native visualization module public API.
 *
 * The visualization module is an opt-in heavy integration over OCCT's native
 * viewer stack.  It displays BRepGraph roots through AIS_BRepGraph and
 * exposes only OCCT-Light handles, IDs, POD options, and status codes.
 *
 * @par Threading.
 *      Rendering calls must run on the thread that owns the native window
 *      and OpenGL context.  The module does not create native windows;
 *      native handles are borrowed from the host UI toolkit.
 */

#ifndef OCCTL_VIZ_H
#define OCCTL_VIZ_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_geom.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Opaque process-local visualization driver handle.
 *
 * Created with #occtl_viz_driver_create and released with
 * #occtl_viz_driver_free. A driver must outlive viewers created from it.
 *
 * @threadsafe No. Use on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_driver_create
 */
typedef struct occtl_viz_driver occtl_viz_driver_t;

/**
 * Opaque viewer handle.
 *
 * Created with #occtl_viz_viewer_create and released with
 * #occtl_viz_viewer_free. A viewer must outlive views and presentables created
 * from it.
 *
 * @threadsafe No. Use on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_viewer_create
 */
typedef struct occtl_viz_viewer occtl_viz_viewer_t;

/**
 * Opaque native or offscreen view handle.
 *
 * Created with #occtl_viz_view_create and released with #occtl_viz_view_free.
 * When a native window handle is supplied in #occtl_viz_view_options_t, the view
 * borrows that handle; the host UI toolkit remains responsible for it.
 *
 * @threadsafe No. Use on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_create
 */
typedef struct occtl_viz_view occtl_viz_view_t;

/**
 * Opaque display object for a BRepGraph root.
 *
 * Created with #occtl_viz_presentable_create and released with
 * #occtl_viz_presentable_free. A presentable borrows its source graph; keep the
 * graph alive until the presentable is freed.
 *
 * @threadsafe No. Use on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_presentable_create
 */
typedef struct occtl_viz_presentable occtl_viz_presentable_t;

/**
 * Display mode used for graph presentables.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_presentable_set_display_mode
 */
typedef enum occtl_viz_display_mode
{
  OCCTL_VIZ_DISPLAY_WIREFRAME         = 0,  /**< Wireframe display. */
  OCCTL_VIZ_DISPLAY_SHADED            = 1,  /**< Shaded display without B-Rep edges. */
  OCCTL_VIZ_DISPLAY_SHADED_WITH_EDGES = 10, /**< Shaded display with B-Rep edges. */
  OCCTL_VIZ_DISPLAY_MESH_DEBUG        = 11, /**< Mesh-debug display mode. */
  OCCTL_VIZ_DISPLAY_RESERVED_FUTURE   = 0x7fffffff
} occtl_viz_display_mode_t;

/**
 * Graph-native selection mode.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_view_activate_selection
 */
typedef enum occtl_viz_selection_mode
{
  OCCTL_VIZ_SELECT_WHOLE           = 0,  /**< Select the whole presentable. */
  OCCTL_VIZ_SELECT_FACE            = 1,  /**< Select faces. */
  OCCTL_VIZ_SELECT_EDGE            = 2,  /**< Select edges. */
  OCCTL_VIZ_SELECT_VERTEX          = 3,  /**< Select vertices. */
  OCCTL_VIZ_SELECT_WIRE            = 4,  /**< Select wires. */
  OCCTL_VIZ_SELECT_SHELL           = 5,  /**< Select shells. */
  OCCTL_VIZ_SELECT_SOLID           = 6,  /**< Select solids. */
  OCCTL_VIZ_SELECT_COEDGE          = 7,  /**< Select coedges. */
  OCCTL_VIZ_SELECT_PRODUCT         = 8,  /**< Select product nodes. */
  OCCTL_VIZ_SELECT_COMPOUND        = 9,  /**< Select compounds. */
  OCCTL_VIZ_SELECT_COMPSOLID       = 10, /**< Select compsolids. */
  OCCTL_VIZ_SELECT_RESERVED_FUTURE = 0x7fffffff
} occtl_viz_selection_mode_t;

/**
 * Standard camera orientation.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_view_set_standard_view
 */
typedef enum occtl_viz_standard_view
{
  OCCTL_VIZ_VIEW_FRONT           = 0, /**< Front view. */
  OCCTL_VIZ_VIEW_BACK            = 1, /**< Back view. */
  OCCTL_VIZ_VIEW_LEFT            = 2, /**< Left view. */
  OCCTL_VIZ_VIEW_RIGHT           = 3, /**< Right view. */
  OCCTL_VIZ_VIEW_TOP             = 4, /**< Top view. */
  OCCTL_VIZ_VIEW_BOTTOM          = 5, /**< Bottom view. */
  OCCTL_VIZ_VIEW_ISO             = 6, /**< Isometric view. */
  OCCTL_VIZ_VIEW_RESERVED_FUTURE = 0x7fffffff
} occtl_viz_standard_view_t;

#define OCCTL_VIZ_DRIVER_OPTIONS_VERSION_1 1u
#define OCCTL_VIZ_VIEW_OPTIONS_VERSION_1 1u

/**
 * Versioned options for occtl_viz_driver_create().
 *
 * Initialize with OCCTL_VIZ_DRIVER_OPTIONS_INIT or
 * occtl_viz_driver_options_init() before use.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_driver_options_init
 */
typedef struct occtl_viz_driver_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_VIZ_DRIVER_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved for extensions; must be NULL. */
  int32_t     enable_vbo;     /**< 0/1; non-zero enables VBOs before view creation. */
  int32_t     enable_vsync;   /**< 0/1; non-zero requests vertical sync. */
} occtl_viz_driver_options_t;

/**
 * Static initializer for occtl_viz_driver_options_t.
 */
#define OCCTL_VIZ_DRIVER_OPTIONS_INIT {OCCTL_VIZ_DRIVER_OPTIONS_VERSION_1, NULL, 1, 1}

/**
 * Versioned options for occtl_viz_view_create().
 *
 * Initialize with OCCTL_VIZ_VIEW_OPTIONS_INIT or
 * occtl_viz_view_options_init() before use.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_view_options_init
 */
typedef struct occtl_viz_view_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_VIZ_VIEW_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved for extensions; must be NULL. */
  int32_t     width;          /**< View width in pixels. Must be positive. */
  int32_t     height;         /**< View height in pixels. Must be positive. */
  void*       native_handle;  /**< Borrowed native window/view handle; NULL for offscreen. */
  int32_t     offscreen;      /**< 0/1; non-zero creates an offscreen-capable neutral view. */
} occtl_viz_view_options_t;

/**
 * Static initializer for occtl_viz_view_options_t.
 */
#define OCCTL_VIZ_VIEW_OPTIONS_INIT {OCCTL_VIZ_VIEW_OPTIONS_VERSION_1, NULL, 640, 480, NULL, 1}

/**
 * Camera placement for a viz view.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_view_set_camera
 */
typedef struct occtl_viz_camera
{
  occtl_point3_t  eye;    /**< Camera eye point. */
  occtl_point3_t  center; /**< Camera target point. */
  occtl_vector3_t up;     /**< Camera up vector. */
} occtl_viz_camera_t;

#define OCCTL_VIZ_PICK_RESULT_VERSION_1 1u

/**
 * Graph identity returned by occtl_viz_view_pick().
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_view_pick
 */
typedef struct occtl_viz_pick_result
{
  uint32_t                   struct_version;  /**< Must be #OCCTL_VIZ_PICK_RESULT_VERSION_1. */
  const void*                p_next;          /**< Reserved; set to NULL. */
  occtl_uid_t                uid;             /**< Persistent identity of the picked node. */
  occtl_node_id_t            node;            /**< Session-local picked node ID. */
  occtl_ref_id_t             ref;             /**< Picked reference, or invalid when none. */
  occtl_viz_selection_mode_t selection_mode;  /**< Selection mode that produced the owner. */
  occtl_transform_t          usage_transform; /**< Accumulated usage transform. */
} occtl_viz_pick_result_t;

#define OCCTL_VIZ_PICK_RESULT_INIT                                                                 \
  {                                                                                                \
    OCCTL_VIZ_PICK_RESULT_VERSION_1, NULL, OCCTL_UID_INVALID, OCCTL_NODE_ID_INVALID,               \
      OCCTL_REF_ID_INVALID, OCCTL_VIZ_SELECT_WHOLE,                                                \
    {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0}}                                 \
  }

/**
 * Runtime initialiser for #occtl_viz_driver_options_t.
 *
 * Sets all fields to #OCCTL_VIZ_DRIVER_OPTIONS_INIT.
 *
 * @param[out] options Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_driver_create
 */
OCCTL_API void OCCTL_CALL occtl_viz_driver_options_init(occtl_viz_driver_options_t* options);

/**
 * Runtime initialiser for #occtl_viz_view_options_t.
 *
 * Sets all fields to #OCCTL_VIZ_VIEW_OPTIONS_INIT.
 *
 * @param[out] options Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_view_create
 */
OCCTL_API void OCCTL_CALL occtl_viz_view_options_init(occtl_viz_view_options_t* options);

/**
 * Runtime initialiser for #occtl_viz_pick_result_t.
 *
 * Sets all fields to #OCCTL_VIZ_PICK_RESULT_INIT.
 *
 * @param[out] pick Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_viz_view_pick
 */
OCCTL_API void OCCTL_CALL occtl_viz_pick_result_init(occtl_viz_pick_result_t* pick);

/**
 * Creates the process-local visualization driver.
 *
 * The returned handle owns the visualization driver state and must be released
 * with #occtl_viz_driver_free. Pass NULL for @p options to use
 * #OCCTL_VIZ_DRIVER_OPTIONS_INIT defaults.
 *
 * @param[in]  options    Borrows it. Optional; NULL uses defaults.
 * @param[out] out_driver Owns it. Must be non-NULL. On success receives a new
 *                        driver handle; on failure receives NULL.
 *
 * @retval OCCTL_OK                Driver created.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_driver is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported version.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_driver_free
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_driver_create(const occtl_viz_driver_options_t* options,
                          occtl_viz_driver_t**              out_driver);

/**
 * Frees a visualization driver.
 *
 * Child viewers, views, and presentables created from @p driver must already be
 * freed. Passing NULL is a no-op.
 *
 * @param[in] driver Owns it. NULL is accepted.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_driver_create
 */
OCCTL_API void OCCTL_CALL occtl_viz_driver_free(occtl_viz_driver_t* driver);

/**
 * Creates a viewer from a visualization driver.
 *
 * The returned viewer borrows @p driver and must be freed before the driver.
 *
 * @param[in]  driver     Borrows it. Must be non-NULL.
 * @param[out] out_viewer Owns it. Must be non-NULL. On success receives a new
 *                        viewer handle; on failure receives NULL.
 *
 * @retval OCCTL_OK                Viewer created.
 * @retval OCCTL_INVALID_ARGUMENT  @p driver or @p out_viewer is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_viewer_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_viewer_create(occtl_viz_driver_t*  driver,
                                                            occtl_viz_viewer_t** out_viewer);

/**
 * Frees a viewer.
 *
 * Views and presentables created from @p viewer must already be freed. Passing
 * NULL is a no-op.
 *
 * @param[in] viewer Owns it. NULL is accepted.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_viewer_create
 */
OCCTL_API void OCCTL_CALL occtl_viz_viewer_free(occtl_viz_viewer_t* viewer);

/**
 * Creates a native or offscreen view.
 *
 * The returned view borrows @p viewer and must be freed before the viewer. Pass
 * NULL for @p options to use #OCCTL_VIZ_VIEW_OPTIONS_INIT defaults.
 *
 * @param[in]  viewer   Borrows it. Must be non-NULL.
 * @param[in]  options  Borrows it. Optional; NULL uses defaults.
 * @param[out] out_view Owns it. Must be non-NULL. On success receives a new
 *                      view handle; on failure receives NULL.
 *
 * @retval OCCTL_OK                View created.
 * @retval OCCTL_INVALID_ARGUMENT  @p viewer or @p out_view is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported version.
 * @retval OCCTL_OUT_OF_RANGE      @p options has a non-positive width or height.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_create(occtl_viz_viewer_t*             viewer,
                                                          const occtl_viz_view_options_t* options,
                                                          occtl_viz_view_t**              out_view);

/**
 * Frees a view.
 *
 * Passing NULL is a no-op.
 *
 * @param[in] view Owns it. NULL is accepted.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_create
 */
OCCTL_API void OCCTL_CALL occtl_viz_view_free(occtl_viz_view_t* view);

/**
 * Creates a display object for a graph root.
 *
 * The returned presentable borrows both @p viewer and @p graph. Keep both alive
 * until the presentable is released with #occtl_viz_presentable_free.
 *
 * @param[in]  viewer          Borrows it. Must be non-NULL.
 * @param[in]  graph           Borrows it. Must be non-NULL and must outlive the
 *                             presentable.
 * @param[in]  root            Root node to display.
 * @param[out] out_presentable Owns it. Must be non-NULL. On success receives a
 *                             new presentable handle; on failure receives NULL.
 *
 * @retval OCCTL_OK                Presentable created.
 * @retval OCCTL_INVALID_ARGUMENT  @p viewer, @p graph, or @p out_presentable is NULL.
 * @retval OCCTL_NOT_FOUND         @p root is not a valid display root.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_presentable_free
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_create(occtl_viz_viewer_t*       viewer,
                               occtl_graph_t*            graph,
                               occtl_node_id_t           root,
                               occtl_viz_presentable_t** out_presentable);

/**
 * Frees a presentable.
 *
 * Passing NULL is a no-op.
 *
 * @param[in] presentable Owns it. NULL is accepted.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_presentable_create
 */
OCCTL_API void OCCTL_CALL occtl_viz_presentable_free(occtl_viz_presentable_t* presentable);

/**
 * Sets the display mode for a presentable.
 *
 * @param[in] presentable Borrows it. Must be non-NULL.
 * @param[in] mode        Display mode.
 *
 * @retval OCCTL_OK                Mode set.
 * @retval OCCTL_INVALID_ARGUMENT  @p presentable is NULL.
 * @retval OCCTL_OUT_OF_RANGE      @p mode is not valid.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_display
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_set_display_mode(occtl_viz_presentable_t* presentable,
                                         occtl_viz_display_mode_t mode);

/**
 * Synchronizes a presentable after graph or metadata changes.
 *
 * @param[in] presentable Borrows it. Must be non-NULL.
 * @param[out] out_has_changed Borrows it. Must be non-NULL; receives a 0/1 changed flag.
 *
 * @retval OCCTL_OK Synchronization completed.
 * @retval OCCTL_INVALID_ARGUMENT @p presentable or @p out_has_changed is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_presentable_invalidate
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_synchronize(occtl_viz_presentable_t* presentable, int32_t* out_has_changed);

/**
 * Invalidates cached presentation state.
 *
 * @param[in] presentable Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK Cache invalidated.
 * @retval OCCTL_INVALID_ARGUMENT @p presentable is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_presentable_synchronize
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_invalidate(occtl_viz_presentable_t* presentable);

/**
 * Restricts display to a visible node mask.
 *
 * @param[in] presentable Borrows it. Must be non-NULL.
 * @param[in] nodes Borrows it. Optional array of node IDs.
 * @param[in] n_nodes Number of entries in @p nodes.
 *
 * @retval OCCTL_OK Mask applied.
 * @retval OCCTL_INVALID_ARGUMENT @p presentable is NULL or @p nodes is NULL with non-zero count.
 * @retval OCCTL_NOT_FOUND An entry in @p nodes is not a valid node ID.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_presentable_clear_visible_nodes
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_set_visible_nodes(occtl_viz_presentable_t* presentable,
                                          const occtl_node_id_t*   nodes,
                                          size_t                   n_nodes);

/**
 * Clears a visible node mask.
 *
 * @param[in] presentable Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK Mask cleared.
 * @retval OCCTL_INVALID_ARGUMENT @p presentable is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_presentable_set_visible_nodes
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_presentable_clear_visible_nodes(occtl_viz_presentable_t* presentable);

/**
 * Displays a presentable in a view.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] presentable Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK Displayed.
 * @retval OCCTL_INVALID_ARGUMENT A required handle is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_erase
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_display(occtl_viz_view_t*        view,
                                                           occtl_viz_presentable_t* presentable);

/**
 * Erases a presentable from a view.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] presentable Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK Erased.
 * @retval OCCTL_INVALID_ARGUMENT A required handle is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_display
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_erase(occtl_viz_view_t*        view,
                                                         occtl_viz_presentable_t* presentable);

/**
 * Resizes a view.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] width New width in pixels.
 * @param[in] height New height in pixels.
 *
 * @retval OCCTL_OK View resized.
 * @retval OCCTL_INVALID_ARGUMENT @p view is NULL.
 * @retval OCCTL_OUT_OF_RANGE @p width or @p height is non-positive.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_redraw
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_resize(occtl_viz_view_t* view,
                                                          int32_t           width,
                                                          int32_t           height);

/**
 * Redraws a view.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK Redrawn.
 * @retval OCCTL_INVALID_ARGUMENT @p view is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_read_pixels_rgba
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_redraw(occtl_viz_view_t* view);

/**
 * Fits all displayed content.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK Camera fitted.
 * @retval OCCTL_INVALID_ARGUMENT @p view is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_set_standard_view
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_fit_all(occtl_viz_view_t* view);

/**
 * Sets the view background color.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] color Background color.
 *
 * @retval OCCTL_OK Color set.
 * @retval OCCTL_INVALID_ARGUMENT @p view is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_redraw
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_set_background(occtl_viz_view_t*  view,
                                                                  occtl_color_rgba_t color);

/**
 * Sets the camera.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] camera Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK Camera set.
 * @retval OCCTL_INVALID_ARGUMENT A required pointer is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_get_camera
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_set_camera(occtl_viz_view_t*         view,
                                                              const occtl_viz_camera_t* camera);

/**
 * Gets the camera.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[out] out_camera Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK Camera returned.
 * @retval OCCTL_INVALID_ARGUMENT A required pointer is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_set_camera
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_get_camera(const occtl_viz_view_t* view,
                                                              occtl_viz_camera_t*     out_camera);

/**
 * Sets a standard camera orientation.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] standard_view Standard view orientation.
 *
 * @retval OCCTL_OK Orientation set.
 * @retval OCCTL_INVALID_ARGUMENT @p view is NULL.
 * @retval OCCTL_OUT_OF_RANGE @p standard_view is not valid.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_fit_all
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_set_standard_view(occtl_viz_view_t* view, occtl_viz_standard_view_t standard_view);

/**
 * Pans the camera by screen-space pixels.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] dx Horizontal delta in pixels.
 * @param[in] dy Vertical delta in pixels.
 *
 * @retval OCCTL_OK Camera panned.
 * @retval OCCTL_INVALID_ARGUMENT @p view is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_zoom
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_pan(occtl_viz_view_t* view,
                                                       int32_t           dx,
                                                       int32_t           dy);

/**
 * Zooms using two screen-space points.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] x1 First x coordinate.
 * @param[in] y1 First y coordinate.
 * @param[in] x2 Second x coordinate.
 * @param[in] y2 Second y coordinate.
 *
 * @retval OCCTL_OK Camera zoomed.
 * @retval OCCTL_INVALID_ARGUMENT @p view is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_pan
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_zoom(occtl_viz_view_t* view, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

/**
 * Starts an orbit interaction.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] x Start x coordinate.
 * @param[in] y Start y coordinate.
 *
 * @retval OCCTL_OK Orbit started.
 * @retval OCCTL_INVALID_ARGUMENT @p view is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_orbit_update
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_orbit_start(occtl_viz_view_t* view,
                                                               int32_t           x,
                                                               int32_t           y);

/**
 * Updates an orbit interaction.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] x Current x coordinate.
 * @param[in] y Current y coordinate.
 *
 * @retval OCCTL_OK Orbit updated.
 * @retval OCCTL_INVALID_ARGUMENT @p view is NULL.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_orbit_start
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_orbit_update(occtl_viz_view_t* view,
                                                                int32_t           x,
                                                                int32_t           y);

/**
 * Activates graph-native selection for a presentable.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] presentable Borrows it. Must be non-NULL.
 * @param[in] mode Selection mode.
 *
 * @retval OCCTL_OK Selection activated.
 * @retval OCCTL_INVALID_ARGUMENT A required handle is NULL.
 * @retval OCCTL_OUT_OF_RANGE @p mode is not valid.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_pick
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_viz_view_activate_selection(occtl_viz_view_t*          view,
                                    occtl_viz_presentable_t*   presentable,
                                    occtl_viz_selection_mode_t mode);

/**
 * Moves/selects at a screen point and returns graph identity.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] x Screen x coordinate.
 * @param[in] y Screen y coordinate.
 * @param[in] select 0/1; non-zero performs selection, zero only detects.
 * @param[out] out_pick Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK Pick result returned.
 * @retval OCCTL_INVALID_ARGUMENT A required pointer is NULL.
 * @retval OCCTL_NOT_FOUND Nothing graph-native was detected.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_activate_selection
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_pick(occtl_viz_view_t*        view,
                                                        int32_t                  x,
                                                        int32_t                  y,
                                                        int32_t                  select,
                                                        occtl_viz_pick_result_t* out_pick);

/**
 * Reads view pixels as tightly packed RGBA bytes.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[out] out_rgba Borrows it. Optional buffer; NULL performs sizing only.
 * @param[in] cap Capacity of @p out_rgba in bytes.
 * @param[out] out_count Borrows it. Receives required or written byte count.
 *
 * @retval OCCTL_OK Pixels written.
 * @retval OCCTL_INVALID_ARGUMENT @p view or @p out_count is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL @p cap is too small for the current view size.
 * @retval OCCTL_OUT_OF_MEMORY Temporary pixel storage could not be allocated.
 * @retval OCCTL_UNSUPPORTED Offscreen pixel extraction is unavailable.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_dump_image
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_read_pixels_rgba(occtl_viz_view_t* view,
                                                                    uint8_t*          out_rgba,
                                                                    size_t            cap,
                                                                    size_t*           out_count);

/**
 * Dumps a view image to a PNG path when OCCT image codecs are available.
 *
 * @param[in] view Borrows it. Must be non-NULL.
 * @param[in] path Borrows it. UTF-8 destination path.
 *
 * @retval OCCTL_OK Image written.
 * @retval OCCTL_INVALID_ARGUMENT A required pointer is NULL.
 * @retval OCCTL_IO_ERROR The image could not be written.
 * @retval OCCTL_UNSUPPORTED Image dumping is unavailable.
 *
 * @threadsafe No. Call on the rendering/native-window owner thread.
 *
 * @sa occtl_viz_view_read_pixels_rgba
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_viz_view_dump_image(occtl_viz_view_t* view,
                                                              const char*       path);

#ifdef __cplusplus
}
#endif

#endif /* OCCTL_VIZ_H */
