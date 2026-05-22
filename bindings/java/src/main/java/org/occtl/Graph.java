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

import java.util.ArrayList;
import java.util.List;

import com.sun.jna.Memory;
import com.sun.jna.NativeLong;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.NativeLongByReference;
import com.sun.jna.ptr.PointerByReference;

import org.occtl.generated.OcctlGenerated;

/** Owns an occtl_graph_t* handle and exposes common idiomatic graph operations. */
public final class Graph implements AutoCloseable {
    private Pointer handle;

    private Graph(Pointer adoptedHandle) {
        this.handle = adoptedHandle;
    }

    public Graph() {
        OcctL.ensureInitialized();
        OcctL.requireFeature("topo", "occtl_graph_create");
        PointerByReference out = new PointerByReference();
        OcctL.check(OcctL.RAW.occtl_graph_create(out));
        this.handle = out.getValue();
    }

    /** Creates an empty graph. */
    public static Graph create() {
        return new Graph();
    }

    /**
     * Adopts an existing native graph pointer.
     *
     * <p>Security/ownership contract:
     * <ul>
     *   <li>{@code raw} must be a live {@code occtl_graph_t*} from the same loaded OCCT-Light library.</li>
     *   <li>Ownership transfers to Java; this Graph will call {@code occtl_graph_free} exactly once.</li>
     *   <li>Passing an invalid, borrowed, or already-owned pointer can corrupt memory.</li>
     * </ul>
     */
    public static Graph fromPointerUnsafe(Pointer raw) {
        OcctL.ensureInitialized();
        OcctL.requireFeature("topo", "occtl_graph_free");
        if (raw == null || Pointer.nativeValue(raw) == 0L) {
            throw new OcctLException(
                OcctlGenerated.OCCTL_INVALID_HANDLE,
                "Graph.fromPointerUnsafe received a NULL pointer");
        }
        return new Graph(raw);
    }

    Pointer nativeHandle() {
        if (handle == null) {
            throw new IllegalStateException("Graph is already closed");
        }
        return handle;
    }

    @Override
    public void close() {
        if (handle != null) {
            OcctL.RAW.occtl_graph_free(handle);
            handle = null;
        }
    }

    public int faceCount() {
        NativeLongByReference out = new NativeLongByReference();
        OcctL.check(OcctL.RAW.occtl_graph_face_count(nativeHandle(), out));
        return Math.toIntExact(out.getValue().longValue());
    }

    public int edgeCount() {
        NativeLongByReference out = new NativeLongByReference();
        OcctL.check(OcctL.RAW.occtl_graph_edge_count(nativeHandle(), out));
        return Math.toIntExact(out.getValue().longValue());
    }

    public int vertexCount() {
        NativeLongByReference out = new NativeLongByReference();
        OcctL.check(OcctL.RAW.occtl_graph_vertex_count(nativeHandle(), out));
        return Math.toIntExact(out.getValue().longValue());
    }

    public int solidCount() {
        NativeLongByReference out = new NativeLongByReference();
        OcctL.check(OcctL.RAW.occtl_graph_solid_count(nativeHandle(), out));
        return Math.toIntExact(out.getValue().longValue());
    }

    public int compoundCount() {
        NativeLongByReference out = new NativeLongByReference();
        OcctL.check(OcctL.RAW.occtl_graph_compound_count(nativeHandle(), out));
        return Math.toIntExact(out.getValue().longValue());
    }

    public List<NodeId> faceIds() {
        PointerByReference outIter = new PointerByReference();
        OcctL.check(OcctL.RAW.occtl_graph_face_iter_create(nativeHandle(), outIter));
        Pointer iter = outIter.getValue();

        ArrayList<NodeId> out = new ArrayList<>();
        try {
            Memory outId = new Memory(8);
            while (true) {
                int status = OcctL.RAW.occtl_node_iter_next(iter, outId);
                if (status == OcctlGenerated.OCCTL_NOT_FOUND) {
                    break;
                }
                OcctL.check(status);
                out.add(new NodeId(outId.getLong(0)));
            }
        } finally {
            OcctL.RAW.occtl_node_iter_free(iter);
        }
        return out;
    }

    public Uid uidOf(NodeId id) {
        Memory outUid = new Memory(8);
        OcctL.check(OcctL.RAW.occtl_graph_uid_from_node_id(nativeHandle(), id.bits(), outUid));
        return new Uid(outUid.getLong(0));
    }

    public int nodeKind(NodeId id) {
        IntByReference out = new IntByReference();
        OcctL.check(OcctL.RAW.occtl_graph_node_kind(nativeHandle(), id.bits(), out));
        return out.getValue();
    }

    public List<Uid> historyModified(Uid input) {
        return historyLookup(true, input);
    }

    public List<Uid> historyGenerated(Uid input) {
        return historyLookup(false, input);
    }

    /** Builds an axis-aligned box and returns the new solid node id. */
    public NodeId makeBox(double dx, double dy, double dz) {
        return Prim.makeBox(this, dx, dy, dz);
    }

    /** Builds a sphere and returns the new solid node id. */
    public NodeId makeSphere(double radius) {
        return Prim.makeSphere(this, radius);
    }

    /** Builds a cylinder and returns the new solid node id. */
    public NodeId makeCylinder(double radius, double height) {
        return Prim.makeCylinder(this, radius, height);
    }

    /** Builds a cone/truncated cone and returns the new solid node id. */
    public NodeId makeCone(double r1, double r2, double height) {
        return Prim.makeCone(this, r1, r2, height);
    }

    /** Builds a torus and returns the new solid node id. */
    public NodeId makeTorus(double r1, double r2) {
        return Prim.makeTorus(this, r1, r2);
    }

    /** Builds a wedge and returns the new solid node id. */
    public NodeId makeWedge(double dx, double dy, double dz, double ltx) {
        return Prim.makeWedge(this, dx, dy, dz, ltx);
    }

    /** Runs boolean fuse and returns the resulting node id. */
    public NodeId fuse(List<NodeId> objects, List<NodeId> tools) {
        return Bool.fuse(this, objects, tools);
    }
    /** Runs boolean fuse with explicit options and returns the resulting node id. */
    public NodeId fuse(List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return Bool.fuse(this, objects, tools, options);
    }

    /** Runs boolean cut and returns the resulting node id. */
    public NodeId cut(List<NodeId> objects, List<NodeId> tools) {
        return Bool.cut(this, objects, tools);
    }
    /** Runs boolean cut with explicit options and returns the resulting node id. */
    public NodeId cut(List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return Bool.cut(this, objects, tools, options);
    }

    /** Runs boolean common and returns the resulting node id. */
    public NodeId common(List<NodeId> objects, List<NodeId> tools) {
        return Bool.common(this, objects, tools);
    }
    /** Runs boolean common with explicit options and returns the resulting node id. */
    public NodeId common(List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return Bool.common(this, objects, tools, options);
    }

    /** Runs boolean section and returns the resulting node id. */
    public NodeId section(List<NodeId> objects, List<NodeId> tools) {
        return Bool.section(this, objects, tools);
    }
    /** Runs boolean section with explicit options and returns the resulting node id. */
    public NodeId section(List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return Bool.section(this, objects, tools, options);
    }

    /** Runs boolean split and returns the resulting node id. */
    public NodeId split(List<NodeId> objects, List<NodeId> tools) {
        return Bool.split(this, objects, tools);
    }
    /** Runs boolean split with explicit options and returns the resulting node id. */
    public NodeId split(List<NodeId> objects, List<NodeId> tools, BoolOptions options) {
        return Bool.split(this, objects, tools, options);
    }

    private List<Uid> historyLookup(boolean modified, Uid input) {
        NativeLongByReference outCount = new NativeLongByReference();
        int status;
        if (modified) {
            status = OcctL.RAW.occtl_graph_history_modified(nativeHandle(), input.bits(), Pointer.NULL, new NativeLong(0), outCount);
        } else {
            status = OcctL.RAW.occtl_graph_history_generated(nativeHandle(), input.bits(), Pointer.NULL, new NativeLong(0), outCount);
        }
        OcctL.check(status);

        long count = outCount.getValue().longValue();
        if (count == 0L) {
            return List.of();
        }

        Memory outBuf = new Memory(count * 8L);
        if (modified) {
            status = OcctL.RAW.occtl_graph_history_modified(nativeHandle(), input.bits(), outBuf, new NativeLong(count), outCount);
        } else {
            status = OcctL.RAW.occtl_graph_history_generated(nativeHandle(), input.bits(), outBuf, new NativeLong(count), outCount);
        }
        OcctL.check(status);

        int n = Math.toIntExact(outCount.getValue().longValue());
        ArrayList<Uid> values = new ArrayList<>(n);
        for (int i = 0; i < n; ++i) {
            values.add(new Uid(outBuf.getLong(i * 8L)));
        }
        return values;
    }
}
