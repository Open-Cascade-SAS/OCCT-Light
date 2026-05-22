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

package org.occtl;

/** Axis-2 placement used by primitive creation options. */
public record Axis2Placement(Point3 location, Direction3 xDir, Direction3 xDirRef) {
    public static Axis2Placement identity() {
        return new Axis2Placement(
            new Point3(0.0, 0.0, 0.0),
            new Direction3(1.0, 0.0, 0.0),
            new Direction3(0.0, 1.0, 0.0)
        );
    }
}
