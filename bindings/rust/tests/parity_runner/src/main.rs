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

use occtl::{bool_, prim, sys, BoxInfo, Graph, NodeId, Uid};
use serde_json::{json, Map, Value};
use std::collections::BTreeMap;
use std::env;
use std::fs;
use std::path::PathBuf;
use std::process::ExitCode;

fn main() -> ExitCode {
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("usage: parity_runner <scenario.json>");
        return ExitCode::from(2);
    }
    let path = PathBuf::from(&args[1]);
    let scenario: Value = serde_json::from_str(&fs::read_to_string(&path).expect("read scenario"))
        .expect("parse scenario json");
    let name = scenario["scenario"].as_str().unwrap_or("");

    let actual = match name {
        "build_box" => run_build_box(&scenario),
        "fuse_two_boxes" => run_fuse_two_boxes(),
        "cut_box_corner" => run_cut_box_corner(),
        "common_two_overlapping_boxes" => run_common(),
        "section_two_overlapping_boxes" => run_section(),
        "split_box_by_box" => run_split(),
        "history_modified_after_fuse" => run_history_modified(),
        other => {
            eprintln!("parity_runner: unknown scenario '{}'", other);
            return ExitCode::from(2);
        }
    };

    let expected = scenario.get("expected").cloned().unwrap_or(Value::Null);
    let matches = matches_expected(&actual, &expected);
    let mut out = Map::new();
    out.insert("scenario".into(), Value::String(name.to_string()));
    out.insert("binding".into(), Value::String("rust".into()));
    out.insert("actual".into(), Value::Object(actual.into_iter().collect()));
    out.insert("matches".into(), Value::Bool(matches));
    println!("{}", serde_json::to_string(&Value::Object(out)).unwrap());
    if matches {
        ExitCode::SUCCESS
    } else {
        ExitCode::FAILURE
    }
}

// ----------------------------------------------------------- scenario helpers

fn kind_str(graph: &Graph, id: NodeId) -> String {
    match graph.node_kind(id) {
        Ok(k) => NodeId::kind_string(k).to_string(),
        Err(_) => "unknown".to_string(),
    }
}

fn face_uids(graph: &Graph) -> Vec<Uid> {
    graph
        .face_iter()
        .map(|it| it.filter_map(|n| graph.uid_of(n).ok()).collect())
        .unwrap_or_default()
}

fn placed(x: f64, y: f64, z: f64) -> sys::occtl_axis2_placement_t {
    prim::placement_at(x, y, z)
}

// ----------------------------------------------------------- scenarios

fn run_build_box(scenario: &Value) -> BTreeMap<String, Value> {
    let params = scenario.get("params").cloned().unwrap_or(Value::Null);
    let dx = params.get("dx").and_then(|v| v.as_f64()).unwrap_or(10.0);
    let dy = params.get("dy").and_then(|v| v.as_f64()).unwrap_or(10.0);
    let dz = params.get("dz").and_then(|v| v.as_f64()).unwrap_or(5.0);

    let g = Graph::new().unwrap();
    prim::make_box(&g, BoxInfo::new(dx, dy, dz)).unwrap();
    let mut out = BTreeMap::new();
    out.insert("face_count".into(), json!(g.face_count()));
    out.insert("edge_count".into(), json!(g.edge_count()));
    out.insert("vertex_count".into(), json!(g.vertex_count()));
    out.insert("solid_count".into(), json!(g.solid_count()));
    out
}

fn run_fuse_two_boxes() -> BTreeMap<String, Value> {
    let g = Graph::new().unwrap();
    let a = prim::make_box(&g, BoxInfo::new(10.0, 10.0, 10.0)).unwrap();
    let b = prim::make_box(
        &g,
        BoxInfo::new(10.0, 10.0, 10.0).with_placement(placed(5.0, 0.0, 0.0)),
    )
    .unwrap();
    let a_faces: Vec<Uid> = face_uids(&g).into_iter().take(6).collect();
    let result = bool_::fuse(&g, &[a], &[b]).unwrap();
    let nonempty = a_faces.iter().any(|uid| has_history_image(&g, *uid));
    let mut out = BTreeMap::new();
    out.insert("root_kind".into(), Value::String(kind_str(&g, result)));
    out.insert(
        "history_modified_nonempty_on_first_face_of_box_a".into(),
        Value::Bool(nonempty),
    );
    out
}

fn run_cut_box_corner() -> BTreeMap<String, Value> {
    let g = Graph::new().unwrap();
    let box_a = prim::make_box(&g, BoxInfo::new(10.0, 10.0, 10.0)).unwrap();
    prim::make_box(
        &g,
        BoxInfo::new(5.0, 5.0, 5.0).with_placement(placed(7.5, 7.5, 7.5)),
    )
    .unwrap();
    let solids: Vec<NodeId> = g.solid_iter().unwrap().collect();
    let tool = solids[1..].to_vec();
    let result = bool_::cut(&g, &[box_a], &tool).unwrap();
    let mut out = BTreeMap::new();
    out.insert("root_kind".into(), Value::String(kind_str(&g, result)));
    out.insert("solid_count".into(), json!(g.solid_count()));
    out
}

fn run_common() -> BTreeMap<String, Value> {
    let g = Graph::new().unwrap();
    let a = prim::make_box(&g, BoxInfo::new(10.0, 10.0, 10.0)).unwrap();
    let b = prim::make_box(
        &g,
        BoxInfo::new(10.0, 10.0, 10.0).with_placement(placed(5.0, 5.0, 5.0)),
    )
    .unwrap();
    let result = bool_::common(&g, &[a], &[b]).unwrap();
    let mut out = BTreeMap::new();
    out.insert("root_kind".into(), Value::String(kind_str(&g, result)));
    out.insert("solid_count".into(), json!(g.solid_count()));
    out
}

fn run_section() -> BTreeMap<String, Value> {
    let g = Graph::new().unwrap();
    let a = prim::make_box(&g, BoxInfo::new(10.0, 10.0, 10.0)).unwrap();
    let b = prim::make_box(
        &g,
        BoxInfo::new(10.0, 10.0, 10.0).with_placement(placed(5.0, 0.0, 0.0)),
    )
    .unwrap();
    let edge_count_before = g.edge_count();
    let result = bool_::section(&g, &[a], &[b]).unwrap();
    let mut out = BTreeMap::new();
    out.insert("root_kind".into(), Value::String(kind_str(&g, result)));
    out.insert(
        "edge_count_increased".into(),
        Value::Bool(g.edge_count() > edge_count_before),
    );
    out
}

fn run_split() -> BTreeMap<String, Value> {
    let g = Graph::new().unwrap();
    let a = prim::make_box(&g, BoxInfo::new(10.0, 10.0, 10.0)).unwrap();
    let b = prim::make_box(
        &g,
        BoxInfo::new(10.0, 10.0, 10.0).with_placement(placed(0.0, 0.0, 5.0)),
    )
    .unwrap();
    let result = bool_::split(&g, &[a], &[b]).unwrap();
    let mut out = BTreeMap::new();
    out.insert("root_kind".into(), Value::String(kind_str(&g, result)));
    out.insert("compound_count".into(), json!(g.compound_count()));
    out.insert("solid_count".into(), json!(g.solid_count()));
    out
}

fn run_history_modified() -> BTreeMap<String, Value> {
    let g = Graph::new().unwrap();
    let a = prim::make_box(&g, BoxInfo::new(10.0, 10.0, 10.0)).unwrap();
    let b = prim::make_box(
        &g,
        BoxInfo::new(10.0, 10.0, 10.0).with_placement(placed(5.0, 0.0, 0.0)),
    )
    .unwrap();
    let before = face_uids(&g);
    let a_faces: Vec<Uid> = before[0..6].to_vec();
    let b_faces: Vec<Uid> = before[6..12].to_vec();
    bool_::fuse(&g, &[a], &[b]).unwrap();

    let any_modified = |uids: &[Uid]| -> bool { uids.iter().any(|u| has_history_image(&g, *u)) };

    let mut out = BTreeMap::new();
    out.insert(
        "box_a_modified_nonempty".into(),
        Value::Bool(any_modified(&a_faces)),
    );
    out.insert(
        "box_b_modified_nonempty".into(),
        Value::Bool(any_modified(&b_faces)),
    );
    out
}

fn has_history_image(graph: &Graph, uid: Uid) -> bool {
    graph
        .history_modified(uid)
        .map(|v| !v.is_empty())
        .unwrap_or(false)
        || graph
            .history_generated(uid)
            .map(|v| !v.is_empty())
            .unwrap_or(false)
}

// ----------------------------------------------------------- expected matcher

fn matches_expected(actual: &BTreeMap<String, Value>, expected: &Value) -> bool {
    let Some(exp) = expected.as_object() else {
        return true;
    };
    for (key, want) in exp {
        if let Some(base) = key.strip_suffix("_in") {
            let Some(got) = actual.get(base) else {
                return false;
            };
            let arr = match want.as_array() {
                Some(a) => a,
                None => return false,
            };
            if !arr.iter().any(|v| v == got) {
                return false;
            }
        } else if let Some(base) = key.strip_suffix("_at_least") {
            let Some(got) = actual.get(base).and_then(|v| v.as_i64()) else {
                return false;
            };
            let Some(min) = want.as_i64() else {
                return false;
            };
            if !(got >= min) {
                return false;
            }
        } else if let Some(base) = key.strip_suffix("_greater_than") {
            let Some(got) = actual.get(base).and_then(|v| v.as_i64()) else {
                return false;
            };
            let Some(min) = want.as_i64() else {
                return false;
            };
            if !(got > min) {
                return false;
            }
        } else if actual.get(key) != Some(want) {
            return false;
        }
    }
    true
}
