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

package org.occtl.tests;

import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.assertThrows;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.occtl.BoxInfo;
import org.occtl.BoolOptions;
import org.occtl.Graph;
import org.occtl.NodeId;
import org.occtl.OcctLException;
import org.occtl.Prim;
import com.sun.jna.Pointer;

/** Tier-1 smoke test for the Java binding. */
public class SmokeTest {

    @Test
    public void createGraphBuildBoxAndCountEntities() {
        try (Graph graph = Graph.create()) {
            NodeId a = graph.makeBox(10.0, 10.0, 5.0);
            NodeId b = graph.makeBox(10.0, 10.0, 5.0);
            graph.fuse(List.of(a), List.of(b), new BoolOptions(0.0, true, false, 1.0e-2, false));
            assertTrue(graph.faceCount() >= 6);
            assertTrue(graph.edgeCount() >= 12);
            assertTrue(graph.vertexCount() >= 8);
            assertTrue(graph.solidCount() >= 1);
        }
    }

    @Test
    public void fromPointerUnsafeRejectsNull() {
        assertThrows(OcctLException.class, () -> Graph.fromPointerUnsafe(Pointer.NULL));
    }
}
