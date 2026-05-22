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
using System.Linq;
using System.Reflection;
using System.Text.Json;
using FluentAssertions;
using Xunit;

namespace OcctL.Tests;

/// <summary>
/// Tier-3 symbol-coverage test. Walks the Phase-0 ABI dump and asserts every public
/// <c>OCCTL_API</c> function has a corresponding wrapper in either the
/// <c>OcctL.Native</c> assembly (raw P/Invoke surface) or <c>OcctL</c> (idiomatic
/// facade). Fails if any are missing — keeps the binding from rotting silently when
/// the C ABI grows.
/// </summary>
public class SymbolCoverageTests
{
    private static string LocateAbiJson()
    {
        // Walk up from the test bin directory to find build/abi.json.
        string? dir = AppContext.BaseDirectory;
        for (int i = 0; i < 10 && dir is not null; i++)
        {
            string candidate = Path.Combine(dir, "build", "abi.json");
            if (File.Exists(candidate)) return candidate;
            dir = Path.GetDirectoryName(dir);
        }
        throw new FileNotFoundException("Unable to locate build/abi.json; run tools/abi_dump.py first.");
    }

    private static HashSet<string>? EnabledHeadersFromManifest()
    {
        static Dictionary<string, string[]> FeatureMap() => new()
        {
            ["core"] = new[] { "occtl.h", "occtl_core.h" },
            ["geom"] = new[] { "occtl_geom.h", "occtl_curves.h", "occtl_curves2d.h", "occtl_curves_common.h", "occtl_surfaces.h" },
            ["topo"] = new[] { "occtl_topo.h", "occtl_topo_types.h", "occtl_topo_algo.h", "occtl_topo_build.h", "occtl_topo_relation.h" },
            ["prim"] = new[] { "occtl_prim.h", "occtl_prim_solid.h", "occtl_prim_sketch.h", "occtl_prim_sweep.h", "occtl_prim_feature.h" },
            ["text"] = new[] { "occtl_text.h" },
            ["bool"] = new[] { "occtl_bool.h" },
            ["mesh"] = new[] { "occtl_mesh.h" },
            ["heal"] = new[] { "occtl_heal.h" },
            ["io_brep"] = new[] { "occtl_io_brep.h" },
            ["io_step"] = new[] { "occtl_io_step.h" },
            ["io_iges"] = new[] { "occtl_io_iges.h" },
            ["io_stl"] = new[] { "occtl_io_stl.h" },
            ["io_obj"] = new[] { "occtl_io_obj.h" },
            ["io_gltf"] = new[] { "occtl_io_gltf.h" },
            ["io_vrml"] = new[] { "occtl_io_vrml.h" },
            ["io_ply"] = new[] { "occtl_io_ply.h" },
            ["de"] = new[] { "occtl_de.h" },
            ["viz"] = new[] { "occtl_viz.h" },
        };

        var candidates = new List<string>();
        string? explicitPath = Environment.GetEnvironmentVariable("OCCTL_FEATURES_PATH");
        if (!string.IsNullOrWhiteSpace(explicitPath))
        {
            candidates.Add(explicitPath);
        }
        else
        {
            string? libPath = Environment.GetEnvironmentVariable("OCCTL_LIBRARY_PATH");
            if (!string.IsNullOrWhiteSpace(libPath))
            {
                candidates.Add(Path.Combine(libPath, "OCCTLFeatures.json"));
                candidates.Add(Path.Combine(Path.GetDirectoryName(libPath) ?? libPath, "OCCTLFeatures.json"));
            }
        }

        foreach (string candidate in candidates)
        {
            if (!File.Exists(candidate))
            {
                continue;
            }
            using var stream = File.OpenRead(candidate);
            using var doc = JsonDocument.Parse(stream);
            if (!doc.RootElement.TryGetProperty("binding_features", out JsonElement features)
                || features.ValueKind != JsonValueKind.Array)
            {
                continue;
            }

            var headers = new HashSet<string>(StringComparer.Ordinal)
            {
                "occtl.h",
                "occtl_core.h",
            };
            var map = FeatureMap();
            foreach (JsonElement item in features.EnumerateArray())
            {
                string feature = item.GetString() ?? string.Empty;
                if (!map.TryGetValue(feature, out var mapped))
                {
                    continue;
                }
                foreach (string header in mapped)
                {
                    headers.Add(header);
                }
            }
            return headers;
        }

        return null;
    }

    private static HashSet<string> CollectKnownEntryPoints()
    {
        var known = new HashSet<string>(StringComparer.Ordinal);

        // Native: scan [LibraryImport] EntryPoint attributes on private/public static methods.
        Assembly native = typeof(OcctL.Native.NativeMethods).Assembly;
        foreach (Type t in native.GetTypes())
        {
            foreach (MethodInfo m in t.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic))
            {
                foreach (CustomAttributeData attr in m.GetCustomAttributesData())
                {
                    if (attr.AttributeType.Name == "LibraryImportAttribute")
                    {
                        foreach (var na in attr.NamedArguments)
                        {
                            if (na.MemberName == "EntryPoint" && na.TypedValue.Value is string ep)
                            {
                                known.Add(ep);
                            }
                        }
                    }
                }
            }
        }
        return known;
    }

    [Fact]
    public void EveryOcctlApiHasAWrapper()
    {
        string abiJsonPath = LocateAbiJson();
        using var stream = File.OpenRead(abiJsonPath);
        using var doc = JsonDocument.Parse(stream);
        HashSet<string>? enabledHeaders = EnabledHeadersFromManifest();
        var allNames = doc.RootElement.GetProperty("functions")
            .EnumerateArray()
            .Where(e => enabledHeaders is null
                || (e.TryGetProperty("header", out JsonElement h)
                    && enabledHeaders.Contains(h.GetString() ?? string.Empty)))
            .Select(e => e.GetProperty("name").GetString()!)
            .ToList();

        allNames.Should().NotBeEmpty();

        var known = CollectKnownEntryPoints();
        var missing = allNames.Where(n => !known.Contains(n)).ToList();
        missing.Should().BeEmpty(
            "every OCCTL_API function must have a wrapper. Missing: " +
            string.Join(", ", missing.Take(20)) + (missing.Count > 20 ? " (and more…)" : string.Empty));
    }

    /// <summary>
    /// Tier-3 facade-presence guard for the hand-written idiomatic surface.
    /// The auto-generated raw classes pass the
    /// <see cref="EveryOcctlApiHasAWrapper"/> check by virtue of the raw P/Invoke
    /// stubs they wrap, but a missing idiomatic method slips through. This test
    /// names the hand-written facade methods expected to ship.
    /// </summary>
    [Theory]
    [InlineData(typeof(OcctL.Bool),    "Fuse")]
    [InlineData(typeof(OcctL.Bool),    "Cut")]
    [InlineData(typeof(OcctL.Bool),    "Common")]
    [InlineData(typeof(OcctL.Bool),    "Section")]
    [InlineData(typeof(OcctL.Bool),    "Split")]
    [InlineData(typeof(OcctL.Graph),   "HistoryModified")]
    [InlineData(typeof(OcctL.Graph),   "HistoryGenerated")]
    [InlineData(typeof(OcctL.Graph),   "HistoryDeletedAll")]
    public void IdiomaticFacadeExposesMethod(Type theType, string theMethod)
    {
        MethodInfo? aMethod = theType.GetMethod(
            theMethod,
            BindingFlags.Public | BindingFlags.Static | BindingFlags.Instance);
        aMethod.Should().NotBeNull(
            $"{theType.FullName}.{theMethod} is the idiomatic surface for occtl_{theType.Name.ToLowerInvariant()}_{theMethod.ToLowerInvariant()} — missing means a hand-written facade was lost.");
    }
}
