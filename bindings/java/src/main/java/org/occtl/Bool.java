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

import java.util.List;

import com.sun.jna.Memory;
import com.sun.jna.NativeLong;
import com.sun.jna.Pointer;

import org.occtl.generated.OcctlGenerated;
import org.occtl.generated.OcctlTypes;

/** Boolean operation helpers. */
public final class Bool {
    private Bool() {}

    public static NodeId fuse(Graph graph, List<NodeId> objects, List<NodeId> tools) {
        return runBinary(graph, objects, tools, BoolOptions.defaults(), Op.FUSE);
    }
    public static NodeId fuse(Graph graph, List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return runBinary(graph, objects, tools, options, Op.FUSE);
    }

    public static NodeId cut(Graph graph, List<NodeId> objects, List<NodeId> tools) {
        return runBinary(graph, objects, tools, BoolOptions.defaults(), Op.CUT);
    }
    public static NodeId cut(Graph graph, List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return runBinary(graph, objects, tools, options, Op.CUT);
    }

    public static NodeId common(Graph graph, List<NodeId> objects, List<NodeId> tools) {
        return runBinary(graph, objects, tools, BoolOptions.defaults(), Op.COMMON);
    }
    public static NodeId common(Graph graph, List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return runBinary(graph, objects, tools, options, Op.COMMON);
    }

    public static NodeId section(Graph graph, List<NodeId> objects, List<NodeId> tools) {
        return runBinary(graph, objects, tools, BoolOptions.defaults(), Op.SECTION);
    }
    public static NodeId section(Graph graph, List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return runBinary(graph, objects, tools, options, Op.SECTION);
    }

    public static NodeId split(Graph graph, List<NodeId> objects, List<NodeId> tools) {
        return runBinary(graph, objects, tools, BoolOptions.defaults(), Op.SPLIT);
    }
    public static NodeId split(Graph graph, List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return runBinary(graph, objects, tools, options, Op.SPLIT);
    }

    private enum Op { FUSE, CUT, COMMON, SECTION, SPLIT }

    private static NodeId runBinary(Graph graph, List<NodeId> objects, List<NodeId> tools, BoolOptions options, Op op) {
        String symbol = switch (op) {
            case FUSE -> "occtl_bool_fuse";
            case CUT -> "occtl_bool_cut";
            case COMMON -> "occtl_bool_common";
            case SECTION -> "occtl_bool_section";
            case SPLIT -> "occtl_bool_split";
        };
        OcctL.requireFeature("bool", symbol);

        Pointer objBuf = encodeNodeIds(objects);
        Pointer toolBuf = encodeNodeIds(tools);

        OcctlTypes.OcctlBoolOptions nativeOpts = new OcctlTypes.OcctlBoolOptions();
        nativeOpts.struct_version = (int) OcctlGenerated.OCCTL_BOOL_OPTIONS_VERSION_1;
        nativeOpts.p_next = null;
        nativeOpts.fuzzy_value = options.fuzzyValue();
        nativeOpts.run_parallel = options.runParallel() ? 1 : 0;
        nativeOpts.simplify_result = options.simplifyResult() ? 1 : 0;
        nativeOpts.simplify_angular_tolerance = options.simplifyAngularTolerance();
        nativeOpts.build_history = options.buildHistory() ? 1 : 0;
        nativeOpts.write();

        Memory outRoot = new Memory(8);
        int status;
        switch (op) {
            case FUSE -> status = OcctL.RAW.occtl_bool_fuse(graph.nativeHandle(), objBuf, new NativeLong(objects.size()), toolBuf, new NativeLong(tools.size()), nativeOpts.getPointer(), outRoot);
            case CUT -> status = OcctL.RAW.occtl_bool_cut(graph.nativeHandle(), objBuf, new NativeLong(objects.size()), toolBuf, new NativeLong(tools.size()), nativeOpts.getPointer(), outRoot);
            case COMMON -> status = OcctL.RAW.occtl_bool_common(graph.nativeHandle(), objBuf, new NativeLong(objects.size()), toolBuf, new NativeLong(tools.size()), nativeOpts.getPointer(), outRoot);
            case SECTION -> status = OcctL.RAW.occtl_bool_section(graph.nativeHandle(), objBuf, new NativeLong(objects.size()), toolBuf, new NativeLong(tools.size()), nativeOpts.getPointer(), outRoot);
            case SPLIT -> status = OcctL.RAW.occtl_bool_split(graph.nativeHandle(), objBuf, new NativeLong(objects.size()), toolBuf, new NativeLong(tools.size()), nativeOpts.getPointer(), outRoot);
            default -> throw new IllegalStateException("Unexpected op " + op);
        }
        OcctL.check(status);
        return new NodeId(outRoot.getLong(0));
    }

    private static Pointer encodeNodeIds(List<NodeId> ids) {
        if (ids == null || ids.isEmpty()) {
            return Pointer.NULL;
        }
        Memory mem = new Memory(ids.size() * 8L);
        for (int i = 0; i < ids.size(); ++i) {
            mem.setLong(i * 8L, ids.get(i).bits());
        }
        return mem;
    }
}
