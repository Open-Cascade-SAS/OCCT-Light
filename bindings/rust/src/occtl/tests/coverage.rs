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

// Raw-symbol coverage for the Rust binding. bindgen must expose every
// OCCTL_API function listed in build/abi.json; the safe `occtl` crate remains
// intentionally narrower while the binding is draft.

use serde_json::Value;
use std::collections::BTreeSet;
use std::path::{Path, PathBuf};

fn repo_root() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .ancestors()
        .nth(4)
        .expect("occtl crate is under bindings/rust/src/occtl")
        .to_path_buf()
}

fn abi_functions(path: &Path, enabled_headers: Option<&BTreeSet<String>>) -> BTreeSet<String> {
    let abi: Value = serde_json::from_str(&std::fs::read_to_string(path).unwrap()).unwrap();
    abi["functions"]
        .as_array()
        .expect("functions array")
        .iter()
        .filter(|f| {
            let Some(headers) = enabled_headers else {
                return true;
            };
            let header = f["header"].as_str().unwrap_or_default();
            headers.contains(header)
        })
        .map(|f| f["name"].as_str().expect("function name string").to_owned())
        .collect()
}

fn candidate_features_paths(root: &Path) -> Vec<PathBuf> {
    let mut out = Vec::new();
    if let Ok(explicit) = std::env::var("OCCTL_FEATURES_PATH") {
        if !explicit.trim().is_empty() {
            out.push(PathBuf::from(explicit));
        }
    }
    out.push(
        root.join("build")
            .join("full-with-viz-shared")
            .join("OCCTLFeatures.json"),
    );
    out.push(
        root.join("build")
            .join("full-shared")
            .join("OCCTLFeatures.json"),
    );
    out.push(root.join("build").join("full").join("OCCTLFeatures.json"));
    out.push(root.join("build").join("cad").join("OCCTLFeatures.json"));
    out.push(
        root.join("build")
            .join("minimal")
            .join("OCCTLFeatures.json"),
    );
    out
}

fn enabled_headers_from_manifest(root: &Path) -> Option<BTreeSet<String>> {
    let mut features: Vec<String> = Vec::new();
    for candidate in candidate_features_paths(root) {
        if !candidate.is_file() {
            continue;
        }
        let Ok(text) = std::fs::read_to_string(&candidate) else {
            continue;
        };
        let Ok(doc) = serde_json::from_str::<Value>(&text) else {
            continue;
        };
        let Some(values) = doc["binding_features"].as_array() else {
            continue;
        };
        for value in values {
            if let Some(name) = value.as_str() {
                if !name.trim().is_empty() {
                    features.push(name.trim().to_owned());
                }
            }
        }
        if !features.is_empty() {
            break;
        }
    }
    if features.is_empty() {
        return None;
    }

    let mut headers = BTreeSet::new();
    headers.insert("occtl_core.h".to_owned());
    headers.insert("occtl.h".to_owned());
    for feature in features {
        match feature.as_str() {
            "geom" => {
                headers.insert("occtl_geom.h".to_owned());
                headers.insert("occtl_curves.h".to_owned());
                headers.insert("occtl_curves2d.h".to_owned());
                headers.insert("occtl_curves_common.h".to_owned());
                headers.insert("occtl_surfaces.h".to_owned());
            }
            "topo" => {
                headers.insert("occtl_topo.h".to_owned());
                headers.insert("occtl_topo_types.h".to_owned());
                headers.insert("occtl_topo_algo.h".to_owned());
                headers.insert("occtl_topo_build.h".to_owned());
                headers.insert("occtl_topo_relation.h".to_owned());
            }
            "prim" => {
                headers.insert("occtl_prim.h".to_owned());
                headers.insert("occtl_prim_solid.h".to_owned());
                headers.insert("occtl_prim_sketch.h".to_owned());
                headers.insert("occtl_prim_sweep.h".to_owned());
                headers.insert("occtl_prim_feature.h".to_owned());
            }
            "text" => {
                headers.insert("occtl_text.h".to_owned());
            }
            "bool" => {
                headers.insert("occtl_bool.h".to_owned());
            }
            "mesh" => {
                headers.insert("occtl_mesh.h".to_owned());
            }
            "heal" => {
                headers.insert("occtl_heal.h".to_owned());
            }
            "io_brep" => {
                headers.insert("occtl_io_brep.h".to_owned());
            }
            "io_step" => {
                headers.insert("occtl_io_step.h".to_owned());
            }
            "io_iges" => {
                headers.insert("occtl_io_iges.h".to_owned());
            }
            "io_stl" => {
                headers.insert("occtl_io_stl.h".to_owned());
            }
            "io_obj" => {
                headers.insert("occtl_io_obj.h".to_owned());
            }
            "io_gltf" => {
                headers.insert("occtl_io_gltf.h".to_owned());
            }
            "io_vrml" => {
                headers.insert("occtl_io_vrml.h".to_owned());
            }
            "io_ply" => {
                headers.insert("occtl_io_ply.h".to_owned());
            }
            "de" => {
                headers.insert("occtl_de.h".to_owned());
            }
            "viz" => {
                headers.insert("occtl_viz.h".to_owned());
            }
            _ => {}
        }
    }
    Some(headers)
}

fn enabled_headers_from_rust_cfg() -> BTreeSet<String> {
    let mut headers = BTreeSet::new();
    headers.insert("occtl.h".to_owned());
    headers.insert("occtl_core.h".to_owned());

    #[cfg(occtl_has_geom)]
    {
        headers.insert("occtl_geom.h".to_owned());
        headers.insert("occtl_curves.h".to_owned());
        headers.insert("occtl_curves2d.h".to_owned());
        headers.insert("occtl_curves_common.h".to_owned());
        headers.insert("occtl_surfaces.h".to_owned());
    }
    #[cfg(occtl_has_topo)]
    {
        headers.insert("occtl_topo.h".to_owned());
        headers.insert("occtl_topo_types.h".to_owned());
        headers.insert("occtl_topo_algo.h".to_owned());
        headers.insert("occtl_topo_build.h".to_owned());
        headers.insert("occtl_topo_relation.h".to_owned());
    }
    #[cfg(occtl_has_prim)]
    {
        headers.insert("occtl_prim.h".to_owned());
        headers.insert("occtl_prim_solid.h".to_owned());
        headers.insert("occtl_prim_sketch.h".to_owned());
        headers.insert("occtl_prim_sweep.h".to_owned());
        headers.insert("occtl_prim_feature.h".to_owned());
    }
    #[cfg(occtl_has_text)]
    {
        headers.insert("occtl_text.h".to_owned());
    }
    #[cfg(occtl_has_bool)]
    {
        headers.insert("occtl_bool.h".to_owned());
    }
    #[cfg(occtl_has_mesh)]
    {
        headers.insert("occtl_mesh.h".to_owned());
    }
    #[cfg(occtl_has_heal)]
    {
        headers.insert("occtl_heal.h".to_owned());
    }
    #[cfg(occtl_has_io_brep)]
    {
        headers.insert("occtl_io_brep.h".to_owned());
    }
    #[cfg(occtl_has_io_step)]
    {
        headers.insert("occtl_io_step.h".to_owned());
    }
    #[cfg(occtl_has_io_iges)]
    {
        headers.insert("occtl_io_iges.h".to_owned());
    }
    #[cfg(occtl_has_io_stl)]
    {
        headers.insert("occtl_io_stl.h".to_owned());
    }
    #[cfg(occtl_has_io_obj)]
    {
        headers.insert("occtl_io_obj.h".to_owned());
    }
    #[cfg(occtl_has_io_gltf)]
    {
        headers.insert("occtl_io_gltf.h".to_owned());
    }
    #[cfg(occtl_has_io_vrml)]
    {
        headers.insert("occtl_io_vrml.h".to_owned());
    }
    #[cfg(occtl_has_io_ply)]
    {
        headers.insert("occtl_io_ply.h".to_owned());
    }
    #[cfg(occtl_has_de)]
    {
        headers.insert("occtl_de.h".to_owned());
    }
    #[cfg(occtl_has_viz)]
    {
        headers.insert("occtl_viz.h".to_owned());
    }

    headers
}

fn bindgen_files(root: &Path) -> Vec<PathBuf> {
    let mut out = Vec::new();
    for profile in ["debug", "release"] {
        let build_dir = root
            .join("bindings")
            .join("rust")
            .join("target")
            .join(profile)
            .join("build");
        let Ok(entries) = std::fs::read_dir(build_dir) else {
            continue;
        };
        let mut newest: Option<(std::time::SystemTime, PathBuf)> = None;
        for entry in entries.flatten() {
            let path = entry.path().join("out").join("bindings.rs");
            let Some(name) = entry.file_name().to_str().map(str::to_owned) else {
                continue;
            };
            if name.starts_with("occtl-sys-") && path.is_file() {
                let modified = path
                    .metadata()
                    .and_then(|m| m.modified())
                    .unwrap_or(std::time::SystemTime::UNIX_EPOCH);
                if match newest.as_ref() {
                    Some((current, _)) => modified > *current,
                    None => true,
                } {
                    newest = Some((modified, path));
                }
            }
        }
        if let Some((_, path)) = newest {
            out.push(path);
        }
    }
    out.sort();
    out
}

fn bindgen_functions(path: &Path) -> BTreeSet<String> {
    let mut functions = BTreeSet::new();
    for line in std::fs::read_to_string(path).unwrap().lines() {
        let Some(rest) = line.trim_start().strip_prefix("pub fn ") else {
            continue;
        };
        let Some((name, _)) = rest.split_once('(') else {
            continue;
        };
        if name.starts_with("occtl_") {
            functions.insert(name.to_owned());
        }
    }
    functions
}

#[test]
fn bindgen_exposes_every_abi_function() {
    let root = repo_root();
    let path = root.join("build").join("abi.json");
    if !path.is_file() {
        eprintln!(
            "coverage: {} missing — run `python3 tools/abi_dump.py --output build/abi.json` first.",
            path.display()
        );
        return;
    }

    let enabled_headers_cfg = enabled_headers_from_rust_cfg();
    let enabled_headers = match enabled_headers_from_manifest(&root) {
        Some(from_manifest) => {
            let intersection = from_manifest
                .intersection(&enabled_headers_cfg)
                .cloned()
                .collect::<BTreeSet<_>>();
            Some(intersection)
        }
        None => Some(enabled_headers_cfg),
    };
    let expected = abi_functions(&path, enabled_headers.as_ref());
    assert!(
        expected.len() > 100,
        "abi.json had only {} functions",
        expected.len()
    );

    let mut actual = BTreeSet::new();
    let files = bindgen_files(&root);
    for file in &files {
        actual.extend(bindgen_functions(file));
    }
    assert!(
        !files.is_empty(),
        "coverage: no occtl-sys bindgen output found under bindings/rust/target/*/build"
    );

    let missing: Vec<_> = expected.difference(&actual).cloned().collect();
    let extra: Vec<_> = actual.difference(&expected).cloned().collect();
    assert!(
        missing.is_empty(),
        "Rust raw binding coverage mismatch: missing={:?}",
        missing
    );
    if !extra.is_empty() {
        eprintln!(
            "coverage: bindgen exposes {} additional symbols beyond the active feature-set expectation",
            extra.len()
        );
    }
    eprintln!(
        "coverage: {} C functions matched across {} bindgen output file(s)",
        expected.len(),
        files.len()
    );
}
