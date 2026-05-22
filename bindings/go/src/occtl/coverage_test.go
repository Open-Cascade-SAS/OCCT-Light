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
	"encoding/json"
	"os"
	"path/filepath"
	"regexp"
	"runtime"
	"sort"
	"testing"
)

type abiDoc struct {
	Functions []struct {
		Name string `json:"name"`
	} `json:"functions"`
}

func abiFunctions(t *testing.T) map[string]bool {
	t.Helper()
	_, file, _, _ := runtime.Caller(0)
	repoRoot := filepath.Join(filepath.Dir(file), "..", "..", "..")
	path := filepath.Join(repoRoot, "build", "abi.json")
	data, err := os.ReadFile(path)
	if err != nil {
		t.Skipf("coverage: %s missing (run python3 tools/abi_dump.py first)", path)
	}
	var doc abiDoc
	if err := json.Unmarshal(data, &doc); err != nil {
		t.Fatalf("coverage: parse abi.json: %v", err)
	}
	if len(doc.Functions) < 100 {
		t.Fatalf("coverage: abi.json had only %d functions", len(doc.Functions))
	}

	out := make(map[string]bool, len(doc.Functions))
	for _, fn := range doc.Functions {
		out[fn.Name] = true
	}
	return out
}

func cgoFunctions(t *testing.T, abi map[string]bool) map[string]bool {
	t.Helper()
	_, file, _, _ := runtime.Caller(0)
	dir := filepath.Dir(file)
	matches := regexp.MustCompile(`C\.(occtl_[A-Za-z0-9_]+)\(`)
	out := make(map[string]bool)

	files, err := filepath.Glob(filepath.Join(dir, "*.go"))
	if err != nil {
		t.Fatalf("coverage: list Go files: %v", err)
	}
	for _, path := range files {
		if filepath.Base(path) == "coverage_test.go" {
			continue
		}
		data, err := os.ReadFile(path)
		if err != nil {
			t.Fatalf("coverage: read %s: %v", path, err)
		}
		for _, match := range matches.FindAllSubmatch(data, -1) {
			name := string(match[1])
			if abi[name] {
				out[name] = true
			}
		}
	}
	return out
}

func sortedKeys(values map[string]bool) []string {
	out := make([]string, 0, len(values))
	for key := range values {
		out = append(out, key)
	}
	sort.Strings(out)
	return out
}

func TestPromotedCgoSurfaceMatchesAbi(t *testing.T) {
	abi := abiFunctions(t)
	cgo := cgoFunctions(t, abi)

	required := promotedSymbols()
	for _, name := range required {
		if !abi[name] {
			t.Fatalf("coverage: required symbol %s is absent from abi.json", name)
		}
		if !cgo[name] {
			t.Fatalf("coverage: promoted Go cgo surface no longer calls %s", name)
		}
	}

	if len(cgo) < len(required) {
		t.Fatalf("coverage: found only %d ABI-backed cgo calls: %v", len(cgo), sortedKeys(cgo))
	}
	t.Logf("coverage: %d promoted Go cgo calls validated against abi.json", len(cgo))
}

func promotedSymbols() []string {
	required := []string{
		"occtl_runtime_init",
		"occtl_runtime_init_info_init",
		"occtl_runtime_abi_version",
		"occtl_status_to_string",
		"occtl_error_last",
		"occtl_graph_create",
		"occtl_graph_free",
		"occtl_graph_history_deleted_all",
		"occtl_graph_history_generated",
		"occtl_graph_history_modified",
		"occtl_graph_rep_uid_from_rep_id",
		"occtl_graph_rep_id_from_rep_uid",
		"occtl_topo_check",
		"occtl_graph_node_kind",
		"occtl_graph_uid_from_node_id",
		"occtl_graph_face_iter_create",
		"occtl_graph_solid_iter_create",
		"occtl_node_iter_next",
		"occtl_node_iter_free",
		"occtl_prim_make_box",
		"occtl_prim_make_sphere",
		"occtl_prim_make_cylinder",
		"occtl_prim_make_cone",
		"occtl_prim_make_torus",
		"occtl_prim_make_wedge",
	}
	required = append(required, promotedBoolSymbols()...)
	required = append(required, promotedIoBrepSymbols()...)
	return required
}
