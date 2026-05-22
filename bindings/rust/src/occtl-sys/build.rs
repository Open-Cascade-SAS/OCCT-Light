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
    println!("cargo:rerun-if-env-changed=OCCTL_INCLUDE_DIR");
    println!("cargo:rerun-if-env-changed=OCCTL_REPO_ROOT");
    println!("cargo:rerun-if-env-changed=OCCTL_LIBRARY_PATH");
    println!("cargo:rerun-if-env-changed=OCCTL_LIBRARY_NAME");

    let repo_root = discover_repo_root();
    let include_dir = env::var("OCCTL_INCLUDE_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| repo_root.join("include"));

    let umbrella = include_dir.join("occtl").join("occtl.h");
    if !umbrella.is_file() {
        panic!(
            "occtl-sys: cannot find {} — set OCCTL_INCLUDE_DIR or OCCTL_REPO_ROOT.",
            umbrella.display()
        );
    }

    println!("cargo:rerun-if-changed={}", umbrella.display());
    for header in std::fs::read_dir(include_dir.join("occtl"))
        .expect("read include/occtl/")
        .flatten()
    {
        println!("cargo:rerun-if-changed={}", header.path().display());
    }

    let (lib_dir, lib_name) = discover_library(&repo_root);
    let binding_features = discover_binding_features(&repo_root, &lib_dir);
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=dylib={}", lib_name);

    // On macOS the dylibs link with an install_name; set rpath fallback so
    // tests can resolve without DYLD_LIBRARY_PATH overrides every time.
    if let Ok(lib_dir) = env::var("OCCTL_LIBRARY_PATH") {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", lib_dir);
    }

    let mut bindgen_builder = bindgen::Builder::default()
        .header(umbrella.to_string_lossy().to_string())
        .clang_arg(format!("-I{}", include_dir.display()))
        .allowlist_function("occtl_.*")
        .allowlist_type("occtl_.*|OCCTL_.*|Occtl.*")
        .allowlist_var("OCCTL_.*")
        .default_enum_style(bindgen::EnumVariation::Rust {
            non_exhaustive: false,
        })
        .derive_debug(true)
        .derive_default(true)
        .derive_eq(true)
        .derive_hash(true)
        .derive_copy(true)
        .layout_tests(false);

    for feature in &binding_features {
        if let Some(macro_name) = feature_macro(feature) {
            bindgen_builder = bindgen_builder.clang_arg(format!("-D{}", macro_name));
        }
    }

    let bindings = bindgen_builder
        .generate()
        .expect("bindgen: failed to generate raw FFI bindings");

    let out = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR"));
    bindings
        .write_to_file(out.join("bindings.rs"))
        .expect("bindgen: write bindings.rs");
}

fn discover_library(repo_root: &Path) -> (PathBuf, String) {
    let preferred = env::var("OCCTL_LIBRARY_NAME")
        .ok()
        .map(|name| normalize_library_token(&name));
    let names: Vec<String> = preferred.map(|name| vec![name]).unwrap_or_else(|| {
        [
            "occtl-full-viz",
            "occtl-full",
            "occtl-cad",
            "occtl-minimal",
            "occtl-geom",
            "occtl-core",
            "occtl-custom",
        ]
        .iter()
        .map(|s| s.to_string())
        .collect()
    });

    let mut dirs = Vec::new();
    if let Ok(lib_dir) = env::var("OCCTL_LIBRARY_PATH") {
        dirs.push(PathBuf::from(lib_dir));
    }
    for preset in &[
        "full-with-viz",
        "full",
        "cad",
        "minimal",
        "geom-only",
        "core-only",
    ] {
        dirs.push(repo_root.join("build").join(preset).join("lib"));
    }

    for dir in dirs {
        if !dir.is_dir() {
            continue;
        }
        for name in &names {
            if library_exists(&dir, name) {
                return (dir, name.clone());
            }
        }
    }

    panic!("occtl-sys: cannot find a libocctl-<feature-set> library; set OCCTL_LIBRARY_PATH or OCCTL_LIBRARY_NAME");
}

fn normalize_library_token(raw_name: &str) -> String {
    let mut name = raw_name.trim().to_owned();
    for suffix in [".dylib", ".so", ".dll", ".lib"] {
        if name.to_ascii_lowercase().ends_with(suffix) {
            let cut = name.len().saturating_sub(suffix.len());
            name.truncate(cut);
            break;
        }
    }
    if name.to_ascii_lowercase().starts_with("lib") {
        name = name[3..].to_owned();
    }
    if !name.to_ascii_lowercase().starts_with("occtl-") {
        name = format!("occtl-{}", name);
    }
    name
}

fn discover_binding_features(repo_root: &Path, lib_dir: &Path) -> Vec<String> {
    println!("cargo:rerun-if-env-changed=OCCTL_FEATURES_PATH");

    let mut candidates = Vec::new();
    if let Ok(path) = env::var("OCCTL_FEATURES_PATH") {
        candidates.push(PathBuf::from(path));
    }
    candidates.push(
        lib_dir
            .parent()
            .unwrap_or(lib_dir)
            .join("OCCTLFeatures.json"),
    );
    candidates.push(
        repo_root
            .join("build")
            .join("full")
            .join("OCCTLFeatures.json"),
    );

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

fn feature_macro(feature: &str) -> Option<&'static str> {
    match feature {
        "geom" => Some("OCCTL_HAS_GEOM"),
        "topo" => Some("OCCTL_HAS_TOPO"),
        "prim" => Some("OCCTL_HAS_PRIM"),
        "text" => Some("OCCTL_HAS_TEXT"),
        "bool" => Some("OCCTL_HAS_BOOL"),
        "mesh" => Some("OCCTL_HAS_MESH"),
        "heal" => Some("OCCTL_HAS_HEAL"),
        "io_brep" => Some("OCCTL_HAS_IO_BREP"),
        "io_step" => Some("OCCTL_HAS_IO_STEP"),
        "io_iges" => Some("OCCTL_HAS_IO_IGES"),
        "io_stl" => Some("OCCTL_HAS_IO_STL"),
        "io_obj" => Some("OCCTL_HAS_IO_OBJ"),
        "io_gltf" => Some("OCCTL_HAS_IO_GLTF"),
        "io_vrml" => Some("OCCTL_HAS_IO_VRML"),
        "io_ply" => Some("OCCTL_HAS_IO_PLY"),
        "de" => Some("OCCTL_HAS_DE"),
        "viz" => Some("OCCTL_HAS_VIZ"),
        _ => None,
    }
}

fn library_exists(dir: &Path, name: &str) -> bool {
    let candidates = [
        format!("lib{}.dylib", name),
        format!("lib{}.so", name),
        format!("{}.dll", name),
        format!("{}.lib", name),
        format!("lib{}.a", name),
    ];
    candidates
        .iter()
        .any(|candidate| dir.join(candidate).is_file())
}

fn discover_repo_root() -> PathBuf {
    if let Ok(root) = env::var("OCCTL_REPO_ROOT") {
        return PathBuf::from(root);
    }
    // Crate is at <root>/bindings/rust/src/occtl-sys.
    let crate_dir = env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR");
    let crate_path = Path::new(&crate_dir);
    crate_path
        .ancestors()
        .nth(4)
        .map(Path::to_path_buf)
        .expect("crate path has at least four ancestors")
}
