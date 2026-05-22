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

//go:build occtl_bool && occtl_topo

package occtl

/*
#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>
#include <occtl/occtl_bool.h>
*/
import "C"
import (
	"runtime"
	"unsafe"
)

type boolFn func(*C.occtl_graph_t, *C.occtl_node_id_t, C.size_t, *C.occtl_node_id_t, C.size_t,
	*C.occtl_bool_options_t, *C.occtl_node_id_t) C.occtl_status_t

// BoolOptions mirrors occtl_bool_options_t.
type BoolOptions struct {
	FuzzyValue               float64
	RunParallel              bool
	SimplifyResult           bool
	SimplifyAngularTolerance float64
	BuildHistory             bool
}

func boolToI32(v bool) C.int32_t {
	if v {
		return 1
	}
	return 0
}

// DefaultBoolOptions returns the C-ABI defaults for boolean operations.
func DefaultBoolOptions() BoolOptions {
	var opts C.occtl_bool_options_t
	C.occtl_bool_options_init(&opts)
	return BoolOptions{
		FuzzyValue:               float64(opts.fuzzy_value),
		RunParallel:              opts.run_parallel != 0,
		SimplifyResult:           opts.simplify_result != 0,
		SimplifyAngularTolerance: float64(opts.simplify_angular_tolerance),
		BuildHistory:             opts.build_history != 0,
	}
}

func runBool(op boolFn, g *Graph, objects, tools []NodeId, options *BoolOptions) (NodeId, error) {
	var opts C.occtl_bool_options_t
	C.occtl_bool_options_init(&opts)
	if options != nil {
		opts.fuzzy_value = C.double(options.FuzzyValue)
		opts.run_parallel = boolToI32(options.RunParallel)
		opts.simplify_result = boolToI32(options.SimplifyResult)
		opts.simplify_angular_tolerance = C.double(options.SimplifyAngularTolerance)
		opts.build_history = boolToI32(options.BuildHistory)
	}
	var root C.occtl_node_id_t

	objPtr, objLen := slicePtrLen(objects)
	toolPtr, toolLen := slicePtrLen(tools)

	st := op(g.ptr, objPtr, objLen, toolPtr, toolLen, &opts, &root)
	runtime.KeepAlive(objects)
	runtime.KeepAlive(tools)
	if err := check(st); err != nil {
		return NodeId{}, err
	}
	return NodeId{raw: root}, nil
}

func slicePtrLen(s []NodeId) (*C.occtl_node_id_t, C.size_t) {
	if len(s) == 0 {
		return nil, 0
	}
	return (*C.occtl_node_id_t)(unsafe.Pointer(&s[0].raw)), C.size_t(len(s))
}

// Fuse runs the Fuse boolean op.
func Fuse(g *Graph, objects, tools []NodeId) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_fuse(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, nil)
}

// FuseWithOptions runs the Fuse boolean op with explicit options.
func FuseWithOptions(g *Graph, objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_fuse(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, &options)
}

// Cut runs the Cut boolean op.
func Cut(g *Graph, objects, tools []NodeId) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_cut(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, nil)
}

// CutWithOptions runs the Cut boolean op with explicit options.
func CutWithOptions(g *Graph, objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_cut(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, &options)
}

// Common runs the Common boolean op.
func Common(g *Graph, objects, tools []NodeId) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_common(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, nil)
}

// CommonWithOptions runs the Common boolean op with explicit options.
func CommonWithOptions(g *Graph, objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_common(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, &options)
}

// Section runs the Section boolean op.
func Section(g *Graph, objects, tools []NodeId) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_section(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, nil)
}

// SectionWithOptions runs the Section boolean op with explicit options.
func SectionWithOptions(g *Graph, objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_section(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, &options)
}

// Split runs the Split boolean op.
func Split(g *Graph, objects, tools []NodeId) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_split(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, nil)
}

// SplitWithOptions runs the Split boolean op with explicit options.
func SplitWithOptions(g *Graph, objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return runBool(func(gp *C.occtl_graph_t, op *C.occtl_node_id_t, on C.size_t,
		tp *C.occtl_node_id_t, tn C.size_t, opts *C.occtl_bool_options_t,
		r *C.occtl_node_id_t) C.occtl_status_t {
		return C.occtl_bool_split(gp, op, on, tp, tn, opts, r)
	}, g, objects, tools, &options)
}

// Fuse runs the Fuse boolean op in this graph.
func (g *Graph) Fuse(objects, tools []NodeId) (NodeId, error) { return Fuse(g, objects, tools) }

// FuseWithOptions runs the Fuse boolean op in this graph with explicit options.
func (g *Graph) FuseWithOptions(objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return FuseWithOptions(g, objects, tools, options)
}

// Cut runs the Cut boolean op in this graph.
func (g *Graph) Cut(objects, tools []NodeId) (NodeId, error) { return Cut(g, objects, tools) }

// CutWithOptions runs the Cut boolean op in this graph with explicit options.
func (g *Graph) CutWithOptions(objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return CutWithOptions(g, objects, tools, options)
}

// Common runs the Common boolean op in this graph.
func (g *Graph) Common(objects, tools []NodeId) (NodeId, error) { return Common(g, objects, tools) }

// CommonWithOptions runs the Common boolean op in this graph with explicit options.
func (g *Graph) CommonWithOptions(objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return CommonWithOptions(g, objects, tools, options)
}

// Section runs the Section boolean op in this graph.
func (g *Graph) Section(objects, tools []NodeId) (NodeId, error) { return Section(g, objects, tools) }

// SectionWithOptions runs the Section boolean op in this graph with explicit options.
func (g *Graph) SectionWithOptions(objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return SectionWithOptions(g, objects, tools, options)
}

// Split runs the Split boolean op in this graph.
func (g *Graph) Split(objects, tools []NodeId) (NodeId, error) { return Split(g, objects, tools) }

// SplitWithOptions runs the Split boolean op in this graph with explicit options.
func (g *Graph) SplitWithOptions(objects, tools []NodeId, options BoolOptions) (NodeId, error) {
	return SplitWithOptions(g, objects, tools, options)
}
