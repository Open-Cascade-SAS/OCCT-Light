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

//go:build occtl_topo

package occtl

/*
#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>
#include <occtl/occtl_topo_algo.h>
*/
import "C"
import (
	"runtime"
	"unsafe"
)

// NodeId is a strong wrapper around occtl_node_id_t.
type NodeId struct{ raw C.occtl_node_id_t }

func (n NodeId) Bits() uint64  { return uint64(n.raw.bits) }
func (n NodeId) IsValid() bool { return n.raw.bits != 0 }

// Uid is a strong wrapper around occtl_uid_t.
type Uid struct{ raw C.occtl_uid_t }

func (u Uid) Bits() uint64 { return uint64(u.raw.bits) }

// RefUid is a strong wrapper around occtl_ref_uid_t.
type RefUid struct{ raw C.occtl_ref_uid_t }

func (u RefUid) Bits() uint64 { return uint64(u.raw.bits) }

// RepId is a strong wrapper around occtl_rep_id_t.
type RepId struct{ raw C.occtl_rep_id_t }

func (r RepId) Bits() uint64 { return uint64(r.raw.bits) }

// RepUid is a strong wrapper around occtl_rep_uid_t.
type RepUid struct{ raw C.occtl_rep_uid_t }

func (r RepUid) Bits() uint64 { return uint64(r.raw.bits) }

// KindString maps occtl_node_kind_t to parity tokens.
func KindString(kind C.occtl_node_kind_t) string {
	switch kind {
	case C.OCCTL_KIND_SOLID:
		return "solid"
	case C.OCCTL_KIND_SHELL:
		return "shell"
	case C.OCCTL_KIND_FACE:
		return "face"
	case C.OCCTL_KIND_WIRE:
		return "wire"
	case C.OCCTL_KIND_EDGE:
		return "edge"
	case C.OCCTL_KIND_VERTEX:
		return "vertex"
	case C.OCCTL_KIND_COMPOUND:
		return "compound"
	case C.OCCTL_KIND_COMPSOLID:
		return "compsolid"
	case C.OCCTL_KIND_COEDGE:
		return "coedge"
	case C.OCCTL_KIND_PRODUCT:
		return "product"
	case C.OCCTL_KIND_OCCURRENCE:
		return "occurrence"
	}
	return "unknown"
}

// Graph owns an occtl_graph_t*.
type Graph struct {
	ptr *C.occtl_graph_t
}

func adoptGraph(ptr *C.occtl_graph_t) *Graph {
	g := &Graph{ptr: ptr}
	runtime.SetFinalizer(g, func(g *Graph) {
		if g.ptr != nil {
			C.occtl_graph_free(g.ptr)
			g.ptr = nil
		}
	})
	return g
}

// NewGraph allocates an empty graph.
func NewGraph() (*Graph, error) {
	ensureInit()
	var p *C.occtl_graph_t
	if err := check(C.occtl_graph_create(&p)); err != nil {
		return nil, err
	}
	return adoptGraph(p), nil
}

// CreateGraph is a naming-aligned alias of NewGraph.
func CreateGraph() (*Graph, error) { return NewGraph() }

// GraphFromPointerUnsafe adopts an existing native graph pointer.
func GraphFromPointerUnsafe(ptr uintptr) (*Graph, error) {
	ensureInit()
	if ptr == 0 {
		return nil, &Error{
			Status:  StatusInvalidHandle,
			Message: "GraphFromPointerUnsafe received a null pointer",
		}
	}
	return adoptGraph((*C.occtl_graph_t)(unsafe.Pointer(ptr))), nil
}

// Free releases the graph immediately.
func (g *Graph) Free() {
	if g.ptr != nil {
		C.occtl_graph_free(g.ptr)
		g.ptr = nil
		runtime.SetFinalizer(g, nil)
	}
}

// Close is a naming-aligned alias of Free.
func (g *Graph) Close() { g.Free() }

func mustCount(st C.occtl_status_t, count C.size_t) int {
	if err := check(st); err != nil {
		panic(err)
	}
	return int(count)
}

func (g *Graph) SolidCount() int {
	var count C.size_t
	return mustCount(C.occtl_graph_solid_count(g.ptr, &count), count)
}

func (g *Graph) ShellCount() int {
	var count C.size_t
	return mustCount(C.occtl_graph_shell_count(g.ptr, &count), count)
}

func (g *Graph) FaceCount() int {
	var count C.size_t
	return mustCount(C.occtl_graph_face_count(g.ptr, &count), count)
}

func (g *Graph) WireCount() int {
	var count C.size_t
	return mustCount(C.occtl_graph_wire_count(g.ptr, &count), count)
}

func (g *Graph) EdgeCount() int {
	var count C.size_t
	return mustCount(C.occtl_graph_edge_count(g.ptr, &count), count)
}

func (g *Graph) VertexCount() int {
	var count C.size_t
	return mustCount(C.occtl_graph_vertex_count(g.ptr, &count), count)
}

func (g *Graph) CompoundCount() int {
	var count C.size_t
	return mustCount(C.occtl_graph_compound_count(g.ptr, &count), count)
}

// CheckIssue mirrors occtl_topo_check_issue_t.
type CheckIssue struct {
	NodeID        NodeId
	ContextNodeID NodeId
	StatusBit     uint32
	Severity      int32
}

// CheckIssues runs graph validation and returns all reported issues.
func (g *Graph) CheckIssues() ([]CheckIssue, error) {
	var count C.size_t
	if err := check(C.occtl_topo_check(g.ptr, nil, 0, &count)); err != nil {
		return nil, err
	}
	if count == 0 {
		return nil, nil
	}

	native := make([]C.occtl_topo_check_issue_t, int(count))
	if err := check(C.occtl_topo_check(g.ptr, &native[0], count, &count)); err != nil {
		return nil, err
	}
	out := make([]CheckIssue, int(count))
	for i := range out {
		out[i] = CheckIssue{
			NodeID:        NodeId{raw: native[i].node_id},
			ContextNodeID: NodeId{raw: native[i].context_node_id},
			StatusBit:     uint32(native[i].status_bit),
			Severity:      int32(native[i].severity),
		}
	}
	return out, nil
}

// IsValid returns true when graph validation reports no issues.
func (g *Graph) IsValid() (bool, error) {
	issues, err := g.CheckIssues()
	if err != nil {
		return false, err
	}
	return len(issues) == 0, nil
}

// NodeKind returns the kind of a node id.
func (g *Graph) NodeKind(id NodeId) (C.occtl_node_kind_t, error) {
	var kind C.occtl_node_kind_t = C.OCCTL_KIND_INVALID
	if err := check(C.occtl_graph_node_kind(g.ptr, id.raw, &kind)); err != nil {
		return kind, err
	}
	return kind, nil
}

// UidOf returns the persistent UID for a node.
func (g *Graph) UidOf(id NodeId) (Uid, error) {
	var u C.occtl_uid_t
	if err := check(C.occtl_graph_uid_from_node_id(g.ptr, id.raw, &u)); err != nil {
		return Uid{}, err
	}
	return Uid{raw: u}, nil
}

// RepUidOf returns the persistent UID for a live representation.
func (g *Graph) RepUidOf(id RepId) (RepUid, error) {
	var u C.occtl_rep_uid_t
	if err := check(C.occtl_graph_rep_uid_from_rep_id(g.ptr, id.raw, &u)); err != nil {
		return RepUid{}, err
	}
	return RepUid{raw: u}, nil
}

// RepIdOf resolves a persistent representation UID to the current RepId.
func (g *Graph) RepIdOf(uid RepUid) (RepId, error) {
	var id C.occtl_rep_id_t
	if err := check(C.occtl_graph_rep_id_from_rep_uid(g.ptr, uid.raw, &id)); err != nil {
		return RepId{}, err
	}
	return RepId{raw: id}, nil
}

// Faces collects every active face id into a slice.
func (g *Graph) Faces() ([]NodeId, error) {
	return g.collect(func(out **C.occtl_node_iter_t) C.occtl_status_t {
		return C.occtl_graph_face_iter_create(g.ptr, out)
	})
}

// Solids collects every active solid id into a slice.
func (g *Graph) Solids() ([]NodeId, error) {
	return g.collect(func(out **C.occtl_node_iter_t) C.occtl_status_t {
		return C.occtl_graph_solid_iter_create(g.ptr, out)
	})
}

func (g *Graph) collect(create func(**C.occtl_node_iter_t) C.occtl_status_t) ([]NodeId, error) {
	var it *C.occtl_node_iter_t
	if err := check(create(&it)); err != nil {
		return nil, err
	}
	defer C.occtl_node_iter_free(it)
	var out []NodeId
	for {
		var id C.occtl_node_id_t
		st := C.occtl_node_iter_next(it, &id)
		if Status(st) != StatusOK {
			break
		}
		out = append(out, NodeId{raw: id})
	}
	return out, nil
}

// HasModified returns true when input has any modified images in graph-owned history.
func (g *Graph) HasModified(input Uid) bool {
	var count C.size_t
	st := C.occtl_graph_history_modified(g.ptr, input.raw, nil, 0, &count)
	if Status(st) != StatusOK {
		return false
	}
	return count > 0
}

// HasGenerated returns true when input has any generated images in graph-owned history.
func (g *Graph) HasGenerated(input Uid) bool {
	var count C.size_t
	st := C.occtl_graph_history_generated(g.ptr, input.raw, nil, 0, &count)
	if Status(st) != StatusOK {
		return false
	}
	return count > 0
}

// HasDeleted returns true when graph-owned history contains any deleted input UID.
func (g *Graph) HasDeleted() bool {
	var count C.size_t
	st := C.occtl_graph_history_deleted_all(g.ptr, nil, 0, &count)
	if Status(st) != StatusOK {
		return false
	}
	return count > 0
}
