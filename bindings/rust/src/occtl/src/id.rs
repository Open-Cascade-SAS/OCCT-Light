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

#[cfg(occtl_has_topo)]
use occtl_sys::{occtl_node_id_t, occtl_ref_id_t, occtl_ref_uid_t, occtl_rep_uid_t};
use occtl_sys::{occtl_rep_id_t, occtl_uid_t};

macro_rules! impl_id {
    ($name:ident, $raw:ty) => {
        #[derive(Copy, Clone, Debug, PartialEq, Eq, Hash, Default)]
        #[repr(transparent)]
        pub struct $name(pub $raw);

        impl $name {
            #[inline]
            pub fn invalid() -> Self {
                Self(<$raw>::default())
            }
            #[inline]
            pub fn is_valid(self) -> bool {
                self.0.bits != 0
            }
            #[inline]
            pub fn bits(self) -> u64 {
                self.0.bits
            }
        }

        impl From<$raw> for $name {
            fn from(raw: $raw) -> Self {
                Self(raw)
            }
        }

        impl From<$name> for $raw {
            fn from(id: $name) -> Self {
                id.0
            }
        }
    };
}

impl_id!(RepId, occtl_rep_id_t);
impl_id!(Uid, occtl_uid_t);
#[cfg(occtl_has_topo)]
impl_id!(NodeId, occtl_node_id_t);
#[cfg(occtl_has_topo)]
impl_id!(RefId, occtl_ref_id_t);
#[cfg(occtl_has_topo)]
impl_id!(RefUid, occtl_ref_uid_t);
#[cfg(occtl_has_topo)]
impl_id!(RepUid, occtl_rep_uid_t);

/// Mirror of `occtl_node_kind_t`.
#[cfg(occtl_has_topo)]
pub use occtl_sys::occtl_node_kind as NodeKind;

#[cfg(occtl_has_topo)]
impl NodeId {
    /// Short lowercase token matching the strings used by the parity
    /// scenarios (`"solid"`, `"face"`, ...).  Returns `"unknown"` for
    /// kinds we have not yet mapped.
    pub fn kind_string(kind: occtl_sys::occtl_node_kind_t) -> &'static str {
        use occtl_sys::occtl_node_kind::*;
        match kind {
            OCCTL_KIND_SOLID => "solid",
            OCCTL_KIND_SHELL => "shell",
            OCCTL_KIND_FACE => "face",
            OCCTL_KIND_WIRE => "wire",
            OCCTL_KIND_EDGE => "edge",
            OCCTL_KIND_VERTEX => "vertex",
            OCCTL_KIND_COMPOUND => "compound",
            OCCTL_KIND_COMPSOLID => "compsolid",
            OCCTL_KIND_COEDGE => "coedge",
            OCCTL_KIND_PRODUCT => "product",
            OCCTL_KIND_OCCURRENCE => "occurrence",
            _ => "unknown",
        }
    }
}
