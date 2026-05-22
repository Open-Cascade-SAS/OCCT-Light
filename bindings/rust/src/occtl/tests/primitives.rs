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

#![cfg(all(occtl_has_topo, occtl_has_prim, occtl_has_bool))]

use occtl::{bool_::BoolOptions, Graph};

#[test]
fn graph_primitive_helpers_build_solids() {
    let graph = Graph::create().expect("create graph");

    assert!(graph.make_box(1.0, 2.0, 3.0).expect("box").is_valid());
    assert!(graph.make_sphere(2.0).expect("sphere").is_valid());
    assert!(graph.make_cylinder(1.0, 4.0).expect("cylinder").is_valid());
    assert!(graph.make_cone(2.0, 1.0, 5.0).expect("cone").is_valid());
    assert!(graph.make_torus(5.0, 1.0).expect("torus").is_valid());
    assert!(graph
        .make_wedge(4.0, 3.0, 2.0, 1.0)
        .expect("wedge")
        .is_valid());

    assert_eq!(graph.solid_count(), 6);
    assert!(graph.check_issues().expect("check issues").is_empty());
    assert!(graph.is_valid().expect("is valid"));

    let left = graph.make_box(10.0, 10.0, 10.0).expect("left");
    let right = graph.make_box(10.0, 10.0, 10.0).expect("right");
    let fused = graph.fuse(&[left], &[right]).expect("fuse");
    assert!(fused.is_valid());

    let fused_with_opts = graph
        .fuse_with_options(
            &[left],
            &[right],
            BoolOptions {
                run_parallel: true,
                build_history: false,
                ..BoolOptions::default()
            },
        )
        .expect("fuse_with_options");
    assert!(fused_with_opts.is_valid());
}

#[test]
fn graph_from_pointer_rejects_null() {
    let result = unsafe { Graph::from_pointer_unsafe(std::ptr::null_mut()) };
    match result {
        Ok(_) => panic!("null pointer must fail"),
        Err(err) => assert_eq!(err.status, occtl::sys::occtl_status::OCCTL_INVALID_HANDLE),
    }
}
