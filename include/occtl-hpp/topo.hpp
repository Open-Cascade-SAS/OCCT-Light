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
 * @brief C++ veneer for the topo module.
 *
 * Header-only RAII wrappers and exception translation over the C ABI.
 * The public API is STL-shaped while local identifiers follow OCCT style.
 */

#ifndef OCCTL_HPP_TOPO_HPP
#define OCCTL_HPP_TOPO_HPP

#include <occtl/occtl_topo.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/geom.hpp>
#include <occtl-hpp/uid.hpp>

#include <occtl/occtl_topo_algo.h>

#include <cstdint>
#include <exception>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace occtl
{

/// @brief Session-local identity of a graph node.  Mirrors @c occtl_node_id_t.
///
/// The all-zero value is the invalid sentinel (#OCCTL_NODE_ID_INVALID).
/// Check with #is_valid before use.  Kind queries go through Graph methods
/// which internally convert to OCCT entity types via TopoMath.hxx.
class NodeId
{
public:
  /// @brief Wraps an existing C value type (zero-cost).
  explicit NodeId(const ::occtl_node_id_t theId) noexcept
      : myId(theId)
  {
  }

  /// @brief Returns the invalid sentinel (all-zero bits).
  static NodeId invalid() noexcept { return NodeId(OCCTL_NODE_ID_INVALID); }

  /// @brief Borrows-it view of the underlying C value type, for direct ABI calls.
  ::occtl_node_id_t get() const noexcept { return myId; }

  bool is_valid() const noexcept
  {
    return myId.bits != 0;
  } ///< True when the ID is not the all-zero sentinel.

  bool operator==(const NodeId& theOther) const noexcept
  {
    return myId.bits == theOther.myId.bits;
  } ///< Bitwise equality.

  bool operator!=(const NodeId& theOther) const noexcept
  {
    return myId.bits != theOther.myId.bits;
  } ///< Bitwise inequality.

private:
  ::occtl_node_id_t myId;
};

/// @brief Session-local identity of a reference entry.  Mirrors @c occtl_ref_id_t.
///
/// Invalidated by graph compaction together with NodeIds.  The all-zero value
/// (#OCCTL_REF_ID_INVALID) is the invalid sentinel.
class RefId
{
public:
  /// @brief Wraps an existing C value type (zero-cost).
  explicit RefId(const ::occtl_ref_id_t theId) noexcept
      : myId(theId)
  {
  }

  /// @brief Returns the invalid sentinel (all-zero bits).
  static RefId invalid() noexcept { return RefId(OCCTL_REF_ID_INVALID); }

  /// @brief Borrows-it view of the underlying C value type, for direct ABI calls.
  ::occtl_ref_id_t get() const noexcept { return myId; }

  bool is_valid() const noexcept
  {
    return myId.bits != 0;
  } ///< True when the ID is not the all-zero sentinel.

  bool operator==(const RefId& theOther) const noexcept
  {
    return myId.bits == theOther.myId.bits;
  } ///< Bitwise equality.

  bool operator!=(const RefId& theOther) const noexcept
  {
    return myId.bits != theOther.myId.bits;
  } ///< Bitwise inequality.

private:
  ::occtl_ref_id_t myId;
};

/// @brief Identity of a representation (geometry / mesh data).  Mirrors @c occtl_rep_id_t.
///
/// The all-zero value (#OCCTL_REP_ID_INVALID) is the invalid sentinel.
class RepId
{
public:
  /// @brief Wraps an existing C value type (zero-cost).
  explicit RepId(const ::occtl_rep_id_t theId) noexcept
      : myId(theId)
  {
  }

  /// @brief Returns the invalid sentinel (all-zero bits).
  static RepId invalid() noexcept { return RepId(OCCTL_REP_ID_INVALID); }

  /// @brief Borrows-it view of the underlying C value type, for direct ABI calls.
  ::occtl_rep_id_t get() const noexcept { return myId; }

  bool is_valid() const noexcept
  {
    return myId.bits != 0;
  } ///< True when the ID is not the all-zero sentinel.

  bool operator==(const RepId& theOther) const noexcept
  {
    return myId.bits == theOther.myId.bits;
  } ///< Bitwise equality.

  bool operator!=(const RepId& theOther) const noexcept
  {
    return myId.bits != theOther.myId.bits;
  } ///< Bitwise inequality.

private:
  ::occtl_rep_id_t myId;
};

/// @brief Session-local identity of an assembly joint record. Mirrors @c occtl_joint_id_t.
class JointId
{
public:
  /// @brief Wraps an existing C value type (zero-cost).
  explicit JointId(const ::occtl_joint_id_t theId) noexcept
      : myId(theId)
  {
  }

  /// @brief Returns the invalid sentinel (all-zero bits).
  static JointId invalid() noexcept { return JointId(OCCTL_JOINT_ID_INVALID); }

  /// @brief Borrows-it view of the underlying C value type, for direct ABI calls.
  ::occtl_joint_id_t get() const noexcept { return myId; }

  bool is_valid() const noexcept
  {
    return myId.bits != 0;
  } ///< True when the ID is not the all-zero sentinel.

  bool operator==(const JointId& theOther) const noexcept
  {
    return myId.bits == theOther.myId.bits;
  } ///< Bitwise equality.

  bool operator!=(const JointId& theOther) const noexcept
  {
    return myId.bits != theOther.myId.bits;
  } ///< Bitwise inequality.

private:
  ::occtl_joint_id_t myId;
};

/// @brief Orientation of a child entity inside its parent.
using Orientation = ::occtl_orientation_t;

/// @brief NodeId + orientation pair used by topology builders and ordered wire queries.
using OrientedNode = ::occtl_oriented_node_t;

/// @brief Semantic relation kind between two graph nodes.
using RelationKind = ::occtl_relation_kind_t;

/// @brief Point classification result for graph topology.
using PointClass = ::occtl_topo_point_class_t;

/// @brief Closest-distance result between two graph nodes.
using DistancePair = ::occtl_topo_distance_pair_t;

/// @brief Relation/contact query options POD.
using RelationOptions = ::occtl_topo_relation_options_t;

/// @brief One contact solution between two graph nodes.
using TouchHit = ::occtl_topo_touch_hit_t;

/// @brief Axis/face intersection hit.
using AxisHit = ::occtl_topo_axis_hit_t;

/// @brief 3D curve kind used by edge geometry.
using CurveKind = ::occtl_curve_kind_t;

/// @brief Surface kind used by face geometry.
using SurfaceKind = ::occtl_surface_kind_t;

/// @brief Selector options POD.
using SelectOptions = ::occtl_select_options_t;

/// @brief Selector metadata filter extension POD.
using SelectMetadataFilter = ::occtl_select_metadata_filter_t;

/// @brief Borrowed tag view POD.
using TagView = ::occtl_tag_view_t;

/// @brief Selector distance-to-node sort extension POD.
using SelectDistanceToNodeSort = ::occtl_select_distance_to_node_sort_t;

/// @brief Selector bounding-box mode.
using SelectBBoxMode = ::occtl_select_bbox_mode_t;

/// @brief Selector principal axis.
using SelectAxis = ::occtl_select_axis_t;

/// @brief Selector axis-position mode.
using SelectAxisPosition = ::occtl_select_axis_position_t;

/// @brief Selector face-normal predicate.
using SelectNormalMode = ::occtl_select_normal_mode_t;

/// @brief Selector OCCT mass-property predicate.
using SelectMeasureKind = ::occtl_select_measure_kind_t;

/// @brief Selector output sort key.
using SelectSortKey = ::occtl_select_sort_key_t;

/// @brief Selector output sort direction.
using SelectSortDirection = ::occtl_select_sort_direction_t;

/// @brief Selector group options POD.
using SelectGroupOptions = ::occtl_select_group_options_t;

/// @brief Selector grouping key.
using SelectGroupKey = ::occtl_select_group_key_t;

/// @brief Selector group view POD.
using SelectGroupView = ::occtl_select_group_view_t;

/// @brief Category roots returned by OCCT hidden-line projection.
struct HlrCategoryRoots
{
  NodeId visible_sharp   = NodeId::invalid();
  NodeId visible_smooth  = NodeId::invalid();
  NodeId visible_seam    = NodeId::invalid();
  NodeId visible_outline = NodeId::invalid();
  NodeId hidden_sharp    = NodeId::invalid();
  NodeId hidden_smooth   = NodeId::invalid();
  NodeId hidden_seam     = NodeId::invalid();
  NodeId hidden_outline  = NodeId::invalid();
};

/// @brief Graph-level length-unit metadata.
struct GraphUnits
{
  double      length_unit_to_meter = 1.0;
  std::string name                 = "m";
};

/// @brief Material-lite data stored on a graph node.
struct GraphMaterial
{
  std::string          name;
  int32_t              has_density       = 0;
  double               density           = 0.0;
  int32_t              has_diffuse_color = 0;
  ::occtl_color_rgba_t diffuse_color     = {1.0f, 1.0f, 1.0f, 1.0f};
  ::occtl_uid_t        metadata_uid      = OCCTL_UID_INVALID;
};

/// @brief Material-lite C POD.
using MaterialInfo = ::occtl_material_info_t;

/// @brief Assembly joint kind.
using JointKind = ::occtl_joint_kind_t;

/// @brief Assembly joint C POD.
using JointInfo = ::occtl_joint_info_t;

/// @brief Public graph node kind.
using NodeKind = ::occtl_node_kind_t;

/// @brief Plane split side-selection mode.
using TopoSplitKeep = ::occtl_topo_split_keep_t;

/// @brief Options for splitting a shape by a plane.
using TopoSplitByPlaneOptions = ::occtl_topo_split_by_plane_options_t;

/// @brief Plane descriptor for sectioning a shape.
using TopoSectionPlane = ::occtl_topo_section_plane_t;

/// @brief Options for sectioning a shape by planes.
using TopoSectionByPlanesOptions = ::occtl_topo_section_by_planes_options_t;

/// @brief Options for extruding Face nodes into prism solids.
using TopoExtrudeFacesOptions = ::occtl_topo_extrude_faces_options_t;

/// @brief Options for estimating selected-edge maximum fillet radius.
using TopoMaxFilletRadiusOptions = ::occtl_topo_max_fillet_radius_options_t;

/// @brief Options for connecting unordered edges into wires.
using EdgesToWiresOptions = ::occtl_topo_edges_to_wires_options_t;

/// @brief Options for planar wire offsets.
using WireOffset2dOptions = ::occtl_topo_wire_offset_2d_options_t;

/// @brief Options for removing degenerate edge usages from a wire.
using WireFixDegenerateEdgesOptions = ::occtl_topo_wire_fix_degenerate_edges_options_t;

/// @brief Options for planar face chamfers.
using FaceChamfer2dOptions = ::occtl_topo_face_chamfer_2d_options_t;

/// @brief Options for planar wire chamfers.
using WireChamfer2dOptions = ::occtl_topo_wire_chamfer_2d_options_t;

/// @brief Options for creating a face from candidate wires with automatic outer-loop detection.
using MakeFaceFromWiresAutoOptions = ::occtl_topo_make_face_from_wires_auto_options_t;

/// @brief Forward declaration needed so NodeIter can precede Graph.
class Graph;

/// @brief RAII range adapter wrapping @c occtl_node_iter_t.
class NodeIter
{
public:
  class iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = NodeId;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const NodeId*;
    using reference         = const NodeId&;

    iterator() noexcept
        : myIter(nullptr),
          myDone(true)
    {
    }

    explicit iterator(::occtl_node_iter_t* theIter)
        : myIter(theIter),
          myDone(false)
    {
      advance();
    }

    iterator(const iterator&)            = delete;
    iterator& operator=(const iterator&) = delete;
    iterator(iterator&&)                 = default;
    iterator& operator=(iterator&&)      = default;

    /// @brief Returns a reference to the current NodeId.
    const NodeId& operator*() const noexcept { return myCurrent; }

    /// @brief Returns a pointer to the current NodeId.
    const NodeId* operator->() const noexcept { return &myCurrent; }

    /// @brief Advances to the next node and returns the iterator.
    iterator& operator++()
    {
      advance();
      return *this;
    }

    /// @brief Advances to the next node (postfix).
    void operator++(int) { advance(); }

    /// @brief Returns true when both iterators are at the same position or both are done.
    bool operator==(const iterator& theOther) const noexcept
    {
      if (myDone && theOther.myDone)
      {
        return true;
      }
      return myIter == theOther.myIter && myDone == theOther.myDone;
    }

    /// @brief Returns true when iterators differ.
    bool operator!=(const iterator& theOther) const noexcept { return !(*this == theOther); }

  private:
    /// @brief Fetches the next node ID from the underlying C iterator.
    void advance()
    {
      if (myDone || myIter == nullptr)
      {
        myDone = true;
        return;
      }
      ::occtl_node_id_t      anId;
      const ::occtl_status_t aStatus = ::occtl_node_iter_next(myIter, &anId);
      if (aStatus == OCCTL_OK)
      {
        myCurrent = NodeId(anId);
      }
      else
      {
        if (aStatus != OCCTL_NOT_FOUND)
        {
          ::occtl::check(aStatus);
        }
        myDone = true;
      }
    }

    ::occtl_node_iter_t* myIter;
    NodeId               myCurrent{::occtl_node_id_t{0}};
    bool                 myDone;
  };

  NodeIter() noexcept = default;

  explicit NodeIter(::occtl_node_iter_t* theIter) noexcept
      : myIter(theIter)
  {
  }

  NodeIter(const NodeIter&)            = delete;
  NodeIter& operator=(const NodeIter&) = delete;

  /// @brief Move constructor — takes ownership of the iterator from @c theOther.
  NodeIter(NodeIter&& theOther) noexcept
      : myIter(theOther.myIter)
  {
    theOther.myIter = nullptr;
  }

  /// @brief Move assignment — releases any previously owned iterator.
  NodeIter& operator=(NodeIter&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_node_iter_free(myIter);
      myIter          = theOther.myIter;
      theOther.myIter = nullptr;
    }
    return *this;
  }

  ~NodeIter() { ::occtl_node_iter_free(myIter); }

  /// @brief Returns the begin iterator, which fetches the first node on construction.
  iterator begin() noexcept { return iterator(myIter); }

  /// @brief Returns the past-the-end sentinel iterator.
  iterator end() noexcept { return iterator(); }

private:
  ::occtl_node_iter_t* myIter = nullptr;
};

/// @brief RAII range adapter wrapping @c occtl_select_iter_t.
class SelectIter
{
public:
  class iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = NodeId;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const NodeId*;
    using reference         = const NodeId&;

    iterator() noexcept
        : myIter(nullptr),
          myDone(true)
    {
    }

    explicit iterator(::occtl_select_iter_t* theIter)
        : myIter(theIter),
          myDone(false)
    {
      advance();
    }

    iterator(const iterator&)            = delete;
    iterator& operator=(const iterator&) = delete;
    iterator(iterator&&)                 = default;
    iterator& operator=(iterator&&)      = default;

    const NodeId& operator*() const noexcept { return myCurrent; }

    const NodeId* operator->() const noexcept { return &myCurrent; }

    iterator& operator++()
    {
      advance();
      return *this;
    }

    void operator++(int) { advance(); }

    bool operator==(const iterator& theOther) const noexcept
    {
      if (myDone && theOther.myDone)
      {
        return true;
      }
      return myIter == theOther.myIter && myDone == theOther.myDone;
    }

    bool operator!=(const iterator& theOther) const noexcept { return !(*this == theOther); }

  private:
    void advance()
    {
      if (myDone || myIter == nullptr)
      {
        myDone = true;
        return;
      }
      ::occtl_node_id_t      anId{};
      const ::occtl_status_t aStatus = ::occtl_select_iter_next(myIter, &anId);
      if (aStatus == OCCTL_OK)
      {
        myCurrent = NodeId(anId);
      }
      else
      {
        if (aStatus != OCCTL_NOT_FOUND)
        {
          ::occtl::check(aStatus);
        }
        myDone = true;
      }
    }

    ::occtl_select_iter_t* myIter;
    NodeId                 myCurrent{::occtl_node_id_t{0}};
    bool                   myDone;
  };

  SelectIter() noexcept = default;

  explicit SelectIter(::occtl_select_iter_t* theIter) noexcept
      : myIter(theIter)
  {
  }

  SelectIter(const SelectIter&)            = delete;
  SelectIter& operator=(const SelectIter&) = delete;

  SelectIter(SelectIter&& theOther) noexcept
      : myIter(theOther.myIter)
  {
    theOther.myIter = nullptr;
  }

  SelectIter& operator=(SelectIter&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_select_iter_free(myIter);
      myIter          = theOther.myIter;
      theOther.myIter = nullptr;
    }
    return *this;
  }

  ~SelectIter() { ::occtl_select_iter_free(myIter); }

  iterator begin() noexcept { return iterator(myIter); }

  iterator end() noexcept { return iterator(); }

private:
  ::occtl_select_iter_t* myIter = nullptr;
};

/// @brief RAII range adapter wrapping @c occtl_select_group_iter_t.
class SelectGroupIter
{
public:
  class iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = SelectGroupView;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const SelectGroupView*;
    using reference         = const SelectGroupView&;

    iterator() noexcept
        : myIter(nullptr),
          myDone(true)
    {
    }

    explicit iterator(::occtl_select_group_iter_t* theIter)
        : myIter(theIter),
          myDone(false)
    {
      advance();
    }

    iterator(const iterator&)            = delete;
    iterator& operator=(const iterator&) = delete;
    iterator(iterator&&)                 = default;
    iterator& operator=(iterator&&)      = default;

    const SelectGroupView& operator*() const noexcept { return myCurrent; }

    const SelectGroupView* operator->() const noexcept { return &myCurrent; }

    iterator& operator++()
    {
      advance();
      return *this;
    }

    void operator++(int) { advance(); }

    bool operator==(const iterator& theOther) const noexcept
    {
      if (myDone && theOther.myDone)
      {
        return true;
      }
      return myIter == theOther.myIter && myDone == theOther.myDone;
    }

    bool operator!=(const iterator& theOther) const noexcept { return !(*this == theOther); }

  private:
    void advance()
    {
      if (myDone || myIter == nullptr)
      {
        myDone = true;
        return;
      }
      ::occtl_select_group_view_t aView   = OCCTL_SELECT_GROUP_VIEW_INIT;
      const ::occtl_status_t      aStatus = ::occtl_select_group_iter_next(myIter, &aView);
      if (aStatus == OCCTL_OK)
      {
        myCurrent = aView;
      }
      else
      {
        if (aStatus != OCCTL_NOT_FOUND)
        {
          ::occtl::check(aStatus);
        }
        myDone = true;
      }
    }

    ::occtl_select_group_iter_t* myIter;
    SelectGroupView              myCurrent = OCCTL_SELECT_GROUP_VIEW_INIT;
    bool                         myDone;
  };

  SelectGroupIter() noexcept = default;

  explicit SelectGroupIter(::occtl_select_group_iter_t* theIter) noexcept
      : myIter(theIter)
  {
  }

  SelectGroupIter(const SelectGroupIter&)            = delete;
  SelectGroupIter& operator=(const SelectGroupIter&) = delete;

  SelectGroupIter(SelectGroupIter&& theOther) noexcept
      : myIter(theOther.myIter)
  {
    theOther.myIter = nullptr;
  }

  SelectGroupIter& operator=(SelectGroupIter&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_select_group_iter_free(myIter);
      myIter          = theOther.myIter;
      theOther.myIter = nullptr;
    }
    return *this;
  }

  ~SelectGroupIter() { ::occtl_select_group_iter_free(myIter); }

  iterator begin() noexcept { return iterator(myIter); }

  iterator end() noexcept { return iterator(); }

private:
  ::occtl_select_group_iter_t* myIter = nullptr;
};

/// @brief RAII range adapter wrapping @c occtl_topo_axis_hit_iter_t.
class AxisHitIter
{
public:
  class iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = AxisHit;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const AxisHit*;
    using reference         = const AxisHit&;

    iterator() noexcept
        : myIter(nullptr),
          myDone(true)
    {
    }

    explicit iterator(::occtl_topo_axis_hit_iter_t* theIter)
        : myIter(theIter),
          myDone(false)
    {
      advance();
    }

    iterator(const iterator&)            = delete;
    iterator& operator=(const iterator&) = delete;
    iterator(iterator&&)                 = default;
    iterator& operator=(iterator&&)      = default;

    const AxisHit& operator*() const noexcept { return myCurrent; }

    const AxisHit* operator->() const noexcept { return &myCurrent; }

    iterator& operator++()
    {
      advance();
      return *this;
    }

    void operator++(int) { advance(); }

    bool operator==(const iterator& theOther) const noexcept
    {
      if (myDone && theOther.myDone)
      {
        return true;
      }
      return myIter == theOther.myIter && myDone == theOther.myDone;
    }

    bool operator!=(const iterator& theOther) const noexcept { return !(*this == theOther); }

  private:
    void advance()
    {
      if (myDone || myIter == nullptr)
      {
        myDone = true;
        return;
      }
      ::occtl_topo_axis_hit_t aHit{};
      const ::occtl_status_t  aStatus = ::occtl_topo_axis_hit_iter_next(myIter, &aHit);
      if (aStatus == OCCTL_OK)
      {
        myCurrent = aHit;
      }
      else
      {
        if (aStatus != OCCTL_NOT_FOUND)
        {
          ::occtl::check(aStatus);
        }
        myDone = true;
      }
    }

    ::occtl_topo_axis_hit_iter_t* myIter;
    AxisHit                       myCurrent{};
    bool                          myDone;
  };

  AxisHitIter() noexcept = default;

  explicit AxisHitIter(::occtl_topo_axis_hit_iter_t* theIter) noexcept
      : myIter(theIter)
  {
  }

  AxisHitIter(const AxisHitIter&)            = delete;
  AxisHitIter& operator=(const AxisHitIter&) = delete;

  AxisHitIter(AxisHitIter&& theOther) noexcept
      : myIter(theOther.myIter)
  {
    theOther.myIter = nullptr;
  }

  AxisHitIter& operator=(AxisHitIter&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_topo_axis_hit_iter_free(myIter);
      myIter          = theOther.myIter;
      theOther.myIter = nullptr;
    }
    return *this;
  }

  ~AxisHitIter() { ::occtl_topo_axis_hit_iter_free(myIter); }

  iterator begin() noexcept { return iterator(myIter); }

  iterator end() noexcept { return iterator(); }

private:
  ::occtl_topo_axis_hit_iter_t* myIter = nullptr;
};

/// @brief RAII range adapter wrapping @c occtl_topo_touch_iter_t.
class TouchIter
{
public:
  class iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = TouchHit;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const TouchHit*;
    using reference         = const TouchHit&;

    iterator() noexcept
        : myIter(nullptr),
          myDone(true)
    {
    }

    explicit iterator(::occtl_topo_touch_iter_t* theIter)
        : myIter(theIter),
          myDone(false)
    {
      advance();
    }

    iterator(const iterator&)            = delete;
    iterator& operator=(const iterator&) = delete;
    iterator(iterator&&)                 = default;
    iterator& operator=(iterator&&)      = default;

    const TouchHit& operator*() const noexcept { return myCurrent; }

    const TouchHit* operator->() const noexcept { return &myCurrent; }

    iterator& operator++()
    {
      advance();
      return *this;
    }

    void operator++(int) { advance(); }

    bool operator==(const iterator& theOther) const noexcept
    {
      if (myDone && theOther.myDone)
      {
        return true;
      }
      return myIter == theOther.myIter && myDone == theOther.myDone;
    }

    bool operator!=(const iterator& theOther) const noexcept { return !(*this == theOther); }

  private:
    void advance()
    {
      if (myDone || myIter == nullptr)
      {
        myDone = true;
        return;
      }
      ::occtl_topo_touch_hit_t aHit{};
      const ::occtl_status_t   aStatus = ::occtl_topo_touch_iter_next(myIter, &aHit);
      if (aStatus == OCCTL_OK)
      {
        myCurrent = aHit;
      }
      else
      {
        if (aStatus != OCCTL_NOT_FOUND)
        {
          ::occtl::check(aStatus);
        }
        myDone = true;
      }
    }

    ::occtl_topo_touch_iter_t* myIter;
    TouchHit                   myCurrent{};
    bool                       myDone;
  };

  TouchIter() noexcept = default;

  explicit TouchIter(::occtl_topo_touch_iter_t* theIter) noexcept
      : myIter(theIter)
  {
  }

  TouchIter(const TouchIter&)            = delete;
  TouchIter& operator=(const TouchIter&) = delete;

  TouchIter(TouchIter&& theOther) noexcept
      : myIter(theOther.myIter)
  {
    theOther.myIter = nullptr;
  }

  TouchIter& operator=(TouchIter&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_topo_touch_iter_free(myIter);
      myIter          = theOther.myIter;
      theOther.myIter = nullptr;
    }
    return *this;
  }

  ~TouchIter() { ::occtl_topo_touch_iter_free(myIter); }

  iterator begin() noexcept { return iterator(myIter); }

  iterator end() noexcept { return iterator(); }

private:
  ::occtl_topo_touch_iter_t* myIter = nullptr;
};

/// @brief RAII range adapter wrapping @c occtl_topo_intersection_iter_t.
class IntersectionIter
{
public:
  class iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = NodeId;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const NodeId*;
    using reference         = const NodeId&;

    iterator() noexcept
        : myIter(nullptr),
          myDone(true)
    {
    }

    explicit iterator(::occtl_topo_intersection_iter_t* theIter)
        : myIter(theIter),
          myDone(false)
    {
      advance();
    }

    iterator(const iterator&)            = delete;
    iterator& operator=(const iterator&) = delete;
    iterator(iterator&&)                 = default;
    iterator& operator=(iterator&&)      = default;

    const NodeId& operator*() const noexcept { return myCurrent; }

    const NodeId* operator->() const noexcept { return &myCurrent; }

    iterator& operator++()
    {
      advance();
      return *this;
    }

    void operator++(int) { advance(); }

    bool operator==(const iterator& theOther) const noexcept
    {
      if (myDone && theOther.myDone)
      {
        return true;
      }
      return myIter == theOther.myIter && myDone == theOther.myDone;
    }

    bool operator!=(const iterator& theOther) const noexcept { return !(*this == theOther); }

  private:
    void advance()
    {
      if (myDone || myIter == nullptr)
      {
        myDone = true;
        return;
      }
      ::occtl_node_id_t      aNode   = OCCTL_NODE_ID_INVALID;
      const ::occtl_status_t aStatus = ::occtl_topo_intersection_iter_next(myIter, &aNode);
      if (aStatus == OCCTL_OK)
      {
        myCurrent = NodeId(aNode);
      }
      else
      {
        if (aStatus != OCCTL_NOT_FOUND)
        {
          ::occtl::check(aStatus);
        }
        myDone = true;
      }
    }

    ::occtl_topo_intersection_iter_t* myIter;
    NodeId                            myCurrent = NodeId::invalid();
    bool                              myDone;
  };

  IntersectionIter() noexcept = default;

  explicit IntersectionIter(::occtl_topo_intersection_iter_t* theIter) noexcept
      : myIter(theIter)
  {
  }

  IntersectionIter(const IntersectionIter&)            = delete;
  IntersectionIter& operator=(const IntersectionIter&) = delete;

  IntersectionIter(IntersectionIter&& theOther) noexcept
      : myIter(theOther.myIter)
  {
    theOther.myIter = nullptr;
  }

  IntersectionIter& operator=(IntersectionIter&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_topo_intersection_iter_free(myIter);
      myIter          = theOther.myIter;
      theOther.myIter = nullptr;
    }
    return *this;
  }

  ~IntersectionIter() { ::occtl_topo_intersection_iter_free(myIter); }

  iterator begin() noexcept { return iterator(myIter); }

  iterator end() noexcept { return iterator(); }

private:
  ::occtl_topo_intersection_iter_t* myIter = nullptr;
};

/// @brief RAII handle for a batched graph mutation scope.  Mirrors @c occtl_batch_t.
///
/// Created by Graph::begin_batch().  Mutations performed on the graph while
/// a Batch is alive are deferred; commit() applies them, abort() discards them.
/// The destructor calls occtl_batch_abort if the batch has not been committed
/// or moved-from.  Move-only.
class Batch
{
public:
  Batch(const Batch&)            = delete;
  Batch& operator=(const Batch&) = delete;

  /// @brief Move constructor — takes ownership from @c theOther.
  Batch(Batch&& theOther) noexcept
      : myBatch(theOther.myBatch),
        myGraph(theOther.myGraph)
  {
    theOther.myBatch = nullptr;
    theOther.myGraph = nullptr;
  }

  /// @brief Move assignment — releases any previously-owned batch.
  Batch& operator=(Batch&& theOther) noexcept
  {
    if (this != &theOther)
    {
      silentAbort();
      myBatch          = theOther.myBatch;
      theOther.myBatch = nullptr;
      myGraph          = theOther.myGraph;
      theOther.myGraph = nullptr;
    }
    return *this;
  }

  /// @brief Calls abort on destruction if the batch has not been committed.
  /// Swallows the abort status so the destructor never throws.
  ~Batch() noexcept { silentAbort(); }

  /// @brief Returns a borrowed pointer to the graph this batch is scoped over.
  ::occtl_graph_t* graph() noexcept { return myGraph; }

  /// @brief Commits all deferred mutations in the batch to the graph.
  /// After this call the batch handle is freed; further calls are no-ops.
  /// @throws Error on failure.
  void commit()
  {
    if (myBatch != nullptr)
    {
      check(::occtl_batch_commit(myBatch));
      myBatch = nullptr;
    }
  }

  /// @brief Aborts the batch, discarding all deferred mutations.
  /// NULL-tolerant; idempotent.
  void abort()
  {
    if (myBatch != nullptr)
    {
      check(::occtl_batch_abort(myBatch));
      myBatch = nullptr;
    }
  }

private:
  friend class Graph;

  /// @brief Wraps an existing C handle (takes ownership).
  explicit Batch(::occtl_batch_t* const theBatch, ::occtl_graph_t* const theGraph) noexcept
      : myBatch(theBatch),
        myGraph(theGraph)
  {
  }

  /// @brief Non-throwing abort used by the destructor and move-assignment.
  /// The C ABI's status is intentionally discarded: a destructor on the
  /// unwinding path must not throw, and a failed deferred-invalidation
  /// abort still releases the handle.
  void silentAbort() noexcept
  {
    if (myBatch != nullptr)
    {
      ::occtl_batch_abort(myBatch);
      myBatch = nullptr;
    }
  }

  ::occtl_batch_t* myBatch = nullptr;
  ::occtl_graph_t* myGraph = nullptr;
};

/// @brief RAII range adapter wrapping @c occtl_topo_explorer_iter_t.
///
/// Yields a tuple of (NodeId, Transform, Orientation) per step.
/// Created by Graph::child_explorer() and Graph::parent_explorer().
class ExplorerIter
{
public:
  /// @brief One yield from an explorer iteration.
  struct Item
  {
    NodeId      node{::occtl_node_id_t{0}};
    Transform   transform{::occtl_transform_t{}};
    Orientation orientation{OCCTL_ORIENTATION_FORWARD};
  };

  class iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = Item;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const Item*;
    using reference         = const Item&;

    iterator() noexcept
        : myIter(nullptr),
          myDone(true)
    {
    }

    explicit iterator(::occtl_topo_explorer_iter_t* theIter)
        : myIter(theIter),
          myDone(false)
    {
      advance();
    }

    iterator(const iterator&)            = delete;
    iterator& operator=(const iterator&) = delete;
    iterator(iterator&&)                 = default;
    iterator& operator=(iterator&&)      = default;

    const Item& operator*() const noexcept { return myCurrent; }

    const Item* operator->() const noexcept { return &myCurrent; }

    iterator& operator++()
    {
      advance();
      return *this;
    }

    void operator++(int) { advance(); }

    bool operator==(const iterator& theOther) const noexcept
    {
      if (myDone && theOther.myDone)
      {
        return true;
      }
      return myIter == theOther.myIter && myDone == theOther.myDone;
    }

    bool operator!=(const iterator& theOther) const noexcept { return !(*this == theOther); }

  private:
    void advance()
    {
      if (myDone || myIter == nullptr)
      {
        myDone = true;
        return;
      }
      ::occtl_node_id_t      anId{};
      ::occtl_transform_t    aTrsf{};
      ::occtl_orientation_t  aOri = OCCTL_ORIENTATION_FORWARD;
      const ::occtl_status_t aStatus =
        ::occtl_topo_explorer_iter_next(myIter, &anId, &aTrsf, &aOri);
      if (aStatus == OCCTL_OK)
      {
        myCurrent = Item{NodeId(anId), Transform(aTrsf), occtl::Orientation{aOri}};
      }
      else
      {
        if (aStatus != OCCTL_NOT_FOUND)
        {
          ::occtl::check(aStatus);
        }
        myDone = true;
      }
    }

    ::occtl_topo_explorer_iter_t* myIter;
    Item                          myCurrent{};
    bool                          myDone;
  };

  ExplorerIter() noexcept = default;

  explicit ExplorerIter(::occtl_topo_explorer_iter_t* theIter) noexcept
      : myIter(theIter)
  {
  }

  ExplorerIter(const ExplorerIter&)            = delete;
  ExplorerIter& operator=(const ExplorerIter&) = delete;

  ExplorerIter(ExplorerIter&& theOther) noexcept
      : myIter(theOther.myIter)
  {
    theOther.myIter = nullptr;
  }

  ExplorerIter& operator=(ExplorerIter&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_topo_explorer_iter_free(myIter);
      myIter          = theOther.myIter;
      theOther.myIter = nullptr;
    }
    return *this;
  }

  ~ExplorerIter() { ::occtl_topo_explorer_iter_free(myIter); }

  iterator begin() noexcept { return iterator(myIter); }

  iterator end() noexcept { return iterator(); }

private:
  ::occtl_topo_explorer_iter_t* myIter = nullptr;
};

/// @brief RAII range adapter wrapping @c occtl_topo_related_iter_t.
///
/// Yields a tuple of (NodeId, RelationKind) per step.
/// Created by Graph::related().
class RelatedIter
{
public:
  struct Item
  {
    NodeId       node{::occtl_node_id_t{0}};
    RelationKind relation{OCCTL_RELATION_BOUNDARY_EDGE};
  };

  class iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = Item;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const Item*;
    using reference         = const Item&;

    iterator() noexcept
        : myIter(nullptr),
          myDone(true)
    {
    }

    explicit iterator(::occtl_topo_related_iter_t* theIter)
        : myIter(theIter),
          myDone(false)
    {
      advance();
    }

    iterator(const iterator&)            = delete;
    iterator& operator=(const iterator&) = delete;
    iterator(iterator&&)                 = default;
    iterator& operator=(iterator&&)      = default;

    const Item& operator*() const noexcept { return myCurrent; }

    const Item* operator->() const noexcept { return &myCurrent; }

    iterator& operator++()
    {
      advance();
      return *this;
    }

    void operator++(int) { advance(); }

    bool operator==(const iterator& theOther) const noexcept
    {
      if (myDone && theOther.myDone)
      {
        return true;
      }
      return myIter == theOther.myIter && myDone == theOther.myDone;
    }

    bool operator!=(const iterator& theOther) const noexcept { return !(*this == theOther); }

  private:
    void advance()
    {
      if (myDone || myIter == nullptr)
      {
        myDone = true;
        return;
      }
      ::occtl_node_id_t       anId{};
      ::occtl_relation_kind_t aKind   = OCCTL_RELATION_BOUNDARY_EDGE;
      const ::occtl_status_t  aStatus = ::occtl_topo_related_iter_next(myIter, &anId, &aKind);
      if (aStatus == OCCTL_OK)
      {
        myCurrent = Item{NodeId(anId), occtl::RelationKind{aKind}};
      }
      else
      {
        if (aStatus != OCCTL_NOT_FOUND)
        {
          ::occtl::check(aStatus);
        }
        myDone = true;
      }
    }

    ::occtl_topo_related_iter_t* myIter;
    Item                         myCurrent{};
    bool                         myDone;
  };

  RelatedIter() noexcept = default;

  explicit RelatedIter(::occtl_topo_related_iter_t* theIter) noexcept
      : myIter(theIter)
  {
  }

  RelatedIter(const RelatedIter&)            = delete;
  RelatedIter& operator=(const RelatedIter&) = delete;

  RelatedIter(RelatedIter&& theOther) noexcept
      : myIter(theOther.myIter)
  {
    theOther.myIter = nullptr;
  }

  RelatedIter& operator=(RelatedIter&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_topo_related_iter_free(myIter);
      myIter          = theOther.myIter;
      theOther.myIter = nullptr;
    }
    return *this;
  }

  ~RelatedIter() { ::occtl_topo_related_iter_free(myIter); }

  iterator begin() noexcept { return iterator(myIter); }

  iterator end() noexcept { return iterator(); }

private:
  ::occtl_topo_related_iter_t* myIter = nullptr;
};

/// @brief RAII handle for a topology graph.  Mirrors @c occtl_graph_t.
///
/// Created empty by the default constructor; topology is added through
/// the make_* / import APIs.  Move-only — copying a graph handle is not
/// meaningful.
///
/// All query methods translate C status codes to exceptions via occtl::check.
class Graph
{
public:
  /// @brief Creates an empty graph.
  Graph() { check(::occtl_graph_create(&myPtr)); }

  /// @brief Creates an empty graph (naming-aligned alias).
  static Graph create() { return Graph(); }

  /// @brief Wraps an existing handle (takes ownership).
  explicit Graph(::occtl_graph_t* const thePtr) noexcept
      : myPtr(thePtr)
  {
  }

  /// @brief Wraps an existing owned handle; throws on NULL.
  static Graph from_pointer_unsafe(::occtl_graph_t* const thePtr)
  {
    if (thePtr == nullptr)
    {
      throw Error(OCCTL_INVALID_HANDLE, "Graph::from_pointer_unsafe received a null pointer", 0, 0);
    }
    return Graph(thePtr);
  }

  /// @brief Releases the underlying graph.
  ~Graph() { ::occtl_graph_free(myPtr); }

  /// @brief Move-constructs, leaving @c theOther empty.
  Graph(Graph&& theOther) noexcept
      : myPtr(theOther.myPtr)
  {
    theOther.myPtr = nullptr;
  }

  /// @brief Move-assigns, releasing any previously-owned graph.
  Graph& operator=(Graph&& theOther) noexcept
  {
    if (this != &theOther)
    {
      ::occtl_graph_free(myPtr);
      myPtr          = theOther.myPtr;
      theOther.myPtr = nullptr;
    }
    return *this;
  }

  Graph(const Graph&)            = delete;
  Graph& operator=(const Graph&) = delete;

  /// @brief Borrows-it pointer to the underlying C handle, for direct ABI calls.
  ::occtl_graph_t* get() const noexcept { return myPtr; }

  /// @brief Returns the kind of a node ID.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theId is invalid or the node has been removed.
  ::occtl_node_kind_t node_id_kind(const NodeId theId) const
  {
    ::occtl_node_kind_t aKind;
    check(::occtl_graph_node_kind(myPtr, theId.get(), &aKind));
    return aKind;
  }

  /// @brief Returns the kind embedded in a UID.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theUid is invalid or the entity has been
  /// removed.
  ::occtl_node_kind_t uid_kind(const UID theUid) const
  {
    ::occtl_node_kind_t aKind;
    check(::occtl_graph_uid_kind(myPtr, theUid.get(), &aKind));
    return aKind;
  }

  /// @brief Returns the kind of a ref ID.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theId is invalid or the ref has been removed.
  ::occtl_ref_kind_t ref_id_kind(const RefId theId) const
  {
    ::occtl_ref_kind_t aKind;
    check(::occtl_graph_ref_kind(myPtr, theId.get(), &aKind));
    return aKind;
  }

  /// @brief Returns the kind embedded in a RefUID.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theUid is invalid or the ref has been removed.
  ::occtl_ref_kind_t ref_uid_kind(const RefUID theUid) const
  {
    ::occtl_ref_kind_t aKind;
    check(::occtl_graph_ref_uid_kind(myPtr, theUid.get(), &aKind));
    return aKind;
  }

  /// @brief Returns the kind of a rep ID.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theId is invalid or the rep has been removed.
  ::occtl_rep_kind_t rep_id_kind(const RepId theId) const
  {
    ::occtl_rep_kind_t aKind;
    check(::occtl_graph_rep_kind(myPtr, theId.get(), &aKind));
    return aKind;
  }

  /// @brief Resolves a persistent UID to its current NodeId.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theUid refers to a removed entity or is
  /// all-zero.
  NodeId node_id_from_uid(const UID theUid) const
  {
    ::occtl_node_id_t aId;
    check(::occtl_graph_node_id_from_uid(myPtr, theUid.get(), &aId));
    return NodeId(aId);
  }

  /// @brief Returns the persistent UID for a NodeId.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theId is invalid or the node has been removed.
  UID uid_from_node_id(const NodeId theId) const
  {
    ::occtl_uid_t aUid;
    check(::occtl_graph_uid_from_node_id(myPtr, theId.get(), &aUid));
    return UID(aUid);
  }

  /// @brief Returns graph-owned Modified history images for an input UID.
  std::vector<UID> history_modified(const UID theInputUid) const
  {
    return fetch_history(&::occtl_graph_history_modified, theInputUid);
  }

  /// @brief Returns graph-owned Generated history images for an input UID.
  std::vector<UID> history_generated(const UID theInputUid) const
  {
    return fetch_history(&::occtl_graph_history_generated, theInputUid);
  }

  /// @brief Returns all graph-owned deleted history input UIDs.
  std::vector<UID> history_deleted_all() const
  {
    size_t aCount = 0;
    check(::occtl_graph_history_deleted_all(myPtr, nullptr, 0, &aCount));
    std::vector<::occtl_uid_t> aRaw(aCount);
    if (aCount != 0)
    {
      check(::occtl_graph_history_deleted_all(myPtr, aRaw.data(), aCount, &aCount));
    }
    std::vector<UID> aOut;
    aOut.reserve(aRaw.size());
    for (const ::occtl_uid_t& aUid : aRaw)
    {
      aOut.emplace_back(aUid);
    }
    return aOut;
  }

  /// @brief Resolves a persistent RefUID to its current RefId.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theUid refers to a removed ref or is all-zero.
  RefId ref_id_from_ref_uid(const RefUID theUid) const
  {
    ::occtl_ref_id_t aId;
    check(::occtl_graph_ref_id_from_ref_uid(myPtr, theUid.get(), &aId));
    return RefId(aId);
  }

  /// @brief Returns the persistent RefUID for a RefId.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theId is invalid or the ref has been removed.
  RefUID ref_uid_from_ref_id(const RefId theId) const
  {
    ::occtl_ref_uid_t aUid;
    check(::occtl_graph_ref_uid_from_ref_id(myPtr, theId.get(), &aUid));
    return RefUID(aUid);
  }

  /// @brief Resolves a persistent RepUID to its current RepId.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theUid refers to a removed rep or is all-zero.
  RepId rep_id_from_rep_uid(const RepUID theUid) const
  {
    ::occtl_rep_id_t aId;
    check(::occtl_graph_rep_id_from_rep_uid(myPtr, theUid.get(), &aId));
    return RepId(aId);
  }

  /// @brief Returns the persistent RepUID for a RepId.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theId is invalid or the rep has been removed.
  RepUID rep_uid_from_rep_id(const RepId theId) const
  {
    ::occtl_rep_uid_t aUid;
    check(::occtl_graph_rep_uid_from_rep_id(myPtr, theId.get(), &aUid));
    return RepUID(aUid);
  }

  /// @brief Returns active RefUID to RefId pairs for all references in the graph.
  /// @sa occtl_graph_ref_uid_table
  std::vector<std::pair<RefUID, RefId>> ref_uid_table() const
  {
    size_t aCount = 0;
    check(::occtl_graph_ref_uid_table(myPtr, nullptr, nullptr, 0, &aCount));
    std::vector<::occtl_ref_uid_t> aUids(aCount);
    std::vector<::occtl_ref_id_t>  aRefs(aCount);
    if (aCount != 0)
    {
      check(::occtl_graph_ref_uid_table(myPtr, aUids.data(), aRefs.data(), aCount, &aCount));
    }

    std::vector<std::pair<RefUID, RefId>> aResult;
    aResult.reserve(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      aResult.emplace_back(RefUID(aUids[anIndex]), RefId(aRefs[anIndex]));
    }
    return aResult;
  }

  size_t solid_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_solid_count(myPtr, &aCount));
    return aCount;
  } ///< Active solid count.

  size_t shell_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_shell_count(myPtr, &aCount));
    return aCount;
  } ///< Active shell count.

  size_t face_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_face_count(myPtr, &aCount));
    return aCount;
  } ///< Active face count.

  size_t wire_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_wire_count(myPtr, &aCount));
    return aCount;
  } ///< Active wire count.

  size_t edge_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_edge_count(myPtr, &aCount));
    return aCount;
  } ///< Active edge count.

  size_t vertex_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_vertex_count(myPtr, &aCount));
    return aCount;
  } ///< Active vertex count.

  size_t compound_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_compound_count(myPtr, &aCount));
    return aCount;
  } ///< Active compound count.

  size_t compsolid_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_compsolid_count(myPtr, &aCount));
    return aCount;
  } ///< Active compsolid count.

  size_t coedge_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_coedge_count(myPtr, &aCount));
    return aCount;
  } ///< Active coedge count.

  size_t product_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_product_count(myPtr, &aCount));
    return aCount;
  } ///< Active product count.

  size_t occurrence_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_occurrence_count(myPtr, &aCount));
    return aCount;
  } ///< Active occurrence count.

  size_t node_count() const
  {
    size_t aCount = 0;
    check(::occtl_graph_node_count(myPtr, &aCount));
    return aCount;
  } ///< Total active node count across all kinds.

  /// @brief Runs graph validation and returns all reported issues.
  /// @throws Error when the underlying C validation call fails.
  std::vector<::occtl_topo_check_issue_t> check_issues() const
  {
    size_t aCount = 0;
    check(::occtl_topo_check(myPtr, nullptr, 0, &aCount));
    std::vector<::occtl_topo_check_issue_t> anIssues(aCount);
    if (aCount != 0)
    {
      check(::occtl_topo_check(myPtr, anIssues.data(), anIssues.size(), &aCount));
      anIssues.resize(aCount);
    }
    return anIssues;
  }

  /// @brief Returns true when graph validation reports no issues.
  /// @throws Error when the underlying C validation call fails.
  bool is_valid() const { return check_issues().empty(); }

  /// @brief Returns the 3D point of a vertex.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theVertex is invalid, removed, or not a
  /// vertex.
  Point3 vertex_point(const NodeId theVertex) const
  {
    ::occtl_point3_t aPoint;
    check(::occtl_topo_vertex_point(myPtr, theVertex.get(), &aPoint));
    return Point3(aPoint);
  }

  /// @brief Returns the tolerance of a vertex.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theVertex is invalid, removed, or not a
  /// vertex.
  double vertex_tolerance(const NodeId theVertex) const
  {
    double aTol = 0.0;
    check(::occtl_topo_vertex_tolerance(myPtr, theVertex.get(), &aTol));
    return aTol;
  }

  /// @brief Returns the parametric range of an edge's 3D curve.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  std::pair<double, double> edge_range(const NodeId theEdge) const
  {
    double aFirst = 0.0, aLast = 0.0;
    check(::occtl_topo_edge_range(myPtr, theEdge.get(), &aFirst, &aLast));
    return {aFirst, aLast};
  }

  /// @brief Returns the tolerance of an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  double edge_tolerance(const NodeId theEdge) const
  {
    double aTol = 0.0;
    check(::occtl_topo_edge_tolerance(myPtr, theEdge.get(), &aTol));
    return aTol;
  }

  /// @brief Returns whether an edge is degenerated.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  bool edge_is_degenerated(const NodeId theEdge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_edge_is_degenerated(myPtr, theEdge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns whether an edge has a 3D curve.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  bool edge_has_curve(const NodeId theEdge) const
  {
    int32_t aHas = 0;
    check(::occtl_topo_edge_has_curve(myPtr, theEdge.get(), &aHas));
    return aHas != 0;
  }

  /// @brief Returns the 3D curve kind carried by an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid or removed.
  /// @throws Error with code OCCTL_WRONG_KIND when @c theEdge is not an edge.
  CurveKind edge_curve_kind(const NodeId theEdge) const
  {
    ::occtl_curve_kind_t aKind = OCCTL_CURVE_KIND_UNDEFINED;
    check(::occtl_topo_edge_curve_kind(myPtr, theEdge.get(), &aKind));
    return aKind;
  }

  /// @brief Returns the start vertex of an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  NodeId edge_start_vertex(const NodeId theEdge) const
  {
    ::occtl_node_id_t aId;
    check(::occtl_topo_edge_start_vertex(myPtr, theEdge.get(), &aId));
    return NodeId(aId);
  }

  /// @brief Returns the end vertex of an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  NodeId edge_end_vertex(const NodeId theEdge) const
  {
    ::occtl_node_id_t aId;
    check(::occtl_topo_edge_end_vertex(myPtr, theEdge.get(), &aId));
    return NodeId(aId);
  }

  /// @brief Returns whether a coedge is a seam (closed-surface) edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  bool coedge_is_seam(const NodeId theCoedge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_coedge_is_seam(myPtr, theCoedge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns the parent edge of a coedge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  NodeId coedge_edge_of(const NodeId theCoedge) const
  {
    ::occtl_node_id_t aId;
    check(::occtl_topo_coedge_edge_of(myPtr, theCoedge.get(), &aId));
    return NodeId(aId);
  }

  /// @brief Returns the parent face of a coedge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  NodeId coedge_face_of(const NodeId theCoedge) const
  {
    ::occtl_node_id_t aId;
    check(::occtl_topo_coedge_face_of(myPtr, theCoedge.get(), &aId));
    return NodeId(aId);
  }

  /// @brief Returns the tolerance of a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  double face_tolerance(const NodeId theFace) const
  {
    double aTol = 0.0;
    check(::occtl_topo_face_tolerance(myPtr, theFace.get(), &aTol));
    return aTol;
  }

  /// @brief Returns the outer wire of a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face,
  ///         or when the face has no outer wire.
  NodeId face_outer_wire(const NodeId theFace) const
  {
    ::occtl_node_id_t aId;
    check(::occtl_topo_face_outer_wire(myPtr, theFace.get(), &aId));
    return NodeId(aId);
  }

  /// @brief Returns the UV parameter bounds of a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  void face_uv_bounds(const NodeId theFace,
                      double&      theUMin,
                      double&      theUMax,
                      double&      theVMin,
                      double&      theVMax) const
  {
    check(
      ::occtl_topo_face_uv_bounds(myPtr, theFace.get(), &theUMin, &theUMax, &theVMin, &theVMax));
  }

  /// @brief Returns whether a face has a surface.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  bool face_has_surface(const NodeId theFace) const
  {
    int32_t aHas = 0;
    check(::occtl_topo_face_has_surface(myPtr, theFace.get(), &aHas));
    return aHas != 0;
  }

  /// @brief Returns the surface kind carried by a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid or removed.
  /// @throws Error with code OCCTL_WRONG_KIND when @c theFace is not a face.
  SurfaceKind face_surface_kind(const NodeId theFace) const
  {
    ::occtl_surface_kind_t aKind = OCCTL_SURFACE_KIND_UNDEFINED;
    check(::occtl_topo_face_surface_kind(myPtr, theFace.get(), &aKind));
    return aKind;
  }

  /// @brief Returns whether a wire is topologically closed.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theWire is invalid, removed, or not a wire.
  bool wire_is_closed(const NodeId theWire) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_wire_is_closed(myPtr, theWire.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns whether a shell is topologically closed (watertight).
  /// @throws Error with code OCCTL_NOT_FOUND when @c theShell is invalid, removed, or not a shell.
  bool shell_is_closed(const NodeId theShell) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_shell_is_closed(myPtr, theShell.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns the parameter of a vertex on an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theVertex or @c theEdge is invalid, removed,
  /// or wrong kind.
  double vertex_parameter(const NodeId theVertex, const NodeId theEdge) const
  {
    double aParam = 0.0;
    check(::occtl_topo_vertex_parameter(myPtr, theVertex.get(), theEdge.get(), &aParam));
    return aParam;
  }

  /// @brief Returns the UV parameters of a vertex on a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theVertex or @c theFace is invalid, removed,
  /// or wrong kind.
  Point2 vertex_parameters(const NodeId theVertex, const NodeId theFace) const
  {
    ::occtl_point2_t aUV;
    check(::occtl_topo_vertex_parameters(myPtr, theVertex.get(), theFace.get(), &aUV));
    return Point2(aUV);
  }

  /// @brief Returns whether an edge has the same parameterisation on every face it bounds.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  bool edge_same_parameter(const NodeId theEdge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_edge_same_parameter(myPtr, theEdge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns whether an edge has the same range in 3D and on its pcurves.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  bool edge_same_range(const NodeId theEdge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_edge_same_range(myPtr, theEdge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns whether an edge is manifold.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  bool edge_is_manifold(const NodeId theEdge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_edge_is_manifold(myPtr, theEdge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns whether an edge is a boundary edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  bool edge_is_boundary(const NodeId theEdge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_edge_is_boundary(myPtr, theEdge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns whether an edge is a seam edge on a given face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge or @c theFace is invalid or removed.
  bool edge_is_seam_on_face(const NodeId theEdge, const NodeId theFace) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_edge_is_seam_on_face(myPtr, theEdge.get(), theFace.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns whether a coedge is reversed.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  bool coedge_is_reversed(const NodeId theCoedge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_coedge_is_reversed(myPtr, theCoedge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns whether a coedge has a pcurve.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  bool coedge_has_pcurve(const NodeId theCoedge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_coedge_has_pcurve(myPtr, theCoedge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns the parameter of a vertex on the pcurve carried by a coedge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge or @c theVertex is invalid, removed,
  /// or wrong kind.
  double coedge_pcurve_parameter(const NodeId theCoedge, const NodeId theVertex) const
  {
    double aParam = 0.0;
    check(::occtl_topo_coedge_pcurve_parameter(myPtr, theCoedge.get(), theVertex.get(), &aParam));
    return aParam;
  }

  /// @brief Returns the parametric range of a coedge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  std::pair<double, double> coedge_range(const NodeId theCoedge) const
  {
    double aFirst = 0.0, aLast = 0.0;
    check(::occtl_topo_coedge_range(myPtr, theCoedge.get(), &aFirst, &aLast));
    return {aFirst, aLast};
  }

  /// @brief Returns the UV points at the start and end of a coedge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  void coedge_uv_points(const NodeId      theCoedge,
                        ::occtl_point2_t& theUVStart,
                        ::occtl_point2_t& theUVEnd) const
  {
    check(::occtl_topo_coedge_uv_points(myPtr, theCoedge.get(), &theUVStart, &theUVEnd));
  }

  /// @brief Returns the paired coedge for a seam edge, or an invalid NodeId for non-seam coedges.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  NodeId coedge_seam_pair(const NodeId theCoedge) const
  {
    ::occtl_node_id_t aPair;
    check(::occtl_topo_coedge_seam_pair(myPtr, theCoedge.get(), &aPair));
    return NodeId(aPair);
  }

  /// @brief Returns whether a face has natural restriction.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  bool face_natural_restriction(const NodeId theFace) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_face_natural_restriction(myPtr, theFace.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns whether a face has a triangulation.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  bool face_has_triangulation(const NodeId theFace) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_face_has_triangulation(myPtr, theFace.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns the face a wire belongs to.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theWire is invalid, removed, or not a wire.
  NodeId wire_face_of(const NodeId theWire) const
  {
    ::occtl_node_id_t aFace;
    check(::occtl_topo_wire_face_of(myPtr, theWire.get(), &aFace));
    return NodeId(aFace);
  }

  /// @brief Returns whether a wire is the outer wire of its parent face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theWire is invalid, removed, or not a wire.
  bool wire_is_outer(const NodeId theWire) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_wire_is_outer(myPtr, theWire.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Evaluates the PCurve UV point on a coedge at parameter @c theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  Point2 coedge_pcurve_eval(const NodeId theCoedge, const double theU) const
  {
    ::occtl_point2_t aUV{};
    check(::occtl_topo_coedge_pcurve_eval(myPtr, theCoedge.get(), theU, &aUV));
    return Point2(aUV);
  }

  /// @brief Evaluates the PCurve UV point and first derivative on a coedge at parameter @c theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  std::pair<Point2, Vector2> coedge_pcurve_eval_d1(const NodeId theCoedge, const double theU) const
  {
    ::occtl_point2_t  aUV{};
    ::occtl_vector2_t aD1{};
    check(::occtl_topo_coedge_pcurve_eval_d1(myPtr, theCoedge.get(), theU, &aUV, &aD1));
    return {Point2{aUV}, Vector2{aD1}};
  }

  /// @brief Evaluates the PCurve UV point and first two derivatives on a coedge at parameter @c
  /// theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  std::tuple<Point2, Vector2, Vector2> coedge_pcurve_eval_d2(const NodeId theCoedge,
                                                             const double theU) const
  {
    ::occtl_point2_t  aUV{};
    ::occtl_vector2_t aD1{}, aD2{};
    check(::occtl_topo_coedge_pcurve_eval_d2(myPtr, theCoedge.get(), theU, &aUV, &aD1, &aD2));
    return {Point2{aUV}, Vector2{aD1}, Vector2{aD2}};
  }

  /// @brief Evaluates the PCurve UV point and first three derivatives on a coedge at parameter @c
  /// theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  std::tuple<Point2, Vector2, Vector2, Vector2> coedge_pcurve_eval_d3(const NodeId theCoedge,
                                                                      const double theU) const
  {
    ::occtl_point2_t  aUV{};
    ::occtl_vector2_t aD1{}, aD2{}, aD3{};
    check(::occtl_topo_coedge_pcurve_eval_d3(myPtr, theCoedge.get(), theU, &aUV, &aD1, &aD2, &aD3));
    return {Point2{aUV}, Vector2{aD1}, Vector2{aD2}, Vector2{aD3}};
  }

  /// @brief Evaluates the Nth derivative on a coedge pcurve at parameter @c theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  Vector2 coedge_pcurve_eval_dn(const NodeId   theCoedge,
                                const double   theU,
                                const uint32_t theN) const
  {
    ::occtl_vector2_t aDN{};
    check(::occtl_topo_coedge_pcurve_eval_dn(myPtr, theCoedge.get(), theU, theN, &aDN));
    return Vector2{aDN};
  }

  /// @brief Evaluates the 3D point on an edge at parameter @c theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  Point3 edge_eval(const NodeId theEdge, const double theU) const
  {
    ::occtl_point3_t aP{};
    check(::occtl_topo_edge_eval(myPtr, theEdge.get(), theU, &aP));
    return Point3(aP);
  }

  /// @brief Evaluates the 3D point and first derivative on an edge at parameter @c theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  std::pair<Point3, Vector3> edge_eval_d1(const NodeId theEdge, const double theU) const
  {
    ::occtl_point3_t  aP{};
    ::occtl_vector3_t aD1{};
    check(::occtl_topo_edge_eval_d1(myPtr, theEdge.get(), theU, &aP, &aD1));
    return {Point3{aP}, Vector3{aD1}};
  }

  /// @brief Evaluates the first two derivatives on an edge at parameter @c theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  std::tuple<Point3, Vector3, Vector3> edge_eval_d2(const NodeId theEdge, const double theU) const
  {
    ::occtl_point3_t  aP{};
    ::occtl_vector3_t aD1{}, aD2{};
    check(::occtl_topo_edge_eval_d2(myPtr, theEdge.get(), theU, &aP, &aD1, &aD2));
    return {Point3{aP}, Vector3{aD1}, Vector3{aD2}};
  }

  /// @brief Evaluates the first three derivatives on an edge at parameter @c theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  std::tuple<Point3, Vector3, Vector3, Vector3> edge_eval_d3(const NodeId theEdge,
                                                             const double theU) const
  {
    ::occtl_point3_t  aP{};
    ::occtl_vector3_t aD1{}, aD2{}, aD3{};
    check(::occtl_topo_edge_eval_d3(myPtr, theEdge.get(), theU, &aP, &aD1, &aD2, &aD3));
    return {Point3{aP}, Vector3{aD1}, Vector3{aD2}, Vector3{aD3}};
  }

  /// @brief Evaluates the Nth derivative on an edge at parameter @c theU.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  Vector3 edge_eval_dn(const NodeId theEdge, const double theU, const uint32_t theN) const
  {
    ::occtl_vector3_t aDN{};
    check(::occtl_topo_edge_eval_dn(myPtr, theEdge.get(), theU, theN, &aDN));
    return Vector3{aDN};
  }

  /// @brief Evaluates the 3D point on a face at UV parameters (@c theU, @c theV).
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  Point3 face_eval(const NodeId theFace, const double theU, const double theV) const
  {
    ::occtl_point3_t aP{};
    check(::occtl_topo_face_eval(myPtr, theFace.get(), theU, theV, &aP));
    return Point3(aP);
  }

  /// @brief Evaluates the 3D point, D1U, and D1V on a face at UV parameters (@c theU, @c theV).
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  std::tuple<Point3, Vector3, Vector3> face_eval_d1(const NodeId theFace,
                                                    const double theU,
                                                    const double theV) const
  {
    ::occtl_point3_t  aP{};
    ::occtl_vector3_t aD1U{}, aD1V{};
    check(::occtl_topo_face_eval_d1(myPtr, theFace.get(), theU, theV, &aP, &aD1U, &aD1V));
    return {Point3{aP}, Vector3{aD1U}, Vector3{aD1V}};
  }

  /// @brief Evaluates the 3D point and first two partial derivatives on a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  std::tuple<Point3, Vector3, Vector3, Vector3, Vector3, Vector3> face_eval_d2(
    const NodeId theFace,
    const double theU,
    const double theV) const
  {
    ::occtl_point3_t  aP{};
    ::occtl_vector3_t aD1U{}, aD1V{}, aD2U{}, aD2V{}, aD2UV{};
    check(::occtl_topo_face_eval_d2(myPtr,
                                    theFace.get(),
                                    theU,
                                    theV,
                                    &aP,
                                    &aD1U,
                                    &aD1V,
                                    &aD2U,
                                    &aD2V,
                                    &aD2UV));
    return {Point3{aP}, Vector3{aD1U}, Vector3{aD1V}, Vector3{aD2U}, Vector3{aD2V}, Vector3{aD2UV}};
  }

  /// @brief Evaluates the 3D point and first three partial derivatives on a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  std::
    tuple<Point3, Vector3, Vector3, Vector3, Vector3, Vector3, Vector3, Vector3, Vector3, Vector3>
    face_eval_d3(const NodeId theFace, const double theU, const double theV) const
  {
    ::occtl_point3_t  aP{};
    ::occtl_vector3_t aD1U{}, aD1V{}, aD2U{}, aD2V{}, aD2UV{};
    ::occtl_vector3_t aD3U{}, aD3V{}, aD3UUV{}, aD3UVV{};
    check(::occtl_topo_face_eval_d3(myPtr,
                                    theFace.get(),
                                    theU,
                                    theV,
                                    &aP,
                                    &aD1U,
                                    &aD1V,
                                    &aD2U,
                                    &aD2V,
                                    &aD2UV,
                                    &aD3U,
                                    &aD3V,
                                    &aD3UUV,
                                    &aD3UVV));
    return {Point3{aP},
            Vector3{aD1U},
            Vector3{aD1V},
            Vector3{aD2U},
            Vector3{aD2V},
            Vector3{aD2UV},
            Vector3{aD3U},
            Vector3{aD3V},
            Vector3{aD3UUV},
            Vector3{aD3UVV}};
  }

  /// @brief Evaluates the (theNu, theNv) cross derivative on a face at UV parameters (@c theU, @c
  /// theV).
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  Vector3 face_eval_dn(const NodeId   theFace,
                       const double   theU,
                       const double   theV,
                       const uint32_t theNu,
                       const uint32_t theNv) const
  {
    ::occtl_vector3_t aDN{};
    check(::occtl_topo_face_eval_dn(myPtr, theFace.get(), theU, theV, theNu, theNv, &aDN));
    return Vector3{aDN};
  }

  /// @brief Returns the number of wires on a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  uint32_t face_wire_count(const NodeId theFace) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_face_wire_count(myPtr, theFace.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of coedges in a wire.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theWire is invalid, removed, or not a wire.
  uint32_t wire_coedge_count(const NodeId theWire) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_wire_coedge_count(myPtr, theWire.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of faces referencing an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  uint32_t edge_face_count(const NodeId theEdge) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_edge_face_count(myPtr, theEdge.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of distinct edges in a wire.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theWire is invalid, removed, or not a wire.
  uint32_t wire_distinct_edge_count(const NodeId theWire) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_wire_distinct_edge_count(myPtr, theWire.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of edges that reference a vertex.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theVertex is invalid, removed, or not a
  /// vertex.
  uint32_t vertex_edge_count(const NodeId theVertex) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_vertex_edge_count(myPtr, theVertex.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of shells in a solid.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theSolid is invalid, removed, or not a solid.
  uint32_t solid_shell_count(const NodeId theSolid) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_solid_shell_count(myPtr, theSolid.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of faces in a shell.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theShell is invalid, removed, or not a shell.
  uint32_t shell_face_count(const NodeId theShell) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_shell_face_count(myPtr, theShell.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of edges in a wire.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theWire is invalid, removed, or not a wire.
  uint32_t wire_edge_count(const NodeId theWire) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_wire_edge_count(myPtr, theWire.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of vertices on an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  uint32_t edge_vertex_count(const NodeId theEdge) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_edge_vertex_count(myPtr, theEdge.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of occurrences of a product.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theProduct is invalid, removed, or not a
  /// product.
  uint32_t product_occurrence_count(const NodeId theProduct) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_product_occurrence_count(myPtr, theProduct.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of children in a compound.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCompound is invalid, removed, or not a
  /// compound.
  uint32_t compound_child_count(const NodeId theCompound) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_compound_child_count(myPtr, theCompound.get(), &aCount));
    return aCount;
  }

  /// @brief Returns the number of solids in a compsolid.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCompSolid is invalid, removed, or not a
  /// compsolid.
  uint32_t compsolid_solid_count(const NodeId theCompSolid) const
  {
    uint32_t aCount = 0;
    check(::occtl_topo_compsolid_solid_count(myPtr, theCompSolid.get(), &aCount));
    return aCount;
  }

  /// @brief Creates a wire explorer that visits coedges in geometric traversal order.
  /// @throws Error on failure (invalid argument, wire not found, or wrong kind).
  NodeIter wire_explorer(const NodeId theWire) const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_topo_wire_explorer_create(myPtr, theWire.get(), &anIt));
    return NodeIter(anIt);
  }

  /// @brief Returns wire edges in endpoint-chaining order with coedge orientation.
  /// @throws Error on failure (invalid argument, wire not found, wrong kind, or buffer error).
  std::vector<OrientedNode> wire_order_edges(const NodeId theWire) const
  {
    size_t aCount = 0;
    check(::occtl_topo_wire_order_edges(myPtr, theWire.get(), nullptr, 0, &aCount));

    std::vector<OrientedNode> aResult(aCount);
    if (aCount > 0)
    {
      check(::occtl_topo_wire_order_edges(myPtr,
                                          theWire.get(),
                                          aResult.data(),
                                          aResult.size(),
                                          &aCount));
      aResult.resize(aCount);
    }
    return aResult;
  }

  /// @brief Creates a child explorer for accumulated-location/orientation downward traversal.
  /// @throws Error on failure (invalid argument, root not found, version mismatch, or OOM).
  ExplorerIter child_explorer(
    const NodeId                                      theRoot,
    const ::occtl_topo_child_explorer_config_t* const theConfig = nullptr) const
  {
    ::occtl_topo_explorer_iter_t* anIt = nullptr;
    check(::occtl_topo_child_explorer_create(myPtr, theRoot.get(), theConfig, &anIt));
    return ExplorerIter(anIt);
  }

  /// @brief Creates a parent explorer for accumulated-location/orientation upward traversal.
  /// @throws Error on failure (invalid argument, node not found, version mismatch, or OOM).
  ExplorerIter parent_explorer(
    const NodeId                                       theNode,
    const ::occtl_topo_parent_explorer_config_t* const theConfig = nullptr) const
  {
    ::occtl_topo_explorer_iter_t* anIt = nullptr;
    check(::occtl_topo_parent_explorer_create(myPtr, theNode.get(), theConfig, &anIt));
    return ExplorerIter(anIt);
  }

  /// @brief Creates a related-iterator over the semantic neighbours of @c theNode.
  /// @throws Error on failure (invalid argument, node not found, or OOM).
  RelatedIter related(const NodeId theNode) const
  {
    ::occtl_topo_related_iter_t* anIt = nullptr;
    check(::occtl_topo_related_iter_create(myPtr, theNode.get(), &anIt));
    return RelatedIter(anIt);
  }

  /// @brief Computes the closest-distance pair between two graph nodes.
  /// @throws Error on failure (invalid argument, node not found, or invalid geometry).
  DistancePair distance_pair(const NodeId theNodeA, const NodeId theNodeB) const
  {
    DistancePair aPair{};
    check(::occtl_topo_distance_pair(myPtr, theNodeA.get(), theNodeB.get(), &aPair));
    return aPair;
  }

  /// @brief Returns contact solutions between two graph nodes.
  /// @throws Error on failure (invalid argument, node not found, or invalid geometry).
  TouchIter touches(const NodeId                 theNodeA,
                    const NodeId                 theNodeB,
                    const RelationOptions* const theOptions = nullptr) const
  {
    ::occtl_topo_touch_iter_t* anIt = nullptr;
    check(::occtl_topo_touch_iter_create(myPtr, theNodeA.get(), theNodeB.get(), theOptions, &anIt));
    return TouchIter(anIt);
  }

  /// @brief Inserts and returns generated intersection nodes for two graph nodes.
  /// @throws Error on failure (invalid argument, node not found, or invalid geometry).
  IntersectionIter intersections(const NodeId                 theNodeA,
                                 const NodeId                 theNodeB,
                                 const RelationOptions* const theOptions = nullptr)
  {
    ::occtl_topo_intersection_iter_t* anIt = nullptr;
    check(::occtl_topo_intersection_iter_create(myPtr,
                                                theNodeA.get(),
                                                theNodeB.get(),
                                                theOptions,
                                                &anIt));
    return IntersectionIter(anIt);
  }

  /// @brief Returns ordered face intersections with an axis under a graph root.
  /// @throws Error on failure (invalid argument, root not found, or invalid geometry).
  AxisHitIter faces_intersected_by_axis(const NodeId          theRoot,
                                        const Axis1Placement& theAxis,
                                        const double          theMinParameter,
                                        const double          theMaxParameter,
                                        const double          theTolerance) const
  {
    ::occtl_topo_axis_hit_iter_t* anIt = nullptr;
    check(::occtl_topo_axis_intersect_faces(myPtr,
                                            theRoot.get(),
                                            theAxis.c_type(),
                                            theMinParameter,
                                            theMaxParameter,
                                            theTolerance,
                                            &anIt));
    return AxisHitIter(anIt);
  }

  /// @brief Tests whether two edge or face nodes share geometric support.
  /// @throws Error on failure (invalid argument, node not found, or wrong kind).
  bool is_same_geometry(const NodeId theNodeA,
                        const NodeId theNodeB,
                        const double theTolerance) const
  {
    int32_t aFlag = 0;
    check(
      ::occtl_topo_is_same_geometry(myPtr, theNodeA.get(), theNodeB.get(), theTolerance, &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns vertex nodes common to two graph roots.
  /// @throws Error on failure (invalid argument, node not found, or buffer error).
  std::vector<NodeId> common_vertices(const NodeId theNodeA, const NodeId theNodeB) const
  {
    size_t aCount = 0;
    check(::occtl_topo_common_vertices(myPtr, theNodeA.get(), theNodeB.get(), nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (aCount > 0)
    {
      check(::occtl_topo_common_vertices(myPtr,
                                         theNodeA.get(),
                                         theNodeB.get(),
                                         aRaw.data(),
                                         aRaw.size(),
                                         &aCount));
    }

    std::vector<NodeId> aResult;
    aResult.reserve(aCount);
    for (size_t anI = 0; anI < aCount; ++anI)
    {
      aResult.push_back(NodeId(aRaw[anI]));
    }
    return aResult;
  }

  /// @brief Returns edges adjacent to @p theEdge through shared vertices.
  /// @throws Error on failure (invalid argument, node not found, wrong kind, or buffer error).
  std::vector<NodeId> adjacent_edges(const NodeId theEdge) const
  {
    size_t aCount = 0;
    check(::occtl_topo_adjacent_edges(myPtr, theEdge.get(), nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (aCount > 0)
    {
      check(::occtl_topo_adjacent_edges(myPtr, theEdge.get(), aRaw.data(), aRaw.size(), &aCount));
    }

    std::vector<NodeId> aResult;
    aResult.reserve(aCount);
    for (size_t anI = 0; anI < aCount; ++anI)
    {
      aResult.push_back(NodeId(aRaw[anI]));
    }
    return aResult;
  }

  /// @brief Returns faces adjacent to @p theFace through shared edges.
  /// @throws Error on failure (invalid argument, node not found, wrong kind, or buffer error).
  std::vector<NodeId> adjacent_faces(const NodeId theFace) const
  {
    size_t aCount = 0;
    check(::occtl_topo_adjacent_faces(myPtr, theFace.get(), nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (aCount > 0)
    {
      check(::occtl_topo_adjacent_faces(myPtr, theFace.get(), aRaw.data(), aRaw.size(), &aCount));
    }

    std::vector<NodeId> aResult;
    aResult.reserve(aCount);
    for (size_t anI = 0; anI < aCount; ++anI)
    {
      aResult.push_back(NodeId(aRaw[anI]));
    }
    return aResult;
  }

  /// @brief Returns all edges connected to @p theSeedEdge through shared vertices.
  /// @throws Error on failure (invalid argument, node not found, wrong kind, or buffer error).
  std::vector<NodeId> connected_edges(const NodeId theSeedEdge) const
  {
    size_t aCount = 0;
    check(::occtl_topo_connected_edges(myPtr, theSeedEdge.get(), nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (aCount > 0)
    {
      check(
        ::occtl_topo_connected_edges(myPtr, theSeedEdge.get(), aRaw.data(), aRaw.size(), &aCount));
    }

    std::vector<NodeId> aResult;
    aResult.reserve(aCount);
    for (size_t anI = 0; anI < aCount; ++anI)
    {
      aResult.push_back(NodeId(aRaw[anI]));
    }
    return aResult;
  }

  /// @brief Returns all faces connected to @p theSeedFace through shared edges.
  /// @throws Error on failure (invalid argument, node not found, wrong kind, or buffer error).
  std::vector<NodeId> connected_faces(const NodeId theSeedFace) const
  {
    size_t aCount = 0;
    check(::occtl_topo_connected_faces(myPtr, theSeedFace.get(), nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (aCount > 0)
    {
      check(
        ::occtl_topo_connected_faces(myPtr, theSeedFace.get(), aRaw.data(), aRaw.size(), &aCount));
    }

    std::vector<NodeId> aResult;
    aResult.reserve(aCount);
    for (size_t anI = 0; anI < aCount; ++anI)
    {
      aResult.push_back(NodeId(aRaw[anI]));
    }
    return aResult;
  }

  /// @brief Returns same-kind topological hop distance under @p theRoot, or -1 if disconnected.
  /// @throws Error on failure (invalid argument, node not found, or wrong kind).
  int32_t graph_distance(const NodeId               theRoot,
                         const std::vector<NodeId>& theSources,
                         const NodeId               theTarget) const
  {
    std::vector<::occtl_node_id_t> aRawSources;
    aRawSources.reserve(theSources.size());
    for (const NodeId& aSource : theSources)
    {
      aRawSources.push_back(aSource.get());
    }

    int32_t aDistance = -1;
    check(::occtl_topo_graph_distance(myPtr,
                                      theRoot.get(),
                                      aRawSources.data(),
                                      aRawSources.size(),
                                      theTarget.get(),
                                      &aDistance));
    return aDistance;
  }

  /// @brief Returns same-kind topological hop distance from one source under @p theRoot.
  /// @throws Error on failure (invalid argument, node not found, or wrong kind).
  int32_t graph_distance(const NodeId theRoot, const NodeId theSource, const NodeId theTarget) const
  {
    const ::occtl_node_id_t aSource   = theSource.get();
    int32_t                 aDistance = -1;
    check(
      ::occtl_topo_graph_distance(myPtr, theRoot.get(), &aSource, 1, theTarget.get(), &aDistance));
    return aDistance;
  }

  /// @brief Classifies a point relative to a solid node.
  /// @throws Error on failure (invalid argument, node not found, or wrong kind).
  PointClass classify_point(const NodeId  theSolid,
                            const Point3& thePoint,
                            const double  theTolerance) const
  {
    PointClass aClass = OCCTL_TOPO_POINT_CLASS_UNKNOWN;
    check(
      ::occtl_topo_classify_point(myPtr, theSolid.get(), thePoint.c_type(), theTolerance, &aClass));
    return aClass;
  }

  /// @brief Returns whether a point is inside a solid node.
  /// @throws Error on failure (invalid argument, node not found, or wrong kind).
  bool is_inside(const NodeId  theSolid,
                 const Point3& thePoint,
                 const double  theTolerance,
                 const bool    theIncludeBoundary = true) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_is_inside(myPtr,
                                 theSolid.get(),
                                 thePoint.c_type(),
                                 theTolerance,
                                 theIncludeBoundary ? 1 : 0,
                                 &aFlag));
    return aFlag != 0;
  }

  NodeIter solids() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_solid_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active solids.

  NodeIter shells() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_shell_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active shells.

  NodeIter faces() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_face_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active faces.

  NodeIter wires() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_wire_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active wires.

  NodeIter edges() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_edge_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active edges.

  NodeIter vertices() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_vertex_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active vertices.

  NodeIter compounds() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_compound_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active compounds.

  NodeIter compsolids() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_compsolid_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active compsolids.

  NodeIter coedges() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_coedge_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active coedges.

  NodeIter products() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_product_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active products.

  NodeIter occurrences() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_occurrence_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all active occurrences.

  NodeIter root_products() const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_graph_root_product_iter_create(myPtr, &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over all root products.

  /// @brief Selects graph nodes matching @p theOptions.
  /// @sa occtl_select_iter_create
  SelectIter select(const SelectOptions& theOptions)
  {
    ::occtl_select_iter_t* anIt = nullptr;
    check(::occtl_select_iter_create(myPtr, &theOptions, &anIt));
    return SelectIter(anIt);
  }

  /// @brief Selects graph nodes matching @p theOptions and carrying @p theTag.
  /// @sa occtl_select_tagged_iter_create
  SelectIter select_tagged(const SelectOptions& theOptions,
                           const char* const    theTag,
                           const size_t         theTagLen)
  {
    ::occtl_select_iter_t* anIt = nullptr;
    check(::occtl_select_tagged_iter_create(myPtr, &theOptions, theTag, theTagLen, &anIt));
    return SelectIter(anIt);
  }

  /// @brief Groups selected graph nodes matching @p theSelectOptions by @p theGroupOptions.
  /// @sa occtl_select_group_iter_create
  SelectGroupIter select_groups(const SelectOptions&      theSelectOptions,
                                const SelectGroupOptions& theGroupOptions)
  {
    ::occtl_select_group_iter_t* anIt = nullptr;
    check(::occtl_select_group_iter_create(myPtr, &theSelectOptions, &theGroupOptions, &anIt));
    return SelectGroupIter(anIt);
  }

  NodeIter shells_of_solid(const NodeId theSolid) const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_topo_shells_of_solid_iter_create(myPtr, theSolid.get(), &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over the shells of a solid.

  NodeIter faces_of_shell(const NodeId theShell) const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_topo_faces_of_shell_iter_create(myPtr, theShell.get(), &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over the faces of a shell.

  NodeIter wires_of_face(const NodeId theFace) const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_topo_wires_of_face_iter_create(myPtr, theFace.get(), &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over the wires of a face.

  NodeIter coedges_of_wire(const NodeId theWire) const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_topo_coedges_of_wire_iter_create(myPtr, theWire.get(), &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over the coedges of a wire.

  NodeIter edges_of_wire(const NodeId theWire) const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_topo_edges_of_wire_iter_create(myPtr, theWire.get(), &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over the edges of a wire.

  NodeIter vertices_of_edge(const NodeId theEdge) const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_topo_vertices_of_edge_iter_create(myPtr, theEdge.get(), &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over the vertices of an edge.

  NodeIter occurrences_of_product(const NodeId theProduct) const
  {
    ::occtl_node_iter_t* anIt = nullptr;
    check(::occtl_topo_occurrences_of_product_iter_create(myPtr, theProduct.get(), &anIt));
    return NodeIter(anIt);
  } ///< Range-for adapter over the occurrences of a product.

  /// @brief Creates a vertex node from a point and tolerance.
  /// @throws Error on failure (invalid argument, version mismatch, or out of memory).
  NodeId make_vertex(const ::occtl_topo_make_vertex_info_t& theInfo)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_vertex(myPtr, &theInfo, &anId));
    return NodeId(anId);
  }

  /// @brief Creates an edge node.
  /// @throws Error on failure (invalid argument, vertex not found, version mismatch, or out of
  /// memory).
  NodeId make_edge(const ::occtl_topo_make_edge_info_t& theInfo)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_edge(myPtr, &theInfo, &anId));
    return NodeId(anId);
  }

  /// @brief Creates a wire node from an ordered span of oriented edges.
  /// @throws Error on failure (invalid argument, child not found, version mismatch, or out of
  /// memory).
  NodeId make_wire(const ::occtl_topo_make_wire_info_t& theInfo)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_wire(myPtr, &theInfo, &anId));
    return NodeId(anId);
  }

  /// @brief Connects unordered edges into wire nodes.
  /// @throws Error on failure (invalid argument, child not found, version mismatch, or out of
  /// memory).
  std::vector<NodeId> edges_to_wires(const EdgesToWiresOptions& theOptions)
  {
    size_t aCount = 0;
    check(::occtl_topo_edges_to_wires(myPtr, &theOptions, nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (aCount > 0)
    {
      check(::occtl_topo_edges_to_wires(myPtr, &theOptions, aRaw.data(), aRaw.size(), &aCount));
    }

    std::vector<NodeId> aResult;
    aResult.reserve(aCount);
    for (size_t anI = 0; anI < aCount; ++anI)
    {
      aResult.push_back(NodeId(aRaw[anI]));
    }
    return aResult;
  }

  /// @brief Creates a planar offset wire in this graph.
  /// @throws Error on failure (invalid argument, wire not found, wrong kind, or OCCT failure).
  NodeId wire_offset_2d(const WireOffset2dOptions& theOptions)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_wire_offset_2d(myPtr, &theOptions, &anId));
    return NodeId(anId);
  }

  /// @brief Removes degenerate edge usages from a wire in place.
  /// @return Number of detached coedge usages.
  /// @throws Error on failure (invalid argument, wire not found, or wrong kind).
  size_t wire_fix_degenerate_edges(const WireFixDegenerateEdgesOptions& theOptions)
  {
    size_t aRemoved = 0;
    check(::occtl_topo_wire_fix_degenerate(myPtr, &theOptions, &aRemoved));
    return aRemoved;
  }

  /// @brief Chamfers corners of a planar face and inserts the result into this graph.
  /// @throws Error on failure (invalid argument, face not found, wrong kind, or OCCT failure).
  NodeId face_chamfer_2d(const FaceChamfer2dOptions& theOptions)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_face_chamfer_2d(myPtr, &theOptions, &anId));
    return NodeId(anId);
  }

  /// @brief Chamfers corners of a planar wire and inserts the result into this graph.
  /// @throws Error on failure (invalid argument, wire not found, wrong kind, or OCCT failure).
  NodeId wire_chamfer_2d(const WireChamfer2dOptions& theOptions)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_wire_chamfer_2d(myPtr, &theOptions, &anId));
    return NodeId(anId);
  }

  /// @brief Creates a face node from a surface, outer wire, and optional inner wires.
  /// @throws Error on failure (invalid argument, child not found, version mismatch, or out of
  /// memory).
  NodeId make_face(const ::occtl_topo_make_face_info_t& theInfo)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_face(myPtr, &theInfo, &anId));
    return NodeId(anId);
  }

  /// @brief Creates a face from candidate wires using automatic outer-loop detection.
  /// @throws Error on failure (invalid argument, wire not found, wrong kind, or OCCT failure).
  NodeId make_face_from_wires_auto(const MakeFaceFromWiresAutoOptions& theOptions)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_face_from_wires_auto(myPtr, &theOptions, &anId));
    return NodeId(anId);
  }

  /// @brief Creates a shell node from a span of oriented faces.
  /// @throws Error on failure (invalid argument, child not found, version mismatch, or out of
  /// memory).
  NodeId make_shell(const ::occtl_topo_make_shell_info_t& theInfo)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_shell(myPtr, &theInfo, &anId));
    return NodeId(anId);
  }

  /// @brief Creates a solid node from a span of oriented shells.
  /// @throws Error on failure (invalid argument, child not found, version mismatch, or out of
  /// memory).
  NodeId make_solid(const ::occtl_topo_make_solid_info_t& theInfo)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_solid(myPtr, &theInfo, &anId));
    return NodeId(anId);
  }

  /// @brief Fills @p theView with the current scalar state of @p theSolid.
  /// @throws Error on failure (invalid argument, not found, wrong kind, or version mismatch).
  /// @sa occtl_topo_solid_view
  void solid_view(const NodeId theSolid, ::occtl_solid_view_t& theView) const
  {
    ::occtl_solid_view_init(&theView);
    check(::occtl_topo_solid_view(myPtr, theSolid.get(), &theView));
  }

  /// @brief Fills @p theView with the current scalar state of @p theCompound.
  /// @throws Error on failure (invalid argument, not found, wrong kind, or version mismatch).
  /// @sa occtl_topo_compound_view
  void compound_view(const NodeId theCompound, ::occtl_compound_view_t& theView) const
  {
    ::occtl_compound_view_init(&theView);
    check(::occtl_topo_compound_view(myPtr, theCompound.get(), &theView));
  }

  /// @brief Fills @p theView with the current scalar state of @p theWire.
  /// @throws Error on failure (invalid argument, not found, wrong kind, or version mismatch).
  /// @sa occtl_topo_wire_view
  void wire_view(const NodeId theWire, ::occtl_wire_view_t& theView) const
  {
    ::occtl_wire_view_init(&theView);
    check(::occtl_topo_wire_view(myPtr, theWire.get(), &theView));
  }

  /// @brief Fills @p theView with the current scalar state of @p theShell.
  /// @throws Error on failure (invalid argument, not found, wrong kind, or version mismatch).
  /// @sa occtl_topo_shell_view
  void shell_view(const NodeId theShell, ::occtl_shell_view_t& theView) const
  {
    ::occtl_shell_view_init(&theView);
    check(::occtl_topo_shell_view(myPtr, theShell.get(), &theView));
  }

  /// @brief Fills @p theView with the current scalar state of @p theVertex.
  /// @throws Error on failure (invalid argument, not found, wrong kind, or version mismatch).
  /// @sa occtl_topo_vertex_view
  void vertex_view(const NodeId theVertex, ::occtl_vertex_view_t& theView) const
  {
    ::occtl_vertex_view_init(&theView);
    check(::occtl_topo_vertex_view(myPtr, theVertex.get(), &theView));
  }

  /// @brief Creates a compsolid node from a span of oriented solids.
  /// @throws Error on failure (invalid argument, child not found, version mismatch, or out of
  /// memory).
  NodeId make_compsolid(const ::occtl_topo_make_compsolid_info_t& theInfo)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_compsolid(myPtr, &theInfo, &anId));
    return NodeId(anId);
  }

  /// @brief Creates a compound node from a span of oriented children.
  /// @throws Error on failure (invalid argument, child not found, version mismatch, or out of
  /// memory).
  NodeId make_compound(const ::occtl_topo_make_compound_info_t& theInfo)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_compound(myPtr, &theInfo, &anId));
    return NodeId(anId);
  }

  /// @brief Removes a node from the graph.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theId is invalid or already removed.
  void remove(const NodeId theId) { check(::occtl_topo_remove(myPtr, theId.get())); }

  /// @brief Recursively removes a node and its entire subgraph.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theId is invalid or already removed.
  void remove_subgraph(const NodeId theId)
  {
    check(::occtl_topo_remove_subgraph(myPtr, theId.get()));
  }

  /// @brief Opens a batched mutation scope on this graph.
  /// @throws Error on failure (OCCTL_INVALID_ARGUMENT or OCCTL_OUT_OF_MEMORY).
  /// @sa Batch, occtl_graph_begin_batch
  Batch begin_batch()
  {
    ::occtl_batch_t* aBatch = nullptr;
    check(::occtl_graph_begin_batch(myPtr, &aBatch));
    return Batch(aBatch, myPtr);
  }

  /// @brief Sets the 3D point of a vertex.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theVertex is invalid, removed, or not a
  /// vertex.
  /// @sa occtl_topo_set_vertex_point
  void set_vertex_point(const NodeId theVertex, const ::occtl_point3_t& thePoint)
  {
    check(::occtl_topo_set_vertex_point(myPtr, theVertex.get(), thePoint));
  }

  /// @brief Sets the tolerance of a vertex.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theVertex is invalid, removed, or not a
  /// vertex.
  /// @sa occtl_topo_set_vertex_tolerance
  void set_vertex_tolerance(const NodeId theVertex, const double theTol)
  {
    check(::occtl_topo_set_vertex_tolerance(myPtr, theVertex.get(), theTol));
  }

  /// @brief Sets the tolerance of an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  /// @sa occtl_topo_set_edge_tolerance
  void set_edge_tolerance(const NodeId theEdge, const double theTol)
  {
    check(::occtl_topo_set_edge_tolerance(myPtr, theEdge.get(), theTol));
  }

  /// @brief Sets the tolerance of a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  /// @sa occtl_topo_set_face_tolerance
  void set_face_tolerance(const NodeId theFace, const double theTol)
  {
    check(::occtl_topo_set_face_tolerance(myPtr, theFace.get(), theTol));
  }

  /// @brief Sets the parametric range of an edge's 3D curve.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  /// @sa occtl_topo_set_edge_param_range
  void set_edge_param_range(const NodeId theEdge, const double theFirst, const double theLast)
  {
    check(::occtl_topo_set_edge_param_range(myPtr, theEdge.get(), theFirst, theLast));
  }

  /// @brief Sets the same-parameter flag on an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  /// @sa occtl_topo_set_edge_same_parameter
  void set_edge_same_parameter(const NodeId theEdge, const bool theFlag)
  {
    check(::occtl_topo_set_edge_same_parameter(myPtr, theEdge.get(), theFlag ? 1 : 0));
  }

  /// @brief Sets the same-range flag on an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  /// @sa occtl_topo_set_edge_same_range
  void set_edge_same_range(const NodeId theEdge, const bool theFlag)
  {
    check(::occtl_topo_set_edge_same_range(myPtr, theEdge.get(), theFlag ? 1 : 0));
  }

  /// @brief Sets the is-degenerate flag on an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  /// @sa occtl_topo_set_edge_is_degenerate
  void set_edge_is_degenerate(const NodeId theEdge, const bool theFlag)
  {
    check(::occtl_topo_set_edge_is_degenerate(myPtr, theEdge.get(), theFlag ? 1 : 0));
  }

  /// @brief Sets the is-closed flag on an edge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theEdge is invalid, removed, or not an edge.
  /// @sa occtl_topo_set_edge_is_closed
  void set_edge_is_closed(const NodeId theEdge, const bool theFlag)
  {
    check(::occtl_topo_set_edge_is_closed(myPtr, theEdge.get(), theFlag ? 1 : 0));
  }

  /// @brief Sets the is-closed flag on a wire.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theWire is invalid, removed, or not a wire.
  /// @sa occtl_topo_set_wire_is_closed
  void set_wire_is_closed(const NodeId theWire, const bool theFlag)
  {
    check(::occtl_topo_set_wire_is_closed(myPtr, theWire.get(), theFlag ? 1 : 0));
  }

  /// @brief Sets the is-closed flag on a shell.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theShell is invalid, removed, or not a shell.
  /// @sa occtl_topo_set_shell_is_closed
  void set_shell_is_closed(const NodeId theShell, const bool theFlag)
  {
    check(::occtl_topo_set_shell_is_closed(myPtr, theShell.get(), theFlag ? 1 : 0));
  }

  /// @brief Sets the natural-restriction flag on a face.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theFace is invalid, removed, or not a face.
  /// @sa occtl_topo_set_face_natural_restriction
  void set_face_natural_restriction(const NodeId theFace, const bool theFlag)
  {
    check(::occtl_topo_set_face_natural_restriction(myPtr, theFace.get(), theFlag ? 1 : 0));
  }

  /// @brief Sets the parametric range of a coedge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  /// @sa occtl_topo_set_coedge_param_range
  void set_coedge_param_range(const NodeId theCoedge, const double theFirst, const double theLast)
  {
    check(::occtl_topo_set_coedge_param_range(myPtr, theCoedge.get(), theFirst, theLast));
  }

  /// @brief Sets the UV box of a coedge.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theCoedge is invalid, removed, or not a
  /// coedge.
  /// @sa occtl_topo_set_coedge_uv_box
  void set_coedge_uv_box(const NodeId            theCoedge,
                         const ::occtl_point2_t& theUvLo,
                         const ::occtl_point2_t& theUvHi)
  {
    check(::occtl_topo_set_coedge_uv_box(myPtr, theCoedge.get(), theUvLo, theUvHi));
  }

  /// @brief Deep-clones this graph into a new, independent graph.
  /// @return A new Graph instance wrapping the clone.
  /// @throws Error on failure (OCCTL_INVALID_ARGUMENT or OCCTL_OUT_OF_MEMORY).
  /// @sa occtl_graph_clone
  Graph clone() const
  {
    ::occtl_graph_t* aClone = nullptr;
    check(::occtl_graph_clone(myPtr, &aClone));
    return Graph(aClone);
  }

  /// @brief Compacts the graph, reclaiming slots from removed nodes.
  /// Invalidates all NodeId and RefId values.
  /// @throws Error on failure (OCCTL_INVALID_ARGUMENT).
  /// @sa occtl_graph_compact
  void compact() { check(::occtl_graph_compact(myPtr)); }

  /// @brief Removes a node and reparents children to a replacement.
  /// @throws Error with code OCCTL_NOT_FOUND when either ID is invalid or removed.
  /// @sa occtl_topo_remove_with_replacement
  void remove_with_replacement(const NodeId theNode, const NodeId theReplacement)
  {
    check(::occtl_topo_remove_with_replacement(myPtr, theNode.get(), theReplacement.get()));
  }

  /// @brief Removes a reference entry by its RefId.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theRefId is invalid or already removed.
  /// @sa occtl_topo_remove_ref
  void remove_ref(const RefId& theRefId) { check(::occtl_topo_remove_ref(myPtr, theRefId.get())); }

  /// @brief Removes a representation entry by its RepId.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theRepId is invalid or already removed.
  /// @sa occtl_topo_remove_rep
  void remove_rep(const RepId& theRepId) { check(::occtl_topo_remove_rep(myPtr, theRepId.get())); }

  /// @brief Cleans up stale references after removal operations. Idempotent.
  /// @throws Error on failure (OCCTL_INVALID_ARGUMENT).
  /// @sa occtl_topo_cleanup_removed_refs
  void cleanup_removed_refs() { check(::occtl_topo_cleanup_removed_refs(myPtr)); }

  /// @brief Rebinds an edge's start vertex to a different vertex.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_set_edge_start_vertex
  void set_edge_start_vertex(const NodeId theEdge, const NodeId theVertex)
  {
    check(::occtl_topo_set_edge_start_vertex(myPtr, theEdge.get(), theVertex.get()));
  }

  /// @brief Rebinds an edge's end vertex to a different vertex.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_set_edge_end_vertex
  void set_edge_end_vertex(const NodeId theEdge, const NodeId theVertex)
  {
    check(::occtl_topo_set_edge_end_vertex(myPtr, theEdge.get(), theVertex.get()));
  }

  /// @brief Sets the orientation of a reference entry.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_set_ref_orientation
  void set_ref_orientation(const RefId& theRefId, const Orientation theOrientation)
  {
    check(::occtl_topo_set_ref_orientation(myPtr, theRefId.get(), theOrientation));
  }

  /// @brief Sets the local location of a reference entry.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_set_ref_location
  void set_ref_location(const RefId& theRefId, const ::occtl_transform_t& theTransform)
  {
    check(::occtl_topo_set_ref_location(myPtr, theRefId.get(), theTransform));
  }

  /// @brief Sets the IsOuter flag on a wire reference.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_set_wire_ref_is_outer
  void set_wire_ref_is_outer(const RefId& theRefId, const bool theFlag)
  {
    check(::occtl_topo_set_wire_ref_is_outer(myPtr, theRefId.get(), theFlag ? 1 : 0));
  }

  /// @brief Sets a colour on a target node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_color_set
  void color_set(const NodeId theTarget, const ::occtl_color_rgba_t& theColor)
  {
    check(::occtl_graph_color_set(myPtr, theTarget.get(), theColor));
  }

  /// @brief Retrieves the colour of a target node, or opaque white if unset.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_color_get
  ::occtl_color_rgba_t color_get(const NodeId theTarget) const
  {
    ::occtl_color_rgba_t aColor{};
    check(::occtl_graph_color_get(myPtr, theTarget.get(), &aColor));
    return aColor;
  }

  /// @brief Removes the colour associated with a target node. Idempotent.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_color_unset
  void color_unset(const NodeId theTarget)
  {
    check(::occtl_graph_color_unset(myPtr, theTarget.get()));
  }

  /// @brief Lists explicit colour entries.
  /// @sa occtl_graph_color_entries
  std::vector<std::pair<NodeId, ::occtl_color_rgba_t>> color_entries() const
  {
    size_t aCount = 0;
    check(::occtl_graph_color_entries(myPtr, nullptr, nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t>    aNodes(aCount);
    std::vector<::occtl_color_rgba_t> aColors(aCount);
    if (aCount != 0)
    {
      check(::occtl_graph_color_entries(myPtr, aNodes.data(), aColors.data(), aCount, &aCount));
    }

    std::vector<std::pair<NodeId, ::occtl_color_rgba_t>> aEntries;
    aEntries.reserve(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      aEntries.emplace_back(NodeId(aNodes[anIndex]), aColors[anIndex]);
    }
    return aEntries;
  }

  /// @brief Sets a human-readable name on a target node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed,
  ///         OCCTL_INVALID_ARGUMENT when @p theName is null with non-zero length.
  /// @sa occtl_graph_name_set
  void name_set(const NodeId theTarget, const char* const theName, const size_t theNameLen)
  {
    check(::occtl_graph_name_set(myPtr, theTarget.get(), theName, theNameLen));
  }

  /// @brief Retrieves the name of a target node. Returns empty string if unset.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed,
  ///         OCCTL_BUFFER_TOO_SMALL if the internal buffer is too small (not used here).
  /// @sa occtl_graph_name_get
  std::string name_get(const NodeId theTarget) const
  {
    size_t aRequired = 0;
    check(::occtl_graph_name_get(myPtr, theTarget.get(), nullptr, 0, &aRequired));
    if (aRequired <= 1)
    {
      return {};
    }
    std::string aResult(aRequired - 1, '\0');
    check(::occtl_graph_name_get(myPtr,
                                 theTarget.get(),
                                 aResult.data(),
                                 aResult.size() + 1,
                                 &aRequired));
    return aResult;
  }

  /// @brief Lists nodes that have explicit names.
  /// @sa occtl_graph_name_nodes
  std::vector<NodeId> name_nodes() const
  {
    size_t aCount = 0;
    check(::occtl_graph_name_nodes(myPtr, nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (!aRaw.empty())
    {
      check(::occtl_graph_name_nodes(myPtr, aRaw.data(), aRaw.size(), &aCount));
    }

    std::vector<NodeId> aNodes;
    aNodes.reserve(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      aNodes.emplace_back(aRaw[anIndex]);
    }
    return aNodes;
  }

  /// @brief Sets material-lite data on a target node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_material_set
  void material_set(const NodeId theTarget, const MaterialInfo& theInfo)
  {
    check(::occtl_graph_material_set(myPtr, theTarget.get(), &theInfo));
  }

  /// @brief Retrieves material-lite data from a target node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget has no material.
  /// @sa occtl_graph_material_get
  GraphMaterial material_get(const NodeId theTarget) const
  {
    MaterialInfo anInfo    = OCCTL_MATERIAL_INFO_INIT;
    size_t       aRequired = 0;
    check(::occtl_graph_material_get(myPtr, theTarget.get(), &anInfo, nullptr, 0, &aRequired));

    GraphMaterial aMaterial;
    if (aRequired > 1)
    {
      aMaterial.name.assign(aRequired - 1, '\0');
      check(::occtl_graph_material_get(myPtr,
                                       theTarget.get(),
                                       &anInfo,
                                       aMaterial.name.data(),
                                       aMaterial.name.size() + 1,
                                       &aRequired));
    }
    else
    {
      char aName[1] = {};
      check(::occtl_graph_material_get(myPtr,
                                       theTarget.get(),
                                       &anInfo,
                                       aName,
                                       sizeof(aName),
                                       &aRequired));
      aMaterial.name.clear();
    }

    aMaterial.has_density       = anInfo.has_density;
    aMaterial.density           = anInfo.density;
    aMaterial.has_diffuse_color = anInfo.has_diffuse_color;
    aMaterial.diffuse_color     = anInfo.diffuse_color;
    aMaterial.metadata_uid      = anInfo.metadata_uid;
    return aMaterial;
  }

  /// @brief Removes material-lite data from a target node. Idempotent.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_material_unset
  void material_unset(const NodeId theTarget)
  {
    check(::occtl_graph_material_unset(myPtr, theTarget.get()));
  }

  /// @brief Lists nodes that have explicit material-lite data.
  /// @sa occtl_graph_material_nodes
  std::vector<NodeId> material_nodes() const
  {
    size_t aCount = 0;
    check(::occtl_graph_material_nodes(myPtr, nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (!aRaw.empty())
    {
      check(::occtl_graph_material_nodes(myPtr, aRaw.data(), aRaw.size(), &aCount));
    }

    std::vector<NodeId> aNodes;
    aNodes.reserve(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      aNodes.emplace_back(aRaw[anIndex]);
    }
    return aNodes;
  }

  /// @brief Sets graph-level length-unit metadata.
  /// @throws Error with code OCCTL_INVALID_ARGUMENT when the scale or name span is invalid.
  /// @sa occtl_graph_units_set
  void graph_units_set(const double      theLengthUnitToMeter,
                       const char* const theName,
                       const size_t      theNameLen)
  {
    check(::occtl_graph_units_set(myPtr, theLengthUnitToMeter, theName, theNameLen));
  }

  /// @brief Retrieves graph-level length-unit metadata.
  /// @throws Error on failure.
  /// @sa occtl_graph_units_get
  GraphUnits graph_units_get() const
  {
    GraphUnits aUnits;
    size_t     aRequired = 0;
    check(::occtl_graph_units_get(myPtr, &aUnits.length_unit_to_meter, nullptr, 0, &aRequired));
    if (aRequired <= 1)
    {
      aUnits.name.clear();
      return aUnits;
    }
    aUnits.name.assign(aRequired - 1, '\0');
    check(::occtl_graph_units_get(myPtr,
                                  &aUnits.length_unit_to_meter,
                                  aUnits.name.data(),
                                  aUnits.name.size() + 1,
                                  &aRequired));
    return aUnits;
  }

  /// @brief Sets UTF-8 metadata on a target node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_node_metadata_set
  void node_metadata_set(const NodeId      theTarget,
                         const char* const theKey,
                         const size_t      theKeyLen,
                         const char* const theValue,
                         const size_t      theValueLen)
  {
    check(::occtl_graph_node_metadata_set(myPtr,
                                          theTarget.get(),
                                          theKey,
                                          theKeyLen,
                                          theValue,
                                          theValueLen));
  }

  /// @brief Retrieves UTF-8 metadata from a target node.
  /// @throws Error with code OCCTL_NOT_FOUND when the target or key is missing.
  /// @sa occtl_graph_node_metadata_get
  std::string node_metadata_get(const NodeId      theTarget,
                                const char* const theKey,
                                const size_t      theKeyLen) const
  {
    size_t aRequired = 0;
    check(::occtl_graph_node_metadata_get(myPtr,
                                          theTarget.get(),
                                          theKey,
                                          theKeyLen,
                                          nullptr,
                                          0,
                                          &aRequired));
    if (aRequired <= 1)
    {
      return {};
    }
    std::string aResult(aRequired - 1, '\0');
    check(::occtl_graph_node_metadata_get(myPtr,
                                          theTarget.get(),
                                          theKey,
                                          theKeyLen,
                                          aResult.data(),
                                          aResult.size() + 1,
                                          &aRequired));
    return aResult;
  }

  /// @brief Lists UTF-8 metadata keys stored on a target node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_node_metadata_keys
  std::vector<std::string> node_metadata_keys(const NodeId theTarget) const
  {
    size_t aCount = 0;
    check(::occtl_graph_node_metadata_keys(myPtr, theTarget.get(), nullptr, 0, &aCount));

    std::vector<::occtl_metadata_key_view_t> aViews(aCount);
    if (!aViews.empty())
    {
      check(::occtl_graph_node_metadata_keys(myPtr,
                                             theTarget.get(),
                                             aViews.data(),
                                             aViews.size(),
                                             &aCount));
    }

    std::vector<std::string> aKeys;
    aKeys.reserve(aCount);
    for (const ::occtl_metadata_key_view_t& aView : aViews)
    {
      aKeys.emplace_back(aView.key, aView.key_len);
    }
    return aKeys;
  }

  /// @brief Lists nodes that have at least one metadata key.
  /// @sa occtl_graph_node_metadata_nodes
  std::vector<NodeId> node_metadata_nodes() const
  {
    size_t aCount = 0;
    check(::occtl_graph_node_metadata_nodes(myPtr, nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (!aRaw.empty())
    {
      check(::occtl_graph_node_metadata_nodes(myPtr, aRaw.data(), aRaw.size(), &aCount));
    }

    std::vector<NodeId> aNodes;
    aNodes.reserve(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      aNodes.emplace_back(aRaw[anIndex]);
    }
    return aNodes;
  }

  /// @brief Sets UTF-8 metadata on the graph itself.
  /// @throws Error with code OCCTL_INVALID_ARGUMENT when a span is invalid.
  /// @sa occtl_graph_metadata_set
  void graph_metadata_set(const char* const theKey,
                          const size_t      theKeyLen,
                          const char* const theValue,
                          const size_t      theValueLen)
  {
    check(::occtl_graph_metadata_set(myPtr, theKey, theKeyLen, theValue, theValueLen));
  }

  /// @brief Retrieves UTF-8 metadata from the graph itself.
  /// @throws Error with code OCCTL_NOT_FOUND when the key is missing.
  /// @sa occtl_graph_metadata_get
  std::string graph_metadata_get(const char* const theKey, const size_t theKeyLen) const
  {
    size_t aRequired = 0;
    check(::occtl_graph_metadata_get(myPtr, theKey, theKeyLen, nullptr, 0, &aRequired));
    if (aRequired <= 1)
    {
      return {};
    }
    std::string aResult(aRequired - 1, '\0');
    check(::occtl_graph_metadata_get(myPtr,
                                     theKey,
                                     theKeyLen,
                                     aResult.data(),
                                     aResult.size() + 1,
                                     &aRequired));
    return aResult;
  }

  /// @brief Lists UTF-8 metadata keys stored on the graph itself.
  /// @sa occtl_graph_metadata_keys
  std::vector<std::string> graph_metadata_keys() const
  {
    size_t aCount = 0;
    check(::occtl_graph_metadata_keys(myPtr, nullptr, 0, &aCount));

    std::vector<::occtl_metadata_key_view_t> aViews(aCount);
    if (!aViews.empty())
    {
      check(::occtl_graph_metadata_keys(myPtr, aViews.data(), aViews.size(), &aCount));
    }

    std::vector<std::string> aKeys;
    aKeys.reserve(aCount);
    for (const ::occtl_metadata_key_view_t& aView : aViews)
    {
      aKeys.emplace_back(aView.key, aView.key_len);
    }
    return aKeys;
  }

  /// @brief Removes one graph-level metadata key. Idempotent.
  /// @sa occtl_graph_metadata_unset
  void graph_metadata_unset(const char* const theKey, const size_t theKeyLen)
  {
    check(::occtl_graph_metadata_unset(myPtr, theKey, theKeyLen));
  }

  /// @brief Removes one metadata key from a target node. Idempotent.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_node_metadata_unset
  void node_metadata_unset(const NodeId theTarget, const char* const theKey, const size_t theKeyLen)
  {
    check(::occtl_graph_node_metadata_unset(myPtr, theTarget.get(), theKey, theKeyLen));
  }

  /// @brief Adds a UTF-8 tag to a target node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_tag_add
  void tag_add(const NodeId theTarget, const char* const theTag, const size_t theTagLen)
  {
    check(::occtl_graph_tag_add(myPtr, theTarget.get(), theTag, theTagLen));
  }

  /// @brief Removes a UTF-8 tag from a target node. Idempotent.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_tag_remove
  void tag_remove(const NodeId theTarget, const char* const theTag, const size_t theTagLen)
  {
    check(::occtl_graph_tag_remove(myPtr, theTarget.get(), theTag, theTagLen));
  }

  /// @brief Tests whether a target node carries a UTF-8 tag.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_tag_has
  bool tag_has(const NodeId theTarget, const char* const theTag, const size_t theTagLen) const
  {
    int32_t hasTag = 0;
    check(::occtl_graph_tag_has(myPtr, theTarget.get(), theTag, theTagLen, &hasTag));
    return hasTag != 0;
  }

  /// @brief Lists UTF-8 tags stored on a target node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theTarget is invalid or removed.
  /// @sa occtl_graph_tag_list
  std::vector<std::string> tag_list(const NodeId theTarget) const
  {
    size_t aCount = 0;
    check(::occtl_graph_tag_list(myPtr, theTarget.get(), nullptr, 0, &aCount));

    std::vector<::occtl_tag_view_t> aViews(aCount);
    if (!aViews.empty())
    {
      check(::occtl_graph_tag_list(myPtr, theTarget.get(), aViews.data(), aViews.size(), &aCount));
    }

    std::vector<std::string> aTags;
    aTags.reserve(aCount);
    for (const ::occtl_tag_view_t& aView : aViews)
    {
      aTags.emplace_back(aView.tag, aView.tag_len);
    }
    return aTags;
  }

  /// @brief Lists nodes that have tags, optionally filtered by exact tag.
  /// @sa occtl_graph_tag_nodes
  std::vector<NodeId> tag_nodes(const char* const theTag    = nullptr,
                                const size_t      theTagLen = 0) const
  {
    size_t aCount = 0;
    check(::occtl_graph_tag_nodes(myPtr, theTag, theTagLen, nullptr, 0, &aCount));

    std::vector<::occtl_node_id_t> aRaw(aCount);
    if (!aRaw.empty())
    {
      check(::occtl_graph_tag_nodes(myPtr, theTag, theTagLen, aRaw.data(), aRaw.size(), &aCount));
    }

    std::vector<NodeId> aNodes;
    aNodes.reserve(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      aNodes.emplace_back(aRaw[anIndex]);
    }
    return aNodes;
  }

  /// @brief Creates an assembly joint record in the graph.
  /// @return The new graph-local joint ID.
  /// @throws Error on failure.
  /// @sa occtl_joint_create
  JointId joint_create(const JointInfo& theInfo)
  {
    ::occtl_joint_id_t aJoint = OCCTL_JOINT_ID_INVALID;
    check(::occtl_joint_create(myPtr, &theInfo, &aJoint));
    return JointId(aJoint);
  }

  /// @brief Retrieves an assembly joint record.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theJoint is missing.
  /// @sa occtl_joint_get
  JointInfo joint_get(const JointId theJoint) const
  {
    JointInfo anInfo = OCCTL_JOINT_INFO_INIT;
    check(::occtl_joint_get(myPtr, theJoint.get(), &anInfo));
    return anInfo;
  }

  /// @brief Removes an assembly joint record. Missing joints are a no-op.
  /// @throws Error with code OCCTL_INVALID_ARGUMENT when @c theJoint is invalid.
  /// @sa occtl_joint_remove
  void joint_remove(const JointId theJoint) { check(::occtl_joint_remove(myPtr, theJoint.get())); }

  /// @brief Lists all assembly joints or those attached to @c theNode.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theNode is invalid or removed.
  /// @sa occtl_joint_list
  std::vector<JointId> joint_list(const NodeId theNode = NodeId::invalid()) const
  {
    size_t aCount = 0;
    check(::occtl_joint_list(myPtr, theNode.get(), nullptr, 0, &aCount));
    std::vector<::occtl_joint_id_t> aIds(aCount);
    if (aCount != 0)
    {
      check(::occtl_joint_list(myPtr, theNode.get(), aIds.data(), aIds.size(), &aCount));
    }
    std::vector<JointId> aResult;
    aResult.reserve(aIds.size());
    for (const ::occtl_joint_id_t& anId : aIds)
    {
      aResult.emplace_back(anId);
    }
    return aResult;
  }

  /// @brief Clears graph-owned computed data related to one node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theNode is invalid or removed.
  /// @sa occtl_graph_clear_cached
  void clear_cached(const NodeId theNode)
  {
    check(::occtl_graph_clear_cached(myPtr, theNode.get(), OCCTL_REF_ID_INVALID));
  }

  /// @brief Clears graph-owned computed data related to one reference.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theRef is invalid or removed.
  /// @sa occtl_graph_clear_cached
  void clear_cached(const RefId theRef)
  {
    check(::occtl_graph_clear_cached(myPtr, OCCTL_NODE_ID_INVALID, theRef.get()));
  }

  /// @brief Returns the computed axis-aligned bounding box for a node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theNode is invalid or removed.
  /// @sa occtl_graph_bbox_get
  ::occtl_select_bbox_t bbox_get(const NodeId theNode)
  {
    ::occtl_select_bbox_t aBox{};
    check(::occtl_graph_bbox_get(myPtr, theNode.get(), &aBox));
    return aBox;
  }

  /// @brief Returns the computed oriented bounding box for a node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theNode is invalid or removed.
  /// @sa occtl_graph_obb_get
  ::occtl_graph_obb_t obb_get(const NodeId theNode)
  {
    ::occtl_graph_obb_t aBox{};
    check(::occtl_graph_obb_get(myPtr, theNode.get(), &aBox));
    return aBox;
  }

  /// @brief Returns computed UV parameter bounds for a Face node.
  /// @throws Error with code OCCTL_WRONG_KIND when @c theFace is not a Face.
  /// @sa occtl_graph_face_uv_bounds_get
  ::occtl_graph_uv_bounds_t face_uv_bounds_get(const NodeId theFace)
  {
    ::occtl_graph_uv_bounds_t aBounds{};
    check(::occtl_graph_face_uv_bounds_get(myPtr, theFace.get(), &aBounds));
    return aBounds;
  }

  /// @brief Returns one computed OCCT mass-property scalar.
  /// @throws Error with code OCCTL_WRONG_KIND when @c theKind does not apply.
  /// @sa occtl_graph_measure_get
  double measure_get(const NodeId theNode, const SelectMeasureKind theKind)
  {
    double aValue = 0.0;
    check(::occtl_graph_measure_get(myPtr, theNode.get(), theKind, &aValue));
    return aValue;
  }

  /// @brief Returns computed combined OCCT mass properties for a node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theNode is invalid or removed.
  /// @sa occtl_graph_mass_properties_get
  ::occtl_graph_mass_properties_t mass_properties_get(const NodeId theNode)
  {
    ::occtl_graph_mass_properties_t aProperties{};
    check(::occtl_graph_mass_properties_get(myPtr, theNode.get(), &aProperties));
    return aProperties;
  }

  /// @brief Returns computed curve-kind classification for an Edge node.
  /// @throws Error with code OCCTL_WRONG_KIND when @c theEdge is not an Edge.
  /// @sa occtl_graph_edge_curve_kind_get
  CurveKind edge_curve_kind_get(const NodeId theEdge)
  {
    CurveKind aKind = OCCTL_CURVE_KIND_UNDEFINED;
    check(::occtl_graph_edge_curve_kind_get(myPtr, theEdge.get(), &aKind));
    return aKind;
  }

  /// @brief Returns computed surface-kind classification for a Face node.
  /// @throws Error with code OCCTL_WRONG_KIND when @c theFace is not a Face.
  /// @sa occtl_graph_face_surface_kind_get
  SurfaceKind face_surface_kind_get(const NodeId theFace)
  {
    SurfaceKind aKind = OCCTL_SURFACE_KIND_UNDEFINED;
    check(::occtl_graph_face_surface_kind_get(myPtr, theFace.get(), &aKind));
    return aKind;
  }

  /// @brief Returns computed descendant Vertex nodes for a root node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theNode is invalid or removed.
  /// @sa occtl_graph_descendant_vertices_get
  std::vector<NodeId> descendant_vertices_get(const NodeId theNode)
  {
    size_t aCount = 0;
    check(::occtl_graph_descendant_vertices_get(myPtr, theNode.get(), nullptr, 0, &aCount));
    std::vector<::occtl_node_id_t> aIds(aCount);
    if (aCount != 0)
    {
      check(::occtl_graph_descendant_vertices_get(myPtr,
                                                  theNode.get(),
                                                  aIds.data(),
                                                  aIds.size(),
                                                  &aCount));
    }
    std::vector<NodeId> aResult;
    aResult.reserve(aIds.size());
    for (const ::occtl_node_id_t& anId : aIds)
    {
      aResult.emplace_back(anId);
    }
    return aResult;
  }

  /// @brief Returns computed descendant Edge nodes for a root node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theNode is invalid or removed.
  /// @sa occtl_graph_descendant_edges_get
  std::vector<NodeId> descendant_edges_get(const NodeId theNode)
  {
    size_t aCount = 0;
    check(::occtl_graph_descendant_edges_get(myPtr, theNode.get(), nullptr, 0, &aCount));
    std::vector<::occtl_node_id_t> aIds(aCount);
    if (aCount != 0)
    {
      check(::occtl_graph_descendant_edges_get(myPtr,
                                               theNode.get(),
                                               aIds.data(),
                                               aIds.size(),
                                               &aCount));
    }
    std::vector<NodeId> aResult;
    aResult.reserve(aIds.size());
    for (const ::occtl_node_id_t& anId : aIds)
    {
      aResult.emplace_back(anId);
    }
    return aResult;
  }

  /// @brief Returns computed descendant Face nodes for a root node.
  /// @throws Error with code OCCTL_NOT_FOUND when @c theNode is invalid or removed.
  /// @sa occtl_graph_descendant_faces_get
  std::vector<NodeId> descendant_faces_get(const NodeId theNode)
  {
    size_t aCount = 0;
    check(::occtl_graph_descendant_faces_get(myPtr, theNode.get(), nullptr, 0, &aCount));
    std::vector<::occtl_node_id_t> aIds(aCount);
    if (aCount != 0)
    {
      check(::occtl_graph_descendant_faces_get(myPtr,
                                               theNode.get(),
                                               aIds.data(),
                                               aIds.size(),
                                               &aCount));
    }
    std::vector<NodeId> aResult;
    aResult.reserve(aIds.size());
    for (const ::occtl_node_id_t& anId : aIds)
    {
      aResult.emplace_back(anId);
    }
    return aResult;
  }

  /// @brief Returns computed descendant nodes of one requested kind.
  /// @throws Error with code OCCTL_INVALID_ARGUMENT when @c theKind is invalid.
  /// @sa occtl_graph_descendants_get
  std::vector<NodeId> descendants_get(const NodeId theNode, const NodeKind theKind)
  {
    size_t aCount = 0;
    check(::occtl_graph_descendants_get(myPtr, theNode.get(), theKind, nullptr, 0, &aCount));
    std::vector<::occtl_node_id_t> aIds(aCount);
    if (aCount != 0)
    {
      check(::occtl_graph_descendants_get(myPtr,
                                          theNode.get(),
                                          theKind,
                                          aIds.data(),
                                          aIds.size(),
                                          &aCount));
    }
    std::vector<NodeId> aResult;
    aResult.reserve(aIds.size());
    for (const ::occtl_node_id_t& anId : aIds)
    {
      aResult.emplace_back(anId);
    }
    return aResult;
  }

  /// @brief Returns computed adjacent Face nodes for a Face node.
  /// @throws Error with code OCCTL_WRONG_KIND when @c theFace is not a Face.
  /// @sa occtl_graph_adjacent_faces_get
  std::vector<NodeId> adjacent_faces_get(const NodeId theFace)
  {
    size_t aCount = 0;
    check(::occtl_graph_adjacent_faces_get(myPtr, theFace.get(), nullptr, 0, &aCount));
    std::vector<::occtl_node_id_t> aIds(aCount);
    if (aCount != 0)
    {
      check(
        ::occtl_graph_adjacent_faces_get(myPtr, theFace.get(), aIds.data(), aIds.size(), &aCount));
    }
    std::vector<NodeId> aResult;
    aResult.reserve(aIds.size());
    for (const ::occtl_node_id_t& anId : aIds)
    {
      aResult.emplace_back(anId);
    }
    return aResult;
  }

  /// @brief Returns computed adjacent Edge nodes for an Edge node.
  /// @throws Error with code OCCTL_WRONG_KIND when @c theEdge is not an Edge.
  /// @sa occtl_graph_adjacent_edges_get
  std::vector<NodeId> adjacent_edges_get(const NodeId theEdge)
  {
    size_t aCount = 0;
    check(::occtl_graph_adjacent_edges_get(myPtr, theEdge.get(), nullptr, 0, &aCount));
    std::vector<::occtl_node_id_t> aIds(aCount);
    if (aCount != 0)
    {
      check(
        ::occtl_graph_adjacent_edges_get(myPtr, theEdge.get(), aIds.data(), aIds.size(), &aCount));
    }
    std::vector<NodeId> aResult;
    aResult.reserve(aIds.size());
    for (const ::occtl_node_id_t& anId : aIds)
    {
      aResult.emplace_back(anId);
    }
    return aResult;
  }

  /// @brief Returns computed OCCT minimum distance between two nodes.
  /// @throws Error with code OCCTL_NOT_FOUND when either node is invalid or removed.
  /// @sa occtl_graph_pair_distance_get
  double pair_distance_get(const NodeId theFirst, const NodeId theSecond)
  {
    double aDistance = 0.0;
    check(::occtl_graph_pair_distance_get(myPtr, theFirst.get(), theSecond.get(), &aDistance));
    return aDistance;
  }

  /// @brief Creates a product in the graph.
  /// @return The new product NodeId.
  /// @throws Error on failure.
  /// @sa occtl_topo_make_product
  NodeId make_product(const ::occtl_topo_make_product_info_t& theInfo)
  {
    ::occtl_node_id_t anId;
    check(::occtl_topo_make_product(myPtr, &theInfo, &anId));
    return NodeId(anId);
  }

  /// @brief Links an existing product to a topology root with a placement.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_link_product
  void link_product_to_topology(const NodeId               theProduct,
                                const NodeId               theRoot,
                                const ::occtl_transform_t& thePlacement)
  {
    check(::occtl_topo_link_product(myPtr, theProduct.get(), theRoot.get(), thePlacement));
  }

  /// @brief Links an existing product to a topology root and returns the new occurrence.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_link_product_occurrence
  NodeId link_product_to_topology_with_occurrence(const NodeId               theProduct,
                                                  const NodeId               theRoot,
                                                  const ::occtl_transform_t& thePlacement)
  {
    ::occtl_node_id_t anOccurrence{};
    check(::occtl_topo_link_product_occurrence(myPtr,
                                               theProduct.get(),
                                               theRoot.get(),
                                               thePlacement,
                                               &anOccurrence));
    return NodeId(anOccurrence);
  }

  /// @brief Links two products via a parent-child occurrence.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_link_products
  void link_products(const NodeId               theParentProduct,
                     const NodeId               theChildProduct,
                     const ::occtl_transform_t& thePlacement,
                     const NodeId               theParentOccurrence = NodeId(OCCTL_NODE_ID_INVALID))
  {
    check(::occtl_topo_link_products(myPtr,
                                     theParentProduct.get(),
                                     theChildProduct.get(),
                                     thePlacement,
                                     theParentOccurrence.get()));
  }

  /// @brief Links two products and returns the new occurrence.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_link_products_occurrence
  NodeId link_products_with_occurrence(
    const NodeId               theParentProduct,
    const NodeId               theChildProduct,
    const ::occtl_transform_t& thePlacement,
    const NodeId               theParentOccurrence = NodeId(OCCTL_NODE_ID_INVALID))
  {
    ::occtl_node_id_t anOccurrence{};
    check(::occtl_topo_link_products_occurrence(myPtr,
                                                theParentProduct.get(),
                                                theChildProduct.get(),
                                                thePlacement,
                                                theParentOccurrence.get(),
                                                &anOccurrence));
    return NodeId(anOccurrence);
  }

  /// @brief Removes an occurrence reference from the graph.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_remove_occurrence
  void remove_occurrence(const RefId& theOccurrenceRef)
  {
    check(::occtl_topo_remove_occurrence(myPtr, theOccurrenceRef.get()));
  }

  /// @brief Sets the local transform of a uniquely referenced occurrence node.
  /// @throws Error with code OCCTL_NOT_FOUND, OCCTL_WRONG_KIND, or OCCTL_INVALID_ARGUMENT.
  /// @sa occtl_topo_occurrence_set_transform
  void occurrence_transform_set(const NodeId theOccurrence, const ::occtl_transform_t& theTransform)
  {
    check(::occtl_topo_occurrence_set_transform(myPtr, theOccurrence.get(), theTransform));
  }

  /// @brief Returns the local transform of a uniquely referenced occurrence node.
  /// @throws Error with code OCCTL_NOT_FOUND, OCCTL_WRONG_KIND, or OCCTL_INVALID_ARGUMENT.
  /// @sa occtl_topo_occurrence_transform
  ::occtl_transform_t occurrence_transform_get(const NodeId theOccurrence) const
  {
    ::occtl_transform_t aTransform;
    check(::occtl_topo_occurrence_transform(myPtr, theOccurrence.get(), &aTransform));
    return aTransform;
  }

  /// @brief Returns an occurrence transform accumulated from a traversal root.
  /// @throws Error with code OCCTL_NOT_FOUND or OCCTL_WRONG_KIND.
  /// @sa occtl_topo_occurrence_world_transform
  ::occtl_transform_t occurrence_world_transform(const NodeId theRoot,
                                                 const NodeId theOccurrence) const
  {
    ::occtl_transform_t aTransform;
    check(::occtl_topo_occurrence_world_transform(myPtr,
                                                  theRoot.get(),
                                                  theOccurrence.get(),
                                                  &aTransform));
    return aTransform;
  }

  /// @brief Adds a face to a shell with the given orientation.
  /// @sa occtl_topo_shell_add_face
  void shell_add_face(const NodeId                theShell,
                      const NodeId                theFace,
                      const ::occtl_orientation_t theOrientation)
  {
    check(::occtl_topo_shell_add_face(myPtr, theShell.get(), theFace.get(), theOrientation));
  }

  /// @brief Removes a face from a shell (the face definition remains in the graph).
  /// @sa occtl_topo_shell_remove_face
  void shell_remove_face(const NodeId theShell, const NodeId theFace)
  {
    check(::occtl_topo_shell_remove_face(myPtr, theShell.get(), theFace.get()));
  }

  /// @brief Adds selected wires as inner wires of a face.
  /// @sa occtl_topo_face_add_holes
  void face_add_holes(const NodeId theFace, const std::vector<NodeId>& theHoles)
  {
    std::vector<::occtl_node_id_t> aRaw;
    aRaw.reserve(theHoles.size());
    for (const NodeId& aHole : theHoles)
    {
      aRaw.push_back(aHole.get());
    }
    check(::occtl_topo_face_add_holes(myPtr,
                                      theFace.get(),
                                      aRaw.empty() ? nullptr : aRaw.data(),
                                      aRaw.size()));
  }

  /// @brief Removes all inner wires from a face.
  /// @sa occtl_topo_face_remove_holes
  void face_remove_holes(const NodeId theFace)
  {
    check(::occtl_topo_face_remove_holes(myPtr, theFace.get(), nullptr, 0));
  }

  /// @brief Removes selected inner wires from a face.
  /// @sa occtl_topo_face_remove_holes
  void face_remove_holes(const NodeId theFace, const std::vector<NodeId>& theHoles)
  {
    std::vector<::occtl_node_id_t> aRaw;
    aRaw.reserve(theHoles.size());
    for (const NodeId& aHole : theHoles)
    {
      aRaw.push_back(aHole.get());
    }
    check(::occtl_topo_face_remove_holes(myPtr,
                                         theFace.get(),
                                         aRaw.empty() ? nullptr : aRaw.data(),
                                         aRaw.size()));
  }

  /// @brief Adds a shell to a solid with the given orientation.
  /// @sa occtl_topo_solid_add_shell
  void solid_add_shell(const NodeId                theSolid,
                       const NodeId                theShell,
                       const ::occtl_orientation_t theOrientation)
  {
    check(::occtl_topo_solid_add_shell(myPtr, theSolid.get(), theShell.get(), theOrientation));
  }

  /// @brief Removes a shell from a solid (the shell definition remains in the graph).
  /// @sa occtl_topo_solid_remove_shell
  void solid_remove_shell(const NodeId theSolid, const NodeId theShell)
  {
    check(::occtl_topo_solid_remove_shell(myPtr, theSolid.get(), theShell.get()));
  }

  /// @brief Adds a child entity to a compound with the given orientation.
  /// @sa occtl_topo_compound_add_child
  void compound_add_child(const NodeId                theCompound,
                          const NodeId                theChild,
                          const ::occtl_orientation_t theOrientation)
  {
    check(
      ::occtl_topo_compound_add_child(myPtr, theCompound.get(), theChild.get(), theOrientation));
  }

  /// @brief Removes a child from a compound (the child definition remains in the graph).
  /// @sa occtl_topo_compound_remove_child
  void compound_remove_child(const NodeId theCompound, const NodeId theChild)
  {
    check(::occtl_topo_compound_remove_child(myPtr, theCompound.get(), theChild.get()));
  }

  /// @brief One yield from #edge_split: the two sub-edges produced at the split point.
  struct EdgeSplitResult
  {
    NodeId Edge1; ///< First sub-edge (start → split parameter).
    NodeId Edge2; ///< Second sub-edge (split parameter → end).
  };

  /// @brief Splits an edge at the given parameter and returns the two new sub-edges.
  /// The original edge is soft-removed; wires referencing it are updated.
  /// @sa occtl_topo_edge_split
  EdgeSplitResult edge_split(const NodeId theEdge, const double theParameter)
  {
    ::occtl_node_id_t aE1{};
    ::occtl_node_id_t aE2{};
    check(::occtl_topo_edge_split(myPtr, theEdge.get(), theParameter, &aE1, &aE2));
    return {NodeId(aE1), NodeId(aE2)};
  }

  /// @brief Adds an internal vertex to an edge.
  /// @sa occtl_topo_edge_add_internal_vertex
  void edge_add_internal_vertex(const NodeId theEdge, const NodeId theVertex)
  {
    check(::occtl_topo_edge_add_internal_vertex(myPtr, theEdge.get(), theVertex.get()));
  }

  /// @brief Removes a vertex (boundary or internal) from an edge.
  /// @sa occtl_topo_edge_remove_vertex
  void edge_remove_vertex(const NodeId theEdge, const NodeId theVertex)
  {
    check(::occtl_topo_edge_remove_vertex(myPtr, theEdge.get(), theVertex.get()));
  }

  /// @brief Replaces an edge's 3D curve.
  /// @sa occtl_topo_replace_edge_curve
  void replace_edge_curve(const NodeId theEdge, const ::occtl_rep_id_t theCurveId)
  {
    check(::occtl_topo_replace_edge_curve(myPtr, theEdge.get(), theCurveId));
  }

  /// @brief Replaces a face's surface.
  /// @sa occtl_topo_replace_face_surface
  void replace_face_surface(const NodeId theFace, const ::occtl_rep_id_t theSurfaceId)
  {
    check(::occtl_topo_replace_face_surface(myPtr, theFace.get(), theSurfaceId));
  }

  /// @brief Replaces a coedge's pcurve.
  /// @sa occtl_topo_replace_coedge_pcurve
  void replace_coedge_pcurve(const NodeId theCoedge, const ::occtl_rep_id_t thePcurveId)
  {
    check(::occtl_topo_replace_coedge_pcurve(myPtr, theCoedge.get(), thePcurveId));
  }

  /// @brief Adds a pcurve binding between an edge and a face, creating a new coedge.
  /// @sa occtl_topo_add_pcurve
  void add_pcurve(const NodeId                theEdge,
                  const NodeId                theFace,
                  const ::occtl_rep_id_t      thePcurveId,
                  const double                theFirst,
                  const double                theLast,
                  const ::occtl_orientation_t theOrientation)
  {
    check(::occtl_topo_add_pcurve(myPtr,
                                  theEdge.get(),
                                  theFace.get(),
                                  thePcurveId,
                                  theFirst,
                                  theLast,
                                  theOrientation));
  }

  /// @brief Returns the geometric continuity between two faces at their shared edge.
  /// @sa occtl_topo_edge_continuity
  ::occtl_shape_continuity_t edge_continuity(const NodeId theEdge,
                                             const NodeId theFaceA,
                                             const NodeId theFaceB) const
  {
    ::occtl_shape_continuity_t aCont{};
    check(
      ::occtl_topo_edge_continuity(myPtr, theEdge.get(), theFaceA.get(), theFaceB.get(), &aCont));
    return aCont;
  }

  /// @brief Returns true if continuity is recorded between two faces at the edge.
  /// @sa occtl_topo_edge_has_continuity
  bool edge_has_continuity(const NodeId theEdge, const NodeId theFaceA, const NodeId theFaceB) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_edge_has_continuity(myPtr,
                                           theEdge.get(),
                                           theFaceA.get(),
                                           theFaceB.get(),
                                           &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns the maximum continuity across all face pairs at the edge.
  /// @sa occtl_topo_edge_max_continuity
  ::occtl_shape_continuity_t edge_max_continuity(const NodeId theEdge) const
  {
    ::occtl_shape_continuity_t aCont{};
    check(::occtl_topo_edge_max_continuity(myPtr, theEdge.get(), &aCont));
    return aCont;
  }

  /// @brief Returns the full TopAbs orientation of a coedge.
  /// @sa occtl_topo_coedge_orientation
  ::occtl_orientation_t coedge_orientation(const NodeId theCoedge) const
  {
    ::occtl_orientation_t anOri{};
    check(::occtl_topo_coedge_orientation(myPtr, theCoedge.get(), &anOri));
    return anOri;
  }

  /// @brief Finds the coedge for an (edge, face) pair.
  /// @sa occtl_topo_edge_find_coedge_on_face
  NodeId edge_find_coedge_on_face(const NodeId theEdge, const NodeId theFace) const
  {
    ::occtl_node_id_t aCoedge{};
    check(::occtl_topo_edge_find_coedge_on_face(myPtr, theEdge.get(), theFace.get(), &aCoedge));
    return NodeId(aCoedge);
  }

  /// @brief Finds the coedge for an (edge, face, orientation) triple. Disambiguates seam edges.
  /// @sa occtl_topo_edge_find_coedge_on_face_oriented
  NodeId edge_find_coedge_on_face_oriented(const NodeId                theEdge,
                                           const NodeId                theFace,
                                           const ::occtl_orientation_t theOrientation) const
  {
    ::occtl_node_id_t aCoedge{};
    check(::occtl_topo_edge_find_coedge_on_face_oriented(myPtr,
                                                         theEdge.get(),
                                                         theFace.get(),
                                                         theOrientation,
                                                         &aCoedge));
    return NodeId(aCoedge);
  }

  /// @brief Returns the vertex point with a parent's accumulated Location applied.
  /// @sa occtl_topo_vertex_point_in_usage
  ::occtl_point3_t vertex_point_in_usage(const NodeId theVertex, const NodeId theParent) const
  {
    ::occtl_point3_t aPoint{};
    check(::occtl_topo_vertex_point_in_usage(myPtr, theVertex.get(), theParent.get(), &aPoint));
    return aPoint;
  }

  /// @brief Returns the 2D parameter of a vertex on a coedge's pcurve.
  /// @sa occtl_topo_vertex_pcurve_parameter
  double vertex_pcurve_parameter(const NodeId theVertex, const NodeId theCoedge) const
  {
    double aU = 0.0;
    check(::occtl_topo_vertex_pcurve_parameter(myPtr, theVertex.get(), theCoedge.get(), &aU));
    return aU;
  }

  /// @brief Returns true if the edge has a 3D polygon discretization.
  /// @sa occtl_topo_edge_has_polygon3d
  bool edge_has_polygon3d(const NodeId theEdge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_edge_has_polygon3d(myPtr, theEdge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Returns true if the coedge has a polygon-on-surface discretization.
  /// @sa occtl_topo_coedge_has_polygon_on_surface
  bool coedge_has_polygon_on_surface(const NodeId theCoedge) const
  {
    int32_t aFlag = 0;
    check(::occtl_topo_coedge_has_polygon_on_surface(myPtr, theCoedge.get(), &aFlag));
    return aFlag != 0;
  }

  /// @brief Evaluates a face surface with its first partials inside a restricted UV box.
  /// @sa occtl_topo_face_uv_bounds_restricted
  void face_uv_bounds_restricted(const NodeId       theFace,
                                 const double       theUMin,
                                 const double       theUMax,
                                 const double       theVMin,
                                 const double       theVMax,
                                 const double       theU,
                                 const double       theV,
                                 ::occtl_point3_t&  theOutPoint,
                                 ::occtl_vector3_t& theOutD1U,
                                 ::occtl_vector3_t& theOutD1V) const
  {
    check(::occtl_topo_face_uv_bounds_restricted(myPtr,
                                                 theFace.get(),
                                                 theUMin,
                                                 theUMax,
                                                 theVMin,
                                                 theVMax,
                                                 theU,
                                                 theV,
                                                 &theOutPoint,
                                                 &theOutD1U,
                                                 &theOutD1V));
  }

  /// @brief Applies a 3D fillet or chamfer to all solids and shells in this graph.
  /// @param theOpts Fillet/chamfer options (radius, chamfer mode, distances).
  /// @return A pair of (new Graph, root NodeId of the filleted result).
  /// @throws Error on failure (invalid argument, version mismatch, or geometric failure).
  /// @sa occtl_topo_fillet
  std::pair<Graph, NodeId> fillet(const ::occtl_topo_fillet_options_t& theOpts)
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_fillet(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Applies a selected-edge 3D fillet or chamfer to @p theRoot.
  /// @param theRoot  Shape root to modify.
  /// @param theEdges Selected Edge NodeIds.
  /// @param theOpts  Blend options; @c root / @c edges fields are overwritten.
  /// @return A pair of (new Graph, root NodeId of the blended result).
  /// @throws Error on failure.
  /// @sa occtl_topo_blend_edges
  std::pair<Graph, NodeId> blend_edges(const NodeId                             theRoot,
                                       const std::vector<NodeId>&               theEdges,
                                       const ::occtl_topo_edge_blend_options_t& theOpts) const
  {
    std::vector<::occtl_node_id_t> aEdges;
    aEdges.reserve(theEdges.size());
    for (const NodeId& anEdge : theEdges)
    {
      aEdges.push_back(anEdge.get());
    }

    ::occtl_topo_edge_blend_options_t aOpts = theOpts;
    aOpts.root                              = theRoot.get();
    aOpts.edges                             = aEdges.data();
    aOpts.edge_count                        = aEdges.size();

    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_blend_edges(myPtr, &aOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Applies a selected-edge constant-radius 3D fillet to @p theRoot.
  /// @throws Error on failure.
  std::pair<Graph, NodeId> fillet_edges(const NodeId               theRoot,
                                        const std::vector<NodeId>& theEdges,
                                        const double               theRadius) const
  {
    ::occtl_topo_edge_blend_options_t aOpts = OCCTL_TOPO_EDGE_BLEND_OPTIONS_INIT;
    aOpts.radius                            = theRadius;
    aOpts.chamfer_mode                      = 0;
    return blend_edges(theRoot, theEdges, aOpts);
  }

  /// @brief Applies a selected-edge 3D chamfer to @p theRoot.
  /// @throws Error on failure.
  std::pair<Graph, NodeId> chamfer_edges(const NodeId               theRoot,
                                         const std::vector<NodeId>& theEdges,
                                         const double               theDist1,
                                         const double               theDist2) const
  {
    ::occtl_topo_edge_blend_options_t aOpts = OCCTL_TOPO_EDGE_BLEND_OPTIONS_INIT;
    aOpts.chamfer_mode                      = 1;
    aOpts.chamfer_dist1                     = theDist1;
    aOpts.chamfer_dist2                     = theDist2;
    return blend_edges(theRoot, theEdges, aOpts);
  }

  /// @brief Estimates the largest selected-edge constant fillet radius accepted by OCCT.
  /// @throws Error on failure.
  /// @sa occtl_topo_max_fillet_radius
  double max_fillet_radius(const NodeId                                    theRoot,
                           const std::vector<NodeId>&                      theEdges,
                           const ::occtl_topo_max_fillet_radius_options_t& theOpts) const
  {
    std::vector<::occtl_node_id_t> aEdges;
    aEdges.reserve(theEdges.size());
    for (const NodeId& anEdge : theEdges)
    {
      aEdges.push_back(anEdge.get());
    }

    ::occtl_topo_max_fillet_radius_options_t aOpts = theOpts;
    aOpts.root                                     = theRoot.get();
    aOpts.edges                                    = aEdges.data();
    aOpts.edge_count                               = aEdges.size();

    double aRadius = 0.0;
    check(::occtl_topo_max_fillet_radius(myPtr, &aOpts, &aRadius));
    return aRadius;
  }

  /// @brief Creates a transformed copy of the shape rooted at @p theRoot.
  /// @throws Error on failure.
  /// @sa occtl_topo_transformed
  std::pair<Graph, NodeId> transformed(const NodeId theRoot, const Transform& theTransform) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(
      ::occtl_topo_transformed(myPtr, theRoot.get(), theTransform.c_type(), &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Projects an edge or wire onto a target face along face normals.
  /// @throws Error on failure.
  /// @sa occtl_topo_project_on_face
  std::pair<Graph, NodeId> project_on_face(
    const ::occtl_topo_project_on_face_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_project_on_face(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Wraps a planar edge, wire, or face onto a target face.
  /// @throws Error on failure.
  /// @sa occtl_topo_wrap_on_face
  std::pair<Graph, NodeId> wrap_on_face(const ::occtl_topo_wrap_on_face_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_wrap_on_face(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Projects a face onto target boundary faces along a fixed direction.
  /// @throws Error on failure.
  /// @sa occtl_topo_project_face_along_direction
  std::pair<Graph, NodeId> project_face_along_direction(
    const ::occtl_topo_project_face_direction_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_project_face_along_direction(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Converts planar face or wire boundaries to line and circular-arc edges.
  /// @throws Error on failure.
  /// @sa occtl_topo_face_to_arcs
  std::pair<Graph, NodeId> face_to_arcs(const ::occtl_topo_face_to_arcs_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_face_to_arcs(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Projects a shape with OCCT hidden-line removal.
  /// @return Pair of (output Graph, category roots in that graph).
  /// @throws Error on failure.
  /// @sa occtl_topo_make_hlr_projection
  std::pair<Graph, HlrCategoryRoots> hlr_project(const ::occtl_topo_hlr_options_t& theOpts) const
  {
    ::occtl_topo_hlr_result_t aResult = OCCTL_TOPO_HLR_RESULT_INIT;
    check(::occtl_topo_make_hlr_projection(myPtr, &theOpts, &aResult));
    HlrCategoryRoots aRoots;
    aRoots.visible_sharp   = NodeId(aResult.visible_sharp);
    aRoots.visible_smooth  = NodeId(aResult.visible_smooth);
    aRoots.visible_seam    = NodeId(aResult.visible_seam);
    aRoots.visible_outline = NodeId(aResult.visible_outline);
    aRoots.hidden_sharp    = NodeId(aResult.hidden_sharp);
    aRoots.hidden_smooth   = NodeId(aResult.hidden_smooth);
    aRoots.hidden_seam     = NodeId(aResult.hidden_seam);
    aRoots.hidden_outline  = NodeId(aResult.hidden_outline);
    return {Graph(aResult.graph), aRoots};
  }

  /// @brief Applies a draft angle to selected faces of a shape.
  /// @throws Error on failure.
  /// @sa occtl_topo_draft_faces
  std::pair<Graph, NodeId> draft_faces(const ::occtl_topo_draft_faces_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_draft_faces(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Removes selected features from @p theRoot.
  /// @param theRoot  Shape root to modify.
  /// @param theSelections Selected feature topology NodeIds.
  /// @param theParallel Whether OCCT may run the operation in parallel.
  /// @return A pair of (new Graph, root NodeId of the defeatured result).
  /// @throws Error on failure.
  /// @sa occtl_topo_remove_features
  std::pair<Graph, NodeId> remove_features(const NodeId               theRoot,
                                           const std::vector<NodeId>& theSelections,
                                           const bool                 theParallel = false) const
  {
    std::vector<::occtl_node_id_t> aSelections;
    aSelections.reserve(theSelections.size());
    for (const NodeId& aSelection : theSelections)
    {
      aSelections.push_back(aSelection.get());
    }

    ::occtl_topo_defeature_options_t aOpts = OCCTL_TOPO_DEFEATURE_OPTIONS_INIT;
    aOpts.root                             = theRoot.get();
    aOpts.selections                       = aSelections.data();
    aOpts.selection_count                  = aSelections.size();
    aOpts.parallel                         = theParallel ? 1 : 0;

    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_defeature(myPtr, &aOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Removes selected features using a fully specified C options struct.
  /// @throws Error on failure.
  /// @sa occtl_topo_remove_features
  std::pair<Graph, NodeId> remove_features(const ::occtl_topo_defeature_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_defeature(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Offsets selected features of @p theRoot.
  /// @param theRoot Shape root to offset.
  /// @param theSelections Selected feature topology NodeIds.
  /// @param theSelectionOffset Offset applied to selected faces.
  /// @param theBaseOffset Offset applied to unselected faces.
  /// @return A pair of (new Graph, root NodeId of the offset result).
  /// @throws Error on failure.
  /// @sa occtl_topo_offset_features
  std::pair<Graph, NodeId> offset_features(const NodeId               theRoot,
                                           const std::vector<NodeId>& theSelections,
                                           const double               theSelectionOffset,
                                           const double               theBaseOffset = 0.0) const
  {
    std::vector<::occtl_node_id_t> aSelections;
    aSelections.reserve(theSelections.size());
    for (const NodeId& aSelection : theSelections)
    {
      aSelections.push_back(aSelection.get());
    }

    ::occtl_topo_offset_features_options_t aOpts = OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT;
    aOpts.root                                   = theRoot.get();
    aOpts.selections                             = aSelections.data();
    aOpts.selection_count                        = aSelections.size();
    aOpts.selection_offset                       = theSelectionOffset;
    aOpts.base_offset                            = theBaseOffset;

    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_make_offset_features(myPtr, &aOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Offsets selected features using a fully specified C options struct.
  /// @throws Error on failure.
  /// @sa occtl_topo_offset_features
  std::pair<Graph, NodeId> offset_features(
    const ::occtl_topo_offset_features_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_make_offset_features(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Builds an N-side filling face from ordered boundary edges.
  /// @throws Error on failure.
  /// @sa occtl_topo_make_filling
  std::pair<Graph, NodeId> make_filling(const ::occtl_topo_filling_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_make_filling(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Builds a filling patch from edge, support-face, and point constraints.
  /// @throws Error on failure.
  /// @sa occtl_topo_make_filling_patch
  std::pair<Graph, NodeId> make_filling_patch(
    const ::occtl_topo_filling_patch_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_make_filling_patch(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Creates a translated copy of the shape rooted at @p theRoot.
  std::pair<Graph, NodeId> translated(const NodeId theRoot, const Vector3& theDelta) const
  {
    return transformed(theRoot, Transform::translation(theDelta));
  }

  /// @brief Creates a rotated copy of the shape rooted at @p theRoot.
  std::pair<Graph, NodeId> rotated(const NodeId          theRoot,
                                   const Axis1Placement& theAxis,
                                   const double          theAngle) const
  {
    return transformed(theRoot, Transform::rotation(theAxis, theAngle));
  }

  /// @brief Creates a uniformly scaled copy of the shape rooted at @p theRoot.
  std::pair<Graph, NodeId> scaled(const NodeId  theRoot,
                                  const Point3& theCenter,
                                  const double  theFactor) const
  {
    return transformed(theRoot, Transform::scale(theCenter, theFactor));
  }

  /// @brief Creates a mirrored copy of the shape rooted at @p theRoot.
  /// @param theRoot    NodeId of the shape to mirror.
  /// @param thePoint   A point on the mirror plane.
  /// @param theNormal  Unit normal of the mirror plane.
  /// @return A pair of (new Graph, root NodeId of the mirrored result).
  /// @throws Error on failure (invalid argument, not found, or geometric failure).
  /// @sa occtl_topo_mirror
  std::pair<Graph, NodeId> mirror(const NodeId                theRoot,
                                  const ::occtl_point3_t&     thePoint,
                                  const ::occtl_direction3_t& theNormal) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_mirrored(myPtr, theRoot.get(), thePoint, theNormal, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Splits a shape by a plane and returns selected side(s).
  /// @param theOpts  Root, plane, and side-selection options.
  /// @return A pair of (new Graph, root NodeId of the split result).
  /// @throws Error on failure (invalid argument, version mismatch, not found, or geometric
  /// failure).
  /// @sa occtl_topo_split_by_plane
  std::pair<Graph, NodeId> split_by_plane(
    const ::occtl_topo_split_by_plane_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_make_split_by_plane(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Sections a shape by one or more planes.
  /// @param theOpts  Root and section plane options.
  /// @return A pair of (new Graph, root NodeId of the section result).
  /// @throws Error on failure (invalid argument, version mismatch, not found, or geometric
  /// failure).
  /// @sa occtl_topo_section_by_planes
  std::pair<Graph, NodeId> section_by_planes(
    const ::occtl_topo_section_by_planes_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_make_sections_by_planes(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Thickens one or more Face nodes into prism solids.
  /// @param theOpts  Face list, thickness, and direction options.
  /// @return A pair of (new Graph, root NodeId of the extrude result).
  ///
  /// @sa occtl_topo_make_face_extrusion
  std::pair<Graph, NodeId> extrude_faces_to_solids(
    const ::occtl_topo_extrude_faces_options_t& theOpts) const
  {
    occtl_graph_t*  aOutGraph = nullptr;
    occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
    check(::occtl_topo_make_face_extrusion(myPtr, &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Creates @c count linearly-spaced copies of a shape.
  /// @param theRoot  NodeId of the shape to pattern.
  /// @param theOpts  Direction, count, and step distance.
  /// @return A pair of (new Graph, root NodeId of the resulting compound).
  /// @throws Error on failure (invalid argument, version mismatch, not found, or geometric
  /// failure).
  /// @sa occtl_topo_linear_pattern
  std::pair<Graph, NodeId> linear_pattern(
    const NodeId                                 theRoot,
    const ::occtl_topo_linear_pattern_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(::occtl_topo_make_linear_pattern(myPtr, theRoot.get(), &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  /// @brief Creates @c count angularly-spaced copies of a shape.
  /// @param theRoot  NodeId of the shape to pattern.
  /// @param theOpts  Axis, count, and angular step in radians.
  /// @return A pair of (new Graph, root NodeId of the resulting compound).
  /// @throws Error on failure (invalid argument, version mismatch, not found, or geometric
  /// failure).
  /// @sa occtl_topo_circular_pattern
  std::pair<Graph, NodeId> circular_pattern(
    const NodeId                                   theRoot,
    const ::occtl_topo_circular_pattern_options_t& theOpts) const
  {
    ::occtl_graph_t*  aOutGraph = nullptr;
    ::occtl_node_id_t aOutRoot{};
    check(
      ::occtl_topo_make_circular_pattern(myPtr, theRoot.get(), &theOpts, &aOutGraph, &aOutRoot));
    return {Graph(aOutGraph), NodeId(aOutRoot)};
  }

  template <typename Fn>
  void for_each(const uint64_t theKindMask, Fn theFn) const
  {
    driveVisitorMask(::occtl_graph_for_each, theKindMask, theFn);
  }

  template <typename Fn>
  void for_each_ref(const uint64_t theRefKindMask, Fn theFn) const
  {
    driveRefVisitor(::occtl_graph_for_each_ref, theRefKindMask, theFn);
  }

  template <typename Fn>
  void for_each_rep(const uint64_t theRepKindMask, Fn theFn) const
  {
    driveRepVisitor(::occtl_graph_for_each_rep, theRepKindMask, theFn);
  }

  template <typename Fn>
  void for_each_related(const NodeId theNode, Fn theFn) const
  {
    driveChildVisitor(::occtl_topo_for_each_related, theNode, theFn);
  }

private:
  /// Borrowed callback state graph visitors.
  template <typename Fn>
  struct VisitorBridge
  {
    Fn*                fn;
    std::exception_ptr exc;
  };

  template <typename Fn>
  static ::occtl_status_t OCCTL_CALL visitorTrampoline(const ::occtl_node_id_t theId,
                                                       void* const             theUser)
  {
    VisitorBridge<Fn>* const aBridge = static_cast<VisitorBridge<Fn>*>(theUser);
    try
    {
      if constexpr (std::is_invocable_r_v<bool, Fn&, NodeId>)
      {
        return (*aBridge->fn)(NodeId(theId)) ? OCCTL_OK : OCCTL_CANCELLED;
      }
      else
      {
        (*aBridge->fn)(NodeId(theId));
        return OCCTL_OK;
      }
    }
    catch (...)
    {
      aBridge->exc = std::current_exception();
      return OCCTL_INTERNAL;
    }
  }

  template <typename Fn>
  struct RefBridge
  {
    Fn*                fn;
    std::exception_ptr exc;
  };

  template <typename Fn>
  static ::occtl_status_t OCCTL_CALL refTrampoline(const ::occtl_ref_id_t theId,
                                                   void* const            theUser)
  {
    RefBridge<Fn>* const aBridge = static_cast<RefBridge<Fn>*>(theUser);
    try
    {
      if constexpr (std::is_invocable_r_v<bool, Fn&, ::occtl_ref_id_t>)
      {
        return (*aBridge->fn)(theId) ? OCCTL_OK : OCCTL_CANCELLED;
      }
      else
      {
        (*aBridge->fn)(theId);
        return OCCTL_OK;
      }
    }
    catch (...)
    {
      aBridge->exc = std::current_exception();
      return OCCTL_INTERNAL;
    }
  }

  template <typename Fn>
  struct RepBridge
  {
    Fn*                fn;
    std::exception_ptr exc;
  };

  template <typename Fn>
  static ::occtl_status_t OCCTL_CALL repTrampoline(const ::occtl_rep_id_t theId,
                                                   void* const            theUser)
  {
    RepBridge<Fn>* const aBridge = static_cast<RepBridge<Fn>*>(theUser);
    try
    {
      if constexpr (std::is_invocable_r_v<bool, Fn&, ::occtl_rep_id_t>)
      {
        return (*aBridge->fn)(theId) ? OCCTL_OK : OCCTL_CANCELLED;
      }
      else
      {
        (*aBridge->fn)(theId);
        return OCCTL_OK;
      }
    }
    catch (...)
    {
      aBridge->exc = std::current_exception();
      return OCCTL_INTERNAL;
    }
  }

  template <typename CFn, typename Fn>
  void driveVisitor(CFn theCFn, Fn& theFn) const
  {
    VisitorBridge<Fn>      aBridge{&theFn, nullptr};
    const ::occtl_status_t aSt = theCFn(myPtr, &visitorTrampoline<Fn>, &aBridge);
    if (aBridge.exc)
    {
      std::rethrow_exception(aBridge.exc);
    }
    check(aSt);
  }

  template <typename CFn, typename Fn>
  void driveChildVisitor(CFn theCFn, const NodeId theParent, Fn& theFn) const
  {
    VisitorBridge<Fn>      aBridge{&theFn, nullptr};
    const ::occtl_status_t aSt = theCFn(myPtr, theParent.get(), &visitorTrampoline<Fn>, &aBridge);
    if (aBridge.exc)
    {
      std::rethrow_exception(aBridge.exc);
    }
    check(aSt);
  }

  template <typename CFn, typename Fn>
  void driveVisitorMask(CFn theCFn, const uint64_t theMask, Fn& theFn) const
  {
    VisitorBridge<Fn>      aBridge{&theFn, nullptr};
    const ::occtl_status_t aSt = theCFn(myPtr, theMask, &visitorTrampoline<Fn>, &aBridge);
    if (aBridge.exc)
    {
      std::rethrow_exception(aBridge.exc);
    }
    check(aSt);
  }

  template <typename CFn, typename Fn>
  void driveRefVisitor(CFn theCFn, const uint64_t theMask, Fn& theFn) const
  {
    RefBridge<Fn>          aBridge{&theFn, nullptr};
    const ::occtl_status_t aSt = theCFn(myPtr, theMask, &refTrampoline<Fn>, &aBridge);
    if (aBridge.exc)
    {
      std::rethrow_exception(aBridge.exc);
    }
    check(aSt);
  }

  template <typename CFn, typename Fn>
  void driveRepVisitor(CFn theCFn, const uint64_t theMask, Fn& theFn) const
  {
    RepBridge<Fn>          aBridge{&theFn, nullptr};
    const ::occtl_status_t aSt = theCFn(myPtr, theMask, &repTrampoline<Fn>, &aBridge);
    if (aBridge.exc)
    {
      std::rethrow_exception(aBridge.exc);
    }
    check(aSt);
  }

  using HistoryAccessor = ::occtl_status_t (*)(const ::occtl_graph_t*,
                                               ::occtl_uid_t,
                                               ::occtl_uid_t*,
                                               std::size_t,
                                               std::size_t*);

  std::vector<UID> fetch_history(const HistoryAccessor theAccessor, const UID theInputUid) const
  {
    size_t aCount = 0;
    check(theAccessor(myPtr, theInputUid.get(), nullptr, 0, &aCount));
    std::vector<::occtl_uid_t> aRaw(aCount);
    if (aCount != 0)
    {
      check(theAccessor(myPtr, theInputUid.get(), aRaw.data(), aCount, &aCount));
    }
    std::vector<UID> aOut;
    aOut.reserve(aRaw.size());
    for (const ::occtl_uid_t& aUid : aRaw)
    {
      aOut.emplace_back(aUid);
    }
    return aOut;
  }

  ::occtl_graph_t* myPtr = nullptr;
};

} // namespace occtl

#endif // OCCTL_HPP_TOPO_HPP
