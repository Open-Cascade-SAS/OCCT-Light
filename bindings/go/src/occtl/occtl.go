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

/*
#include <occtl/occtl_core.h>
*/
import "C"
import (
	"fmt"
	"sync"
)

var initOnce sync.Once

func ensureInit() {
	initOnce.Do(func() {
		var info C.occtl_runtime_init_info_t
		C.occtl_runtime_init_info_init(&info)
		_ = C.occtl_runtime_init(&info)
	})
}

// AbiVersion returns the runtime ABI version compiled into the loaded library.
func AbiVersion() uint32 {
	ensureInit()
	return uint32(C.occtl_runtime_abi_version())
}

// Status mirrors occtl_status_t.
type Status int32

const (
	StatusOK              Status = 0
	StatusError           Status = 1
	StatusInvalidArgument Status = 2
	StatusInvalidHandle   Status = 3
	StatusNotFound        Status = 4
	StatusFormatError     Status = 11
	StatusUnsupported     Status = 12
	StatusCancelled       Status = 13
	StatusBufferTooSmall  Status = 14
	StatusVersionMismatch Status = 15
	StatusInternal        Status = 16
	StatusWrongKind       Status = 17
)

// Error wraps a non-OK status code together with the thread-local message.
type Error struct {
	Status  Status
	Message string
}

func (e *Error) Error() string {
	name := C.GoString(C.occtl_status_to_string(C.occtl_status_t(e.Status)))
	if e.Message == "" {
		return fmt.Sprintf("%s (%d)", name, int32(e.Status))
	}
	return fmt.Sprintf("%s (%d): %s", name, int32(e.Status), e.Message)
}

func check(st C.occtl_status_t) error {
	if Status(st) == StatusOK {
		return nil
	}
	info := C.occtl_error_last()
	msg := ""
	if info != nil && info.message != nil {
		msg = C.GoString(info.message)
	}
	return &Error{Status: Status(st), Message: msg}
}
