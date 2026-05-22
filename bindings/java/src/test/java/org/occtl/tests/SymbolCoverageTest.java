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

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;
import org.occtl.Bool;
import org.occtl.Graph;
import org.occtl.Prim;
import org.occtl.generated.OcctlGenerated;
import org.occtl.generated.OcctlRawLibrary;
import org.occtl.generated.OcctlTyped;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

/** Tier-3 coverage: every OCCTL_API function must exist in the generated raw interface. */
public class SymbolCoverageTest {

    private static Path locateAbiJson() {
        String fromEnv = System.getenv("OCCTL_ABI_JSON");
        if (fromEnv != null && !fromEnv.isBlank()) {
            Path envPath = Path.of(fromEnv).toAbsolutePath();
            if (Files.isRegularFile(envPath)) {
                return envPath;
            }
        }

        Path here = Path.of("").toAbsolutePath();
        for (int i = 0; i < 8; ++i) {
            Path candidate = here.resolve("build").resolve("abi.json");
            if (Files.isRegularFile(candidate)) {
                return candidate;
            }
            if (here.getParent() == null) {
                break;
            }
            here = here.getParent();
        }
        throw new IllegalStateException("build/abi.json not found; run tools/abi_dump.py first.");
    }

    private static List<String> abiFunctions() throws IOException {
        ObjectMapper mapper = new ObjectMapper();
        JsonNode root = mapper.readTree(locateAbiJson().toFile());
        Set<String> enabledHeaders = enabledHeadersFromManifest();
        return stream(root.get("functions").elements())
            .filter(node -> enabledHeaders == null
                || enabledHeaders.contains(node.path("header").asText("")))
            .map(n -> n.get("name").asText())
            .collect(Collectors.toList());
    }

    private static Set<String> enabledHeadersFromManifest() {
        List<Path> candidates = candidateManifestPaths();
        if (candidates.isEmpty()) {
            return null;
        }

        Map<String, List<String>> featureToHeaders = new HashMap<>();
        featureToHeaders.put("core", Arrays.asList("occtl.h", "occtl_core.h"));
        featureToHeaders.put("geom", Arrays.asList("occtl_geom.h", "occtl_curves.h", "occtl_curves2d.h", "occtl_curves_common.h", "occtl_surfaces.h"));
        featureToHeaders.put("topo", Arrays.asList("occtl_topo.h", "occtl_topo_types.h", "occtl_topo_algo.h", "occtl_topo_build.h", "occtl_topo_relation.h"));
        featureToHeaders.put("prim", Arrays.asList("occtl_prim.h", "occtl_prim_solid.h", "occtl_prim_sketch.h", "occtl_prim_sweep.h", "occtl_prim_feature.h"));
        featureToHeaders.put("text", Collections.singletonList("occtl_text.h"));
        featureToHeaders.put("bool", Collections.singletonList("occtl_bool.h"));
        featureToHeaders.put("mesh", Collections.singletonList("occtl_mesh.h"));
        featureToHeaders.put("heal", Collections.singletonList("occtl_heal.h"));
        featureToHeaders.put("io_brep", Collections.singletonList("occtl_io_brep.h"));
        featureToHeaders.put("io_step", Collections.singletonList("occtl_io_step.h"));
        featureToHeaders.put("io_iges", Collections.singletonList("occtl_io_iges.h"));
        featureToHeaders.put("io_stl", Collections.singletonList("occtl_io_stl.h"));
        featureToHeaders.put("io_obj", Collections.singletonList("occtl_io_obj.h"));
        featureToHeaders.put("io_gltf", Collections.singletonList("occtl_io_gltf.h"));
        featureToHeaders.put("io_vrml", Collections.singletonList("occtl_io_vrml.h"));
        featureToHeaders.put("io_ply", Collections.singletonList("occtl_io_ply.h"));
        featureToHeaders.put("de", Collections.singletonList("occtl_de.h"));
        featureToHeaders.put("viz", Collections.singletonList("occtl_viz.h"));

        ObjectMapper mapper = new ObjectMapper();
        for (Path candidate : candidates) {
            try {
                if (!Files.isRegularFile(candidate)) {
                    continue;
                }
                JsonNode root = mapper.readTree(candidate.toFile());
                JsonNode features = root.get("binding_features");
                if (features == null || !features.isArray()) {
                    continue;
                }
                LinkedHashSet<String> headers = new LinkedHashSet<>();
                headers.add("occtl.h");
                headers.add("occtl_core.h");
                for (JsonNode featureNode : features) {
                    String feature = featureNode.asText("");
                    for (String header : featureToHeaders.getOrDefault(feature, List.of())) {
                        headers.add(header);
                    }
                }
                return headers;
            } catch (Exception ignored) {
                // try next
            }
        }
        return null;
    }

    private static List<Path> candidateManifestPaths() {
        String explicit = System.getenv("OCCTL_FEATURES_PATH");
        if (explicit != null && !explicit.isBlank()) {
            return List.of(Path.of(explicit));
        }
        String libPath = System.getenv("OCCTL_LIBRARY_PATH");
        if (libPath == null || libPath.isBlank()) {
            return List.of();
        }
        Path base = Path.of(libPath);
        ArrayList<Path> out = new ArrayList<>(2);
        out.add(base.resolve("OCCTLFeatures.json"));
        Path parent = base.getParent();
        if (parent != null) {
            out.add(parent.resolve("OCCTLFeatures.json"));
        }
        return out;
    }

    private static <T> java.util.stream.Stream<T> stream(java.util.Iterator<T> it) {
        Iterable<T> iterable = () -> it;
        return java.util.stream.StreamSupport.stream(iterable.spliterator(), false);
    }

    @Test
    public void everyAbiFunctionIsDeclaredInRawInterface() throws Exception {
        List<String> abi = abiFunctions();
        Set<String> rawMethods = new HashSet<>();
        for (var m : OcctlRawLibrary.class.getMethods()) {
            if (m.getName().startsWith("occtl_")) {
                rawMethods.add(m.getName());
            }
        }

        List<String> missing = abi.stream().filter(name -> !rawMethods.contains(name)).toList();
        assertTrue(missing.isEmpty(), "Missing raw methods: " + missing.stream().limit(20).toList());

        Set<String> exported = new HashSet<>(OcctlGenerated.EXPORTED_FUNCTIONS);
        List<String> missingFromExported = abi.stream().filter(name -> !exported.contains(name)).toList();
        assertTrue(missingFromExported.isEmpty(), "Missing EXPORTED_FUNCTIONS entries: " + missingFromExported.stream().limit(20).toList());

        Set<String> typedMethods = new HashSet<>();
        for (var m : OcctlTyped.class.getMethods()) {
            if (m.getName().startsWith("occtl_")) {
                typedMethods.add(m.getName());
            }
        }
        List<String> missingFromTyped = abi.stream().filter(name -> !typedMethods.contains(name)).toList();
        assertTrue(missingFromTyped.isEmpty(), "Missing typed methods: " + missingFromTyped.stream().limit(20).toList());
    }

    @Test
    public void idiomaticFacadePresence() {
        Set<String> graphMethods = Set.of(
            "faceCount", "edgeCount", "vertexCount", "solidCount", "compoundCount",
            "faceIds", "uidOf", "nodeKind", "historyModified", "historyGenerated");

        Set<String> actualGraphMethods = java.util.Arrays.stream(Graph.class.getMethods())
            .map(m -> m.getName())
            .collect(Collectors.toSet());
        assertTrue(actualGraphMethods.containsAll(graphMethods), "Graph facade methods missing");

        Set<String> primMethods = java.util.Arrays.stream(Prim.class.getMethods())
            .map(m -> m.getName())
            .collect(Collectors.toSet());
        assertTrue(primMethods.contains("makeBox"), "Prim.makeBox missing");

        Set<String> boolMethods = java.util.Arrays.stream(Bool.class.getMethods())
            .map(m -> m.getName())
            .collect(Collectors.toSet());
        for (String name : List.of("fuse", "cut", "common", "section", "split")) {
            assertTrue(boolMethods.contains(name), "Bool." + name + " missing");
        }
    }

    @Test
    public void abiVersionMatchesGeneratedConstant() throws Exception {
        ObjectMapper mapper = new ObjectMapper();
        JsonNode root = mapper.readTree(locateAbiJson().toFile());
        assertEquals(root.get("abi_version").asInt(), OcctlGenerated.ABI_VERSION);
    }
}
