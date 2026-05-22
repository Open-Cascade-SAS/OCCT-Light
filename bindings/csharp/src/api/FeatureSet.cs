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

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

namespace OcctL;

/// <summary>
/// Installed feature-set probe based on <c>OCCTLFeatures.json</c>.
/// Used by handwritten facades to return typed <see cref="UnsupportedException"/>
/// before touching missing native entry points.
/// </summary>
internal static class FeatureSet
{
    private sealed record Snapshot(bool Known, HashSet<string> Features);

    private static readonly Lazy<Snapshot> s_snapshot = new(LoadFeatures);

    internal static bool Has(string feature)
    {
        Snapshot snap = s_snapshot.Value;
        return !snap.Known || snap.Features.Contains(feature);
    }

    internal static void Require(string feature, string symbol)
    {
        if (Has(feature))
        {
            return;
        }

        throw new UnsupportedException(
            OcctlStatus.Unsupported,
            $"Feature '{feature}' is not available in this OCCT-Light build (missing {symbol}).",
            default,
            0);
    }

    private static Snapshot LoadFeatures()
    {
        foreach (string manifest in CandidateManifestPaths())
        {
            try
            {
                if (!File.Exists(manifest))
                {
                    continue;
                }
                using var doc = JsonDocument.Parse(File.ReadAllText(manifest));
                if (!doc.RootElement.TryGetProperty("binding_features", out JsonElement features)
                    || features.ValueKind != JsonValueKind.Array)
                {
                    continue;
                }

                var outSet = new HashSet<string>(StringComparer.Ordinal);
                foreach (JsonElement entry in features.EnumerateArray())
                {
                    if (entry.ValueKind == JsonValueKind.String)
                    {
                        string? value = entry.GetString();
                        if (!string.IsNullOrWhiteSpace(value))
                        {
                            outSet.Add(value);
                        }
                    }
                }
                return new Snapshot(true, outSet);
            }
            catch
            {
                // Ignore malformed files and keep probing candidates.
            }
        }

        return new Snapshot(false, new HashSet<string>(StringComparer.Ordinal));
    }

    private static string[] CandidateManifestPaths()
    {
        string? explicitManifest = Environment.GetEnvironmentVariable("OCCTL_FEATURES_PATH");
        if (!string.IsNullOrWhiteSpace(explicitManifest))
        {
            return new[] { explicitManifest };
        }

        string? libraryPath = Environment.GetEnvironmentVariable("OCCTL_LIBRARY_PATH");
        if (!string.IsNullOrWhiteSpace(libraryPath))
        {
            return new[]
            {
                Path.Combine(libraryPath, "OCCTLFeatures.json"),
                Path.Combine(Path.GetDirectoryName(libraryPath) ?? libraryPath, "OCCTLFeatures.json"),
            };
        }

        return Array.Empty<string>();
    }
}
