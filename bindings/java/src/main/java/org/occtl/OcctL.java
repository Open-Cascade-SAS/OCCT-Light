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

import com.sun.jna.Pointer;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.occtl.generated.OcctlGenerated;
import org.occtl.generated.OcctlRawLibrary;
import org.occtl.generated.OcctlTyped;

/** Entry point and runtime helpers for the Java binding. */
public final class OcctL {
    private static final Pattern LIBRARY_NAME_PATTERN =
        Pattern.compile("\"library_name\"\\s*:\\s*\"([^\"]+)\"");
    private static final Pattern FEATURES_BLOCK_PATTERN =
        Pattern.compile("\"binding_features\"\\s*:\\s*\\[(.*?)\\]", Pattern.DOTALL);
    private static final Pattern FEATURE_TOKEN_PATTERN = Pattern.compile("\"([^\"]+)\"");

    public static final OcctlRawLibrary RAW = loadRaw();
    public static final OcctlTyped TYPED = new OcctlTyped(RAW);
    private static final RuntimeFeatures RUNTIME_FEATURES = loadRuntimeFeatures();
    private static volatile boolean initialized = false;

    private OcctL() {}

    private static OcctlRawLibrary loadRaw() {
        String libPath = System.getenv("OCCTL_LIBRARY_PATH");
        if (libPath != null && !libPath.isBlank()) {
            String prev = System.getProperty("jna.library.path", "");
            String sep = System.getProperty("path.separator");
            if (prev.isBlank()) {
                System.setProperty("jna.library.path", libPath);
            } else {
                System.setProperty("jna.library.path", libPath + sep + prev);
            }
        }

        String libName = System.getenv("OCCTL_LIBRARY_NAME");
        if (libName == null || libName.isBlank()) {
            libName = resolveLibraryNameFromManifest("occtl-full");
        }
        libName = normalizeLibraryName(libName);
        return OcctlRawLibrary.load(libName);
    }

    static boolean hasFeature(String feature) {
        if (!RUNTIME_FEATURES.known()) {
            return true;
        }
        return RUNTIME_FEATURES.features().contains(feature);
    }

    static void requireFeature(String feature, String symbol) {
        if (hasFeature(feature)) {
            return;
        }
        throw new OcctLException(
            OcctlGenerated.OCCTL_UNSUPPORTED,
            "Feature '" + feature + "' is not available in this OCCT-Light build (missing " + symbol + ").");
    }

    public static synchronized void ensureInitialized() {
        if (initialized) {
            return;
        }

        int runtimeAbi = RAW.occtl_runtime_abi_version();
        if (runtimeAbi != OcctlGenerated.ABI_VERSION) {
            throw new OcctLException(
                runtimeAbi,
                "ABI mismatch: runtime=" + runtimeAbi + " binding=" + OcctlGenerated.ABI_VERSION
            );
        }

        org.occtl.generated.OcctlTypes.OcctlRuntimeInitInfo info = new org.occtl.generated.OcctlTypes.OcctlRuntimeInitInfo();
        check(RAW.occtl_runtime_init_info_init(info.getPointer()));
        check(RAW.occtl_runtime_init(info.getPointer()));
        initialized = true;
    }

    public static void check(int status) {
        if (status == OcctlGenerated.OCCTL_OK) {
            return;
        }

        String message = "OCCT-Light call failed (status=" + status + ")";
        Pointer statusText = RAW.occtl_status_to_string(status);
        if (statusText != null) {
            String text = statusText.getString(0, "UTF-8");
            if (text != null && !text.isBlank()) {
                message = text;
            }
        }

        Pointer errPtr = RAW.occtl_error_last();
        if (errPtr != null) {
            org.occtl.generated.OcctlTypes.OcctlError err = new org.occtl.generated.OcctlTypes.OcctlError(errPtr);
            if (err.message != null) {
                String errMsg = err.message.getString(0, "UTF-8");
                if (errMsg != null && !errMsg.isBlank()) {
                    message = errMsg;
                }
            }
        }

        throw new OcctLException(status, message);
    }

    private static String resolveLibraryNameFromManifest(String fallbackName) {
        for (Path manifest : candidateFeatureManifestPaths()) {
            String text = readUtf8(manifest);
            if (text == null) {
                continue;
            }
            Matcher matcher = LIBRARY_NAME_PATTERN.matcher(text);
            if (matcher.find()) {
                String value = matcher.group(1).trim();
                if (!value.isEmpty()) {
                    return value;
                }
            }
        }
        return fallbackName;
    }

    private static String normalizeLibraryName(String rawName) {
        String name = rawName.trim();
        name = name.replaceFirst("(?i)\\.(dylib|so|dll|lib)$", "");
        name = name.replaceFirst("(?i)^lib", "");
        if (!name.toLowerCase().startsWith("occtl-")) {
            name = "occtl-" + name;
        }
        return name;
    }

    private static RuntimeFeatures loadRuntimeFeatures() {
        for (Path manifest : candidateFeatureManifestPaths()) {
            String text = readUtf8(manifest);
            if (text == null) {
                continue;
            }
            Matcher block = FEATURES_BLOCK_PATTERN.matcher(text);
            if (!block.find()) {
                continue;
            }

            LinkedHashSet<String> features = new LinkedHashSet<>();
            Matcher token = FEATURE_TOKEN_PATTERN.matcher(block.group(1));
            while (token.find()) {
                String feature = token.group(1).trim();
                if (!feature.isEmpty()) {
                    features.add(feature);
                }
            }
            if (!features.isEmpty()) {
                return new RuntimeFeatures(true, Collections.unmodifiableSet(features));
            }
        }
        return new RuntimeFeatures(false, Set.of());
    }

    private static List<Path> candidateFeatureManifestPaths() {
        String explicitManifest = System.getenv("OCCTL_FEATURES_PATH");
        if (explicitManifest != null && !explicitManifest.isBlank()) {
            return List.of(Path.of(explicitManifest));
        }

        String libraryPath = System.getenv("OCCTL_LIBRARY_PATH");
        if (libraryPath == null || libraryPath.isBlank()) {
            return List.of();
        }

        Path libPath = Path.of(libraryPath);
        ArrayList<Path> out = new ArrayList<>(2);
        out.add(libPath.resolve("OCCTLFeatures.json"));
        Path parent = libPath.getParent();
        if (parent != null) {
            out.add(parent.resolve("OCCTLFeatures.json"));
        }
        return out;
    }

    private static String readUtf8(Path path) {
        try {
            if (!Files.exists(path)) {
                return null;
            }
            return Files.readString(path, StandardCharsets.UTF_8);
        } catch (IOException ex) {
            return null;
        }
    }

    private record RuntimeFeatures(boolean known, Set<String> features) {}
}
