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

//go:build occtl_io_brep && occtl_topo

package occtl

/*
#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>
#include <occtl/occtl_io_brep.h>
#include <stdlib.h>
*/
import "C"
import "unsafe"

// IoBrepWriteOptions mirrors occtl_io_brep_write_options_t.
// The output format (binary / ASCII) is managed internally by the OCCT
// DEBREP_Provider.
type IoBrepWriteOptions struct {
	WriteTriangulation bool
}

// DefaultBrepWriteOptions returns defaults.
func DefaultBrepWriteOptions() IoBrepWriteOptions {
	return IoBrepWriteOptions{WriteTriangulation: true}
}

// IoBrepRead reads a BRep file and returns the new graph + root.
func IoBrepRead(path string) (*Graph, NodeId, error) {
	ensureInit()
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	var p *C.occtl_graph_t
	var root C.occtl_node_id_t
	if err := check(C.occtl_io_brep_read(cpath, &p, &root)); err != nil {
		return nil, NodeId{}, err
	}
	return adoptGraph(p), NodeId{raw: root}, nil
}

// IoBrepWrite writes the topology rooted at `root` to a file.
func IoBrepWrite(g *Graph, root NodeId, path string, options IoBrepWriteOptions) error {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	var opts C.occtl_io_brep_write_options_t
	C.occtl_io_brep_write_options_init(&opts)
	if options.WriteTriangulation {
		opts.write_triangulation = 1
	}
	return check(C.occtl_io_brep_write(g.ptr, root.raw, cpath, &opts))
}
