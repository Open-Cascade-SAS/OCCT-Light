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
using System.Text.Json;
using OcctL;
using OcctL.Native;
using OcctL.Options;

namespace OcctL.Parity;

/// <summary>
/// Parity runner. Reads one scenario from <c>tests/binding_parity</c>, executes it
/// through the idiomatic C# facade, checks the expected fields, and prints one
/// canonical JSON object.
/// </summary>
public static class Runner
{
    public static int Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.Error.WriteLine("usage: OcctL.Parity <scenario.json>");
            return 2;
        }

        string aPath = args[0];
        if (!File.Exists(aPath))
        {
            Console.Error.WriteLine($"Scenario not found: {aPath}");
            return 2;
        }

        using var aStream = File.OpenRead(aPath);
        using var aDoc = JsonDocument.Parse(aStream);
        JsonElement aRoot = aDoc.RootElement;
        string aScenario = aRoot.GetProperty("scenario").GetString() ?? string.Empty;
        IReadOnlyDictionary<string, object> anActual = RunScenario(aScenario, aRoot);
        bool aMatches = Matches(aRoot.GetProperty("expected"), anActual);

        string aJson = JsonSerializer.Serialize(new
        {
            scenario = aScenario,
            binding = "csharp",
            actual = anActual,
            matches = aMatches,
        });
        Console.WriteLine(aJson);
        return aMatches ? 0 : 1;
    }

    private static IReadOnlyDictionary<string, object> RunScenario(string scenario, JsonElement doc)
        => scenario switch
        {
            "build_box" => RunBuildBox(doc),
            "fuse_two_boxes" => RunFuseTwoBoxes(),
            "cut_box_corner" => RunCutBoxCorner(),
            "common_two_overlapping_boxes" => RunCommonTwoOverlappingBoxes(),
            "section_two_overlapping_boxes" => RunSectionTwoOverlappingBoxes(),
            "split_box_by_box" => RunSplitBoxByBox(),
            "history_modified_after_fuse" => RunHistoryModifiedAfterFuse(),
            _ => throw new InvalidOperationException($"Unsupported parity scenario: {scenario}"),
        };

    private static IReadOnlyDictionary<string, object> RunBuildBox(JsonElement doc)
    {
        double aDx = 10.0;
        double aDy = 10.0;
        double aDz = 5.0;
        if (doc.TryGetProperty("params", out JsonElement aParams))
        {
            if (aParams.TryGetProperty("dx", out JsonElement aDxValue)) aDx = aDxValue.GetDouble();
            if (aParams.TryGetProperty("dy", out JsonElement aDyValue)) aDy = aDyValue.GetDouble();
            if (aParams.TryGetProperty("dz", out JsonElement aDzValue)) aDz = aDzValue.GetDouble();
        }

        using var aGraph = new Graph();
        Prim.MakeBox(aGraph, aDx, aDy, aDz);
        return new Dictionary<string, object>
        {
            ["face_count"] = aGraph.FaceCount,
            ["edge_count"] = aGraph.EdgeCount,
            ["vertex_count"] = aGraph.VertexCount,
            ["solid_count"] = aGraph.SolidCount,
        };
    }

    private static IReadOnlyDictionary<string, object> RunFuseTwoBoxes()
    {
        using var aGraph = new Graph();
        NodeId aBoxA = Prim.MakeBox(aGraph, 10.0, 10.0, 10.0);
        NodeId aBoxB = Prim.MakeBox(aGraph, Box(10.0, 10.0, 10.0, 5.0, 0.0, 0.0));
        Uid[] aBoxAFaceUids = aGraph.Faces().Take(6).Select(aGraph.UidOf).ToArray();

        NodeId aResult = Bool.Fuse(aGraph, [aBoxA], [aBoxB]);
        bool aHistoryNonEmpty = aBoxAFaceUids.Any(uid => aGraph.HistoryModified(uid).Length != 0
                                                      || aGraph.HistoryGenerated(uid).Length != 0);

        return new Dictionary<string, object>
        {
            ["root_kind"] = KindString(aGraph.NodeKind(aResult)),
            ["history_modified_nonempty_on_first_face_of_box_a"] = aHistoryNonEmpty,
        };
    }

    private static IReadOnlyDictionary<string, object> RunCutBoxCorner()
    {
        using var aGraph = new Graph();
        NodeId aBox = Prim.MakeBox(aGraph, 10.0, 10.0, 10.0);
        NodeId aTool = Prim.MakeBox(aGraph, Box(5.0, 5.0, 5.0, 7.5, 7.5, 7.5));
        NodeId aResult = Bool.Cut(aGraph, [aBox], [aTool]);

        return new Dictionary<string, object>
        {
            ["root_kind"] = KindString(aGraph.NodeKind(aResult)),
            ["solid_count"] = aGraph.SolidCount,
        };
    }

    private static IReadOnlyDictionary<string, object> RunCommonTwoOverlappingBoxes()
    {
        using var aGraph = new Graph();
        NodeId aBoxA = Prim.MakeBox(aGraph, 10.0, 10.0, 10.0);
        NodeId aBoxB = Prim.MakeBox(aGraph, Box(10.0, 10.0, 10.0, 5.0, 5.0, 5.0));
        NodeId aResult = Bool.Common(aGraph, [aBoxA], [aBoxB]);

        return new Dictionary<string, object>
        {
            ["root_kind"] = KindString(aGraph.NodeKind(aResult)),
            ["solid_count"] = aGraph.SolidCount,
        };
    }

    private static IReadOnlyDictionary<string, object> RunSectionTwoOverlappingBoxes()
    {
        using var aGraph = new Graph();
        NodeId aBoxA = Prim.MakeBox(aGraph, 10.0, 10.0, 10.0);
        NodeId aBoxB = Prim.MakeBox(aGraph, Box(10.0, 10.0, 10.0, 5.0, 0.0, 0.0));
        int aEdgesBefore = aGraph.EdgeCount;
        NodeId aResult = Bool.Section(aGraph, [aBoxA], [aBoxB]);

        return new Dictionary<string, object>
        {
            ["root_kind"] = KindString(aGraph.NodeKind(aResult)),
            ["edge_count_increased"] = aGraph.EdgeCount > aEdgesBefore,
        };
    }

    private static IReadOnlyDictionary<string, object> RunSplitBoxByBox()
    {
        using var aGraph = new Graph();
        NodeId aBoxA = Prim.MakeBox(aGraph, 10.0, 10.0, 10.0);
        NodeId aBoxB = Prim.MakeBox(aGraph, Box(10.0, 10.0, 10.0, 0.0, 0.0, 5.0));
        NodeId aResult = Bool.Split(aGraph, [aBoxA], [aBoxB]);

        return new Dictionary<string, object>
        {
            ["root_kind"] = KindString(aGraph.NodeKind(aResult)),
            ["compound_count"] = aGraph.CompoundCount,
            ["solid_count"] = aGraph.SolidCount,
        };
    }

    private static IReadOnlyDictionary<string, object> RunHistoryModifiedAfterFuse()
    {
        using var aGraph = new Graph();
        NodeId aBoxA = Prim.MakeBox(aGraph, 10.0, 10.0, 10.0);
        NodeId aBoxB = Prim.MakeBox(aGraph, Box(10.0, 10.0, 10.0, 5.0, 0.0, 0.0));
        Uid[] aFaceUids = aGraph.Faces().Take(12).Select(aGraph.UidOf).ToArray();
        Uid[] aBoxAFaceUids = aFaceUids[0..6];
        Uid[] aBoxBFaceUids = aFaceUids[6..12];

        NodeId aResult = Bool.Fuse(aGraph, [aBoxA], [aBoxB]);
        bool aBoxAModified = AnyModified(aGraph, aBoxAFaceUids);
        bool aBoxBModified = AnyModified(aGraph, aBoxBFaceUids);

        return new Dictionary<string, object>
        {
            ["box_a_modified_nonempty"] = aBoxAModified,
            ["box_b_modified_nonempty"] = aBoxBModified,
        };
    }

    private static bool AnyModified(Graph graph, IEnumerable<Uid> uids)
        => uids.Any(uid => graph.HistoryModified(uid).Length != 0 || graph.HistoryGenerated(uid).Length != 0);

    private static PrimBoxInfo Box(double dx, double dy, double dz, double x, double y, double z)
        => new()
        {
            Dx = dx,
            Dy = dy,
            Dz = dz,
            Placement = Placement(x, y, z),
        };

    private static OcctlAxis2Placement Placement(double x, double y, double z)
        => new()
        {
            Location = new OcctlPoint3 { X = x, Y = y, Z = z },
            XDir = new OcctlDirection3 { X = 1.0, Y = 0.0, Z = 0.0 },
            XDirRef = new OcctlDirection3 { X = 0.0, Y = 1.0, Z = 0.0 },
        };

    private static string KindString(OcctlNodeKind kind)
        => kind switch
        {
            OcctlNodeKind.KindSolid => "solid",
            OcctlNodeKind.KindShell => "shell",
            OcctlNodeKind.KindFace => "face",
            OcctlNodeKind.KindWire => "wire",
            OcctlNodeKind.KindEdge => "edge",
            OcctlNodeKind.KindVertex => "vertex",
            OcctlNodeKind.KindCompound => "compound",
            OcctlNodeKind.KindCompsolid => "compsolid",
            _ => "unknown",
        };

    private static bool Matches(JsonElement expected, IReadOnlyDictionary<string, object> actual)
    {
        foreach (JsonProperty aProperty in expected.EnumerateObject())
        {
            string aName = aProperty.Name;
            if (aName.EndsWith("_at_least", StringComparison.Ordinal))
            {
                string anActualName = aName[..^"_at_least".Length];
                if (!actual.TryGetValue(anActualName, out object? anActualValue)
                    || Convert.ToInt32(anActualValue) < aProperty.Value.GetInt32())
                {
                    return false;
                }
                continue;
            }

            if (aName.EndsWith("_in", StringComparison.Ordinal))
            {
                string anActualName = aName[..^"_in".Length];
                if (!actual.TryGetValue(anActualName, out object? anActualValue)
                    || !aProperty.Value.EnumerateArray().Any(v => v.GetString() == Convert.ToString(anActualValue)))
                {
                    return false;
                }
                continue;
            }

            if (!actual.TryGetValue(aName, out object? aValue) || !MatchesScalar(aProperty.Value, aValue))
            {
                return false;
            }
        }
        return true;
    }

    private static bool MatchesScalar(JsonElement expected, object actual)
        => expected.ValueKind switch
        {
            JsonValueKind.True => actual is bool b && b,
            JsonValueKind.False => actual is bool b && !b,
            JsonValueKind.Number => Convert.ToDouble(actual) == expected.GetDouble(),
            JsonValueKind.String => Convert.ToString(actual) == expected.GetString(),
            _ => false,
        };
}
