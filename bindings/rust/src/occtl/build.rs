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

use std::env;
use std::path::{Path, PathBuf};

fn main() {
    println!("cargo:rerun-if-env-changed=OCCTL_FEATURES_PATH");
    println!("cargo:rerun-if-env-changed=OCCTL_REPO_ROOT");
    println!("cargo:rerun-if-env-changed=OCCTL_LIBRARY_PATH");

    for name in [
        "occtl_has_geom",
        "occtl_has_topo",
        "occtl_has_prim",
        "occtl_has_text",
        "occtl_has_bool",
        "occtl_has_mesh",
        "occtl_has_heal",
        "occtl_has_io_brep",
        "occtl_has_io_step",
        "occtl_has_io_iges",
        "occtl_has_io_stl",
        "occtl_has_io_obj",
        "occtl_has_io_gltf",
        "occtl_has_io_vrml",
        "occtl_has_io_ply",
        "occtl_has_de",
        "occtl_has_viz",
    ] {
        println!("cargo:rustc-check-cfg=cfg({})", name);
    }

    for feature in discover_binding_features() {
        if let Some(cfg_name) = feature_cfg_name(&feature) {
            println!("cargo:rustc-cfg={}", cfg_name);
        }
    }
}

fn discover_binding_features() -> Vec<String> {
    let repo_root = discover_repo_root();
    let mut candidates = Vec::new();
    if let Ok(path) = env::var("OCCTL_FEATURES_PATH") {
        candidates.push(PathBuf::from(path));
    }
    if let Ok(lib_dir) = env::var("OCCTL_LIBRARY_PATH") {
        let lib_path = PathBuf::from(lib_dir);
        candidates.push(
            lib_path
                .parent()
                .unwrap_or(&lib_path)
                .join("OCCTLFeatures.json"),
        );
    }
    for preset in [
        "minimal",
        "minimal-shared",
        "cad",
        "cad-shared",
        "full",
        "full-shared",
        "full-with-viz",
        "full-with-viz-shared",
    ] {
        candidates.push(
            repo_root
                .join("build")
                .join(preset)
                .join("OCCTLFeatures.json"),
        );
    }

    for path in candidates {
        if !path.is_file() {
            continue;
        }
        println!("cargo:rerun-if-changed={}", path.display());
        if let Ok(contents) = std::fs::read_to_string(&path) {
            if let Some(features) = parse_string_array_field(&contents, "binding_features") {
                if !features.is_empty() {
                    return features;
                }
            }
        }
    }

    Vec::new()
}

fn discover_repo_root() -> PathBuf {
    if let Ok(root) = env::var("OCCTL_REPO_ROOT") {
        return PathBuf::from(root);
    }
    let crate_dir = env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR");
    let crate_path = Path::new(&crate_dir);
    crate_path
        .ancestors()
        .nth(3)
        .map(Path::to_path_buf)
        .expect("crate path has at least three ancestors")
}

fn parse_string_array_field(contents: &str, field: &str) -> Option<Vec<String>> {
    let key = format!("\"{}\"", field);
    let key_pos = contents.find(&key)?;
    let start_bracket = contents[key_pos..].find('[')? + key_pos;
    let end_bracket = contents[start_bracket..].find(']')? + start_bracket;
    let body = &contents[start_bracket + 1..end_bracket];

    let mut out = Vec::new();
    let mut in_string = false;
    let mut token = String::new();
    let mut escaped = false;
    for ch in body.chars() {
        if in_string {
            if escaped {
                token.push(ch);
                escaped = false;
                continue;
            }
            match ch {
                '\\' => escaped = true,
                '"' => {
                    out.push(token.clone());
                    token.clear();
                    in_string = false;
                }
                _ => token.push(ch),
            }
        } else if ch == '"' {
            in_string = true;
        }
    }
    Some(out)
}

fn feature_cfg_name(feature: &str) -> Option<&'static str> {
    match feature {
        "geom" => Some("occtl_has_geom"),
        "topo" => Some("occtl_has_topo"),
        "prim" => Some("occtl_has_prim"),
        "text" => Some("occtl_has_text"),
        "bool" => Some("occtl_has_bool"),
        "mesh" => Some("occtl_has_mesh"),
        "heal" => Some("occtl_has_heal"),
        "io_brep" => Some("occtl_has_io_brep"),
        "io_step" => Some("occtl_has_io_step"),
        "io_iges" => Some("occtl_has_io_iges"),
        "io_stl" => Some("occtl_has_io_stl"),
        "io_obj" => Some("occtl_has_io_obj"),
        "io_gltf" => Some("occtl_has_io_gltf"),
        "io_vrml" => Some("occtl_has_io_vrml"),
        "io_ply" => Some("occtl_has_io_ply"),
        "de" => Some("occtl_has_de"),
        "viz" => Some("occtl_has_viz"),
        _ => None,
    }
}
