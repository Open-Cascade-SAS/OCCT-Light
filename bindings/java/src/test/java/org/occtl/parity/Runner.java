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

package org.occtl.parity;

import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import org.occtl.Axis2Placement;
import org.occtl.Bool;
import org.occtl.BoxInfo;
import org.occtl.Direction3;
import org.occtl.Graph;
import org.occtl.NodeId;
import org.occtl.Point3;
import org.occtl.Prim;
import org.occtl.Uid;
import org.occtl.generated.OcctlGenerated;

/** Parity runner that mirrors tests/binding_parity scenario behavior. */
public final class Runner {
    private Runner() {}

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("usage: java org.occtl.parity.Runner <scenario.json>");
            System.exit(2);
        }

        ObjectMapper mapper = new ObjectMapper();
        JsonNode scenario = mapper.readTree(Path.of(args[0]).toFile());
        String scenarioName = scenario.get("scenario").asText();

        Map<String, Object> actual = runScenario(scenarioName, scenario);
        boolean matches = matchesExpected(scenario.get("expected"), actual);

        Map<String, Object> out = new LinkedHashMap<>();
        out.put("scenario", scenarioName);
        out.put("binding", "java");
        out.put("actual", actual);
        out.put("matches", matches);

        System.out.println(mapper.writeValueAsString(out));
        if (!matches) {
            System.exit(1);
        }
    }

    private static Map<String, Object> runScenario(String name, JsonNode scenario) {
        return switch (name) {
            case "build_box" -> runBuildBox(scenario);
            case "fuse_two_boxes" -> runFuseTwoBoxes();
            case "cut_box_corner" -> runCutBoxCorner();
            case "common_two_overlapping_boxes" -> runCommonTwoOverlappingBoxes();
            case "section_two_overlapping_boxes" -> runSectionTwoOverlappingBoxes();
            case "split_box_by_box" -> runSplitBoxByBox();
            case "history_modified_after_fuse" -> runHistoryModifiedAfterFuse();
            default -> throw new IllegalArgumentException("Unsupported scenario: " + name);
        };
    }

    private static Map<String, Object> runBuildBox(JsonNode scenario) {
        JsonNode params = scenario.get("params");
        double dx = params != null && params.has("dx") ? params.get("dx").asDouble() : 10.0;
        double dy = params != null && params.has("dy") ? params.get("dy").asDouble() : 10.0;
        double dz = params != null && params.has("dz") ? params.get("dz").asDouble() : 5.0;

        try (Graph g = new Graph()) {
            Prim.makeBox(g, new BoxInfo(dx, dy, dz));
            Map<String, Object> actual = new LinkedHashMap<>();
            actual.put("face_count", g.faceCount());
            actual.put("edge_count", g.edgeCount());
            actual.put("vertex_count", g.vertexCount());
            actual.put("solid_count", g.solidCount());
            return actual;
        }
    }

    private static Map<String, Object> runFuseTwoBoxes() {
        try (Graph g = new Graph()) {
            NodeId a = Prim.makeBox(g, new BoxInfo(10.0, 10.0, 10.0));
            NodeId b = Prim.makeBox(g, movedBox(10.0, 10.0, 10.0, 5.0, 0.0, 0.0));
            List<Uid> faceUidsA = g.faceIds().stream().limit(6).map(g::uidOf).toList();

            NodeId root = Bool.fuse(g, List.of(a), List.of(b));

            boolean historyNonEmpty = false;
            for (Uid uid : faceUidsA) {
                if (!g.historyModified(uid).isEmpty() || !g.historyGenerated(uid).isEmpty()) {
                    historyNonEmpty = true;
                    break;
                }
            }

            Map<String, Object> actual = new LinkedHashMap<>();
            actual.put("root_kind", kindString(g.nodeKind(root)));
            actual.put("history_modified_nonempty_on_first_face_of_box_a", historyNonEmpty);
            return actual;
        }
    }

    private static Map<String, Object> runCutBoxCorner() {
        try (Graph g = new Graph()) {
            NodeId box = Prim.makeBox(g, new BoxInfo(10.0, 10.0, 10.0));
            NodeId tool = Prim.makeBox(g, movedBox(5.0, 5.0, 5.0, 7.5, 7.5, 7.5));
            NodeId root = Bool.cut(g, List.of(box), List.of(tool));

            Map<String, Object> actual = new LinkedHashMap<>();
            actual.put("root_kind", kindString(g.nodeKind(root)));
            actual.put("solid_count", g.solidCount());
            return actual;
        }
    }

    private static Map<String, Object> runCommonTwoOverlappingBoxes() {
        try (Graph g = new Graph()) {
            NodeId a = Prim.makeBox(g, new BoxInfo(10.0, 10.0, 10.0));
            NodeId b = Prim.makeBox(g, movedBox(10.0, 10.0, 10.0, 5.0, 5.0, 5.0));
            NodeId root = Bool.common(g, List.of(a), List.of(b));

            Map<String, Object> actual = new LinkedHashMap<>();
            actual.put("root_kind", kindString(g.nodeKind(root)));
            actual.put("solid_count", g.solidCount());
            return actual;
        }
    }

    private static Map<String, Object> runSectionTwoOverlappingBoxes() {
        try (Graph g = new Graph()) {
            NodeId a = Prim.makeBox(g, new BoxInfo(10.0, 10.0, 10.0));
            NodeId b = Prim.makeBox(g, movedBox(10.0, 10.0, 10.0, 5.0, 0.0, 0.0));
            int before = g.edgeCount();
            NodeId root = Bool.section(g, List.of(a), List.of(b));

            Map<String, Object> actual = new LinkedHashMap<>();
            actual.put("root_kind", kindString(g.nodeKind(root)));
            actual.put("edge_count_increased", g.edgeCount() > before);
            return actual;
        }
    }

    private static Map<String, Object> runSplitBoxByBox() {
        try (Graph g = new Graph()) {
            NodeId a = Prim.makeBox(g, new BoxInfo(10.0, 10.0, 10.0));
            NodeId b = Prim.makeBox(g, movedBox(10.0, 10.0, 10.0, 0.0, 0.0, 5.0));
            NodeId root = Bool.split(g, List.of(a), List.of(b));

            Map<String, Object> actual = new LinkedHashMap<>();
            actual.put("root_kind", kindString(g.nodeKind(root)));
            actual.put("compound_count", g.compoundCount());
            actual.put("solid_count", g.solidCount());
            return actual;
        }
    }

    private static Map<String, Object> runHistoryModifiedAfterFuse() {
        try (Graph g = new Graph()) {
            NodeId a = Prim.makeBox(g, new BoxInfo(10.0, 10.0, 10.0));
            NodeId b = Prim.makeBox(g, movedBox(10.0, 10.0, 10.0, 5.0, 0.0, 0.0));
            List<Uid> all = g.faceIds().stream().limit(12).map(g::uidOf).toList();
            List<Uid> aUids = all.subList(0, Math.min(6, all.size()));
            List<Uid> bUids = all.subList(Math.min(6, all.size()), Math.min(12, all.size()));

            Bool.fuse(g, List.of(a), List.of(b));

            Map<String, Object> actual = new LinkedHashMap<>();
            actual.put("box_a_modified_nonempty", anyModified(g, aUids));
            actual.put("box_b_modified_nonempty", anyModified(g, bUids));
            return actual;
        }
    }

    private static boolean anyModified(Graph g, List<Uid> uids) {
        for (Uid uid : uids) {
            if (!g.historyModified(uid).isEmpty() || !g.historyGenerated(uid).isEmpty()) {
                return true;
            }
        }
        return false;
    }

    private static BoxInfo movedBox(double dx, double dy, double dz, double x, double y, double z) {
        Axis2Placement placement = new Axis2Placement(
            new Point3(x, y, z),
            new Direction3(1.0, 0.0, 0.0),
            new Direction3(0.0, 1.0, 0.0)
        );
        return new BoxInfo(dx, dy, dz, placement);
    }

    private static String kindString(int kind) {
        return switch (kind) {
            case OcctlGenerated.OCCTL_KIND_SOLID -> "solid";
            case OcctlGenerated.OCCTL_KIND_SHELL -> "shell";
            case OcctlGenerated.OCCTL_KIND_FACE -> "face";
            case OcctlGenerated.OCCTL_KIND_WIRE -> "wire";
            case OcctlGenerated.OCCTL_KIND_EDGE -> "edge";
            case OcctlGenerated.OCCTL_KIND_VERTEX -> "vertex";
            case OcctlGenerated.OCCTL_KIND_COMPOUND -> "compound";
            case OcctlGenerated.OCCTL_KIND_COMPSOLID -> "compsolid";
            default -> "unknown";
        };
    }

    private static boolean matchesExpected(JsonNode expected, Map<String, Object> actual) {
        if (expected == null || !expected.isObject()) {
            return true;
        }

        for (var fields = expected.fields(); fields.hasNext();) {
            var entry = fields.next();
            String key = entry.getKey();
            JsonNode value = entry.getValue();

            if (key.endsWith("_at_least")) {
                String real = key.substring(0, key.length() - "_at_least".length());
                Object got = actual.get(real);
                if (!(got instanceof Number n) || n.intValue() < value.asInt()) {
                    return false;
                }
                continue;
            }

            if (key.endsWith("_in")) {
                String real = key.substring(0, key.length() - "_in".length());
                Object got = actual.get(real);
                boolean ok = false;
                if (got != null && value.isArray()) {
                    for (JsonNode item : value) {
                        if (item.asText().equals(String.valueOf(got))) {
                            ok = true;
                            break;
                        }
                    }
                }
                if (!ok) {
                    return false;
                }
                continue;
            }

            Object got = actual.get(key);
            if (!matchesScalar(value, got)) {
                return false;
            }
        }

        return true;
    }

    private static boolean matchesScalar(JsonNode expected, Object actual) {
        if (actual == null) {
            return false;
        }
        if (expected.isBoolean()) {
            return (actual instanceof Boolean b) && b == expected.asBoolean();
        }
        if (expected.isNumber()) {
            return (actual instanceof Number n) && Double.compare(n.doubleValue(), expected.asDouble()) == 0;
        }
        if (expected.isTextual()) {
            return expected.asText().equals(String.valueOf(actual));
        }
        return false;
    }
}
