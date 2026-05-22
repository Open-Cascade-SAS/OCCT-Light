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

package main

import (
	"encoding/json"
	"fmt"
	"os"
	"sort"

	occtl "github.com/Open-Cascade-SAS/OCCT-Light/bindings/go/src/occtl"
)

type scenarioDoc struct {
	Scenario string         `json:"scenario"`
	Params   map[string]any `json:"params,omitempty"`
	Expected map[string]any `json:"expected,omitempty"`
}

func main() {
	if len(os.Args) != 2 {
		fmt.Fprintln(os.Stderr, "usage: parity_runner <scenario.json>")
		os.Exit(2)
	}
	raw, err := os.ReadFile(os.Args[1])
	if err != nil {
		fmt.Fprintf(os.Stderr, "parity_runner: %v\n", err)
		os.Exit(2)
	}
	var s scenarioDoc
	if err := json.Unmarshal(raw, &s); err != nil {
		fmt.Fprintf(os.Stderr, "parity_runner: parse scenario: %v\n", err)
		os.Exit(2)
	}
	var actual map[string]any
	switch s.Scenario {
	case "build_box":
		actual = runBuildBox(s)
	case "fuse_two_boxes":
		actual = runFuseTwoBoxes()
	case "cut_box_corner":
		actual = runCutBoxCorner()
	case "common_two_overlapping_boxes":
		actual = runCommon()
	case "section_two_overlapping_boxes":
		actual = runSection()
	case "split_box_by_box":
		actual = runSplit()
	case "history_modified_after_fuse":
		actual = runHistoryModified()
	default:
		fmt.Fprintf(os.Stderr, "parity_runner: unknown scenario '%s'\n", s.Scenario)
		os.Exit(2)
	}
	matches := matchesExpected(actual, s.Expected)
	out := map[string]any{
		"scenario": s.Scenario,
		"binding":  "go",
		"actual":   actual,
		"matches":  matches,
	}
	enc := json.NewEncoder(os.Stdout)
	enc.SetEscapeHTML(false)
	_ = enc.Encode(out)
	if !matches {
		os.Exit(1)
	}
}

// -------------------------------------------------------------- helpers

func paramFloat(s scenarioDoc, key string, dflt float64) float64 {
	if v, ok := s.Params[key]; ok {
		if f, ok := v.(float64); ok {
			return f
		}
	}
	return dflt
}

func kindStr(g *occtl.Graph, id occtl.NodeId) string {
	k, err := g.NodeKind(id)
	if err != nil {
		return "unknown"
	}
	return occtl.KindString(k)
}

func faceUIDs(g *occtl.Graph) []occtl.Uid {
	faces, _ := g.Faces()
	out := make([]occtl.Uid, 0, len(faces))
	for _, f := range faces {
		u, err := g.UidOf(f)
		if err == nil {
			out = append(out, u)
		}
	}
	return out
}

// -------------------------------------------------------------- scenarios

func runBuildBox(s scenarioDoc) map[string]any {
	dx := paramFloat(s, "dx", 10.0)
	dy := paramFloat(s, "dy", 10.0)
	dz := paramFloat(s, "dz", 5.0)
	g, _ := occtl.NewGraph()
	defer g.Free()
	_, _ = occtl.MakeBox(g, dx, dy, dz)
	return map[string]any{
		"face_count":   g.FaceCount(),
		"edge_count":   g.EdgeCount(),
		"vertex_count": g.VertexCount(),
		"solid_count":  g.SolidCount(),
	}
}

func runFuseTwoBoxes() map[string]any {
	g, _ := occtl.NewGraph()
	defer g.Free()
	a, _ := occtl.MakeBox(g, 10, 10, 10)
	b, _ := occtl.MakeBoxAt(g, 10, 10, 10, occtl.PlacementAt(5, 0, 0))
	beforeUIDs := faceUIDs(g)
	aFaces := beforeUIDs[:6]
	res, _ := occtl.Fuse(g, []occtl.NodeId{a}, []occtl.NodeId{b})
	nonempty := false
	for _, u := range aFaces {
		if g.HasModified(u) || g.HasGenerated(u) {
			nonempty = true
			break
		}
	}
	return map[string]any{
		"root_kind": kindStr(g, res),
		"history_modified_nonempty_on_first_face_of_box_a": nonempty,
	}
}

func runCutBoxCorner() map[string]any {
	g, _ := occtl.NewGraph()
	defer g.Free()
	a, _ := occtl.MakeBox(g, 10, 10, 10)
	_, _ = occtl.MakeBoxAt(g, 5, 5, 5, occtl.PlacementAt(7.5, 7.5, 7.5))
	solids, _ := g.Solids()
	res, _ := occtl.Cut(g, []occtl.NodeId{a}, solids[1:])
	return map[string]any{
		"root_kind":   kindStr(g, res),
		"solid_count": g.SolidCount(),
	}
}

func runCommon() map[string]any {
	g, _ := occtl.NewGraph()
	defer g.Free()
	a, _ := occtl.MakeBox(g, 10, 10, 10)
	b, _ := occtl.MakeBoxAt(g, 10, 10, 10, occtl.PlacementAt(5, 5, 5))
	res, _ := occtl.Common(g, []occtl.NodeId{a}, []occtl.NodeId{b})
	return map[string]any{
		"root_kind":   kindStr(g, res),
		"solid_count": g.SolidCount(),
	}
}

func runSection() map[string]any {
	g, _ := occtl.NewGraph()
	defer g.Free()
	a, _ := occtl.MakeBox(g, 10, 10, 10)
	b, _ := occtl.MakeBoxAt(g, 10, 10, 10, occtl.PlacementAt(5, 0, 0))
	before := g.EdgeCount()
	res, _ := occtl.Section(g, []occtl.NodeId{a}, []occtl.NodeId{b})
	return map[string]any{
		"root_kind":            kindStr(g, res),
		"edge_count_increased": g.EdgeCount() > before,
	}
}

func runSplit() map[string]any {
	g, _ := occtl.NewGraph()
	defer g.Free()
	a, _ := occtl.MakeBox(g, 10, 10, 10)
	b, _ := occtl.MakeBoxAt(g, 10, 10, 10, occtl.PlacementAt(0, 0, 5))
	res, _ := occtl.Split(g, []occtl.NodeId{a}, []occtl.NodeId{b})
	return map[string]any{
		"root_kind":      kindStr(g, res),
		"compound_count": g.CompoundCount(),
		"solid_count":    g.SolidCount(),
	}
}

func runHistoryModified() map[string]any {
	g, _ := occtl.NewGraph()
	defer g.Free()
	a, _ := occtl.MakeBox(g, 10, 10, 10)
	b, _ := occtl.MakeBoxAt(g, 10, 10, 10, occtl.PlacementAt(5, 0, 0))
	before := faceUIDs(g)
	aFaces := before[:6]
	bFaces := before[6:12]
	_, _ = occtl.Fuse(g, []occtl.NodeId{a}, []occtl.NodeId{b})
	anyModified := func(uids []occtl.Uid) bool {
		for _, u := range uids {
			if g.HasModified(u) || g.HasGenerated(u) {
				return true
			}
		}
		return false
	}
	return map[string]any{
		"box_a_modified_nonempty": anyModified(aFaces),
		"box_b_modified_nonempty": anyModified(bFaces),
	}
}

// -------------------------------------------------------------- expected matcher

func matchesExpected(actual, expected map[string]any) bool {
	if expected == nil {
		return true
	}
	keys := make([]string, 0, len(expected))
	for k := range expected {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	for _, key := range keys {
		want := expected[key]
		if base, ok := strip(key, "_in"); ok {
			got := actual[base]
			arr, ok := want.([]any)
			if !ok {
				return false
			}
			match := false
			for _, candidate := range arr {
				if equalAny(got, candidate) {
					match = true
					break
				}
			}
			if !match {
				return false
			}
		} else if base, ok := strip(key, "_at_least"); ok {
			if !geqAny(actual[base], want) {
				return false
			}
		} else if base, ok := strip(key, "_greater_than"); ok {
			if !gtAny(actual[base], want) {
				return false
			}
		} else if !equalAny(actual[key], want) {
			return false
		}
	}
	return true
}

func strip(s, suffix string) (string, bool) {
	if len(s) > len(suffix) && s[len(s)-len(suffix):] == suffix {
		return s[:len(s)-len(suffix)], true
	}
	return "", false
}

func equalAny(a, b any) bool {
	// Normalise numbers — JSON-decoded bools and ints get encoded as
	// either bool / float64; our actual map uses int / bool natively.
	switch av := a.(type) {
	case int:
		if bv, ok := b.(float64); ok {
			return float64(av) == bv
		}
	case bool:
		if bv, ok := b.(bool); ok {
			return av == bv
		}
	case string:
		if bv, ok := b.(string); ok {
			return av == bv
		}
	}
	return fmt.Sprintf("%v", a) == fmt.Sprintf("%v", b)
}

func geqAny(a, b any) bool {
	af, bf := toF(a), toF(b)
	return af >= bf
}
func gtAny(a, b any) bool {
	af, bf := toF(a), toF(b)
	return af > bf
}
func toF(v any) float64 {
	switch x := v.(type) {
	case int:
		return float64(x)
	case float64:
		return x
	}
	return 0
}
