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

package occtl

import (
	"errors"
	"testing"
)

func TestPrimitiveBuildersCreateSolids(t *testing.T) {
	g, err := CreateGraph()
	if err != nil {
		t.Fatalf("CreateGraph: %v", err)
	}
	defer g.Close()

	checkNode := func(name string, id NodeId, err error) {
		t.Helper()
		if err != nil {
			t.Fatalf("%s: %v", name, err)
		}
		if !id.IsValid() {
			t.Fatalf("%s: invalid node id", name)
		}
	}

	id, err := g.MakeBox(1, 2, 3)
	checkNode("MakeBox", id, err)
	id, err = g.MakeSphere(2)
	checkNode("MakeSphere", id, err)
	id, err = g.MakeCylinder(1, 4)
	checkNode("MakeCylinder", id, err)
	id, err = g.MakeCone(2, 1, 5)
	checkNode("MakeCone", id, err)
	id, err = g.MakeTorus(5, 1)
	checkNode("MakeTorus", id, err)
	id, err = g.MakeWedge(4, 3, 2, 1)
	checkNode("MakeWedge", id, err)

	if got := g.SolidCount(); got != 6 {
		t.Fatalf("SolidCount = %d, want 6", got)
	}
	issues, err := g.CheckIssues()
	if err != nil {
		t.Fatalf("CheckIssues: %v", err)
	}
	if len(issues) != 0 {
		t.Fatalf("CheckIssues returned %d issue(s), want 0", len(issues))
	}
	valid, err := g.IsValid()
	if err != nil {
		t.Fatalf("IsValid: %v", err)
	}
	if !valid {
		t.Fatalf("IsValid = false, want true")
	}
}

func TestGraphFromPointerRejectsNull(t *testing.T) {
	_, err := GraphFromPointerUnsafe(0)
	if err == nil {
		t.Fatal("GraphFromPointerUnsafe(0) unexpectedly succeeded")
	}

	var occtlErr *Error
	if !errors.As(err, &occtlErr) {
		t.Fatalf("GraphFromPointerUnsafe error type = %T, want *Error", err)
	}
	if occtlErr.Status != StatusInvalidHandle {
		t.Fatalf("GraphFromPointerUnsafe status = %v, want %v", occtlErr.Status, StatusInvalidHandle)
	}
}
