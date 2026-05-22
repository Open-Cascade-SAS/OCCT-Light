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

use std::ffi::CStr;
use std::fmt;

/// Mirror of `occtl_status_t`.  See `include/occtl/occtl_core.h`.
pub type Status = occtl_sys::occtl_status_t;

/// Structured error from the OCCT-Light C ABI.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error {
    pub status: Status,
    pub message: String,
}

impl Error {
    /// Reads `occtl_error_last()` immediately and builds an `Error`.
    pub fn current(status: Status) -> Self {
        let message = unsafe {
            let info = occtl_sys::occtl_error_last();
            if info.is_null() {
                String::new()
            } else {
                let msg_ptr = (*info).message;
                if msg_ptr.is_null() {
                    String::new()
                } else {
                    CStr::from_ptr(msg_ptr).to_string_lossy().into_owned()
                }
            }
        };
        Self { status, message }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let name = unsafe {
            let p = occtl_sys::occtl_status_to_string(self.status);
            if p.is_null() {
                "OCCTL_UNKNOWN"
            } else {
                CStr::from_ptr(p).to_str().unwrap_or("OCCTL_UNKNOWN")
            }
        };
        if self.message.is_empty() {
            write!(f, "{} ({})", name, self.status as i32)
        } else {
            write!(f, "{} ({}): {}", name, self.status as i32, self.message)
        }
    }
}

impl std::error::Error for Error {}

pub type Result<T> = std::result::Result<T, Error>;

/// Translate a C-ABI status into `Result<()>`.
#[inline]
pub fn check(status: Status) -> Result<()> {
    if status == occtl_sys::occtl_status::OCCTL_OK {
        Ok(())
    } else {
        Err(Error::current(status))
    }
}
