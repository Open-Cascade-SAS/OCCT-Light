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
using System.IO;
using System.Linq;
using FluentAssertions;
using OcctL;
using OcctL.Native;
using Xunit;

namespace OcctL.Tests;

/// <summary>
/// Tier-1 smoke tests: prove the binding loads, the handshake works, and one
/// representative happy/error path is wired correctly.
/// </summary>
public class SmokeTests
{
    [Fact]
    public void AbiHandshakeMatches()
    {
        // Versions must agree at the binding/runtime boundary.
        Core.AbiVersion.Should().Be(OcctL.AbiHandshake.ExpectedAbiVersion);
        Core.RuntimeAbiVersion.Should().Be(Core.AbiVersion);
    }

    [Fact]
    public void BuildVertexAndIterate()
    {
        using var g = new Graph();
        NodeId v = g.MakeVertex(1.0, 2.0, 3.0);
        v.IsInvalid.Should().BeFalse();

        using var enumerator = g.Vertices();
        var vertices = enumerator.ToList();
        vertices.Should().HaveCount(1);
        vertices[0].Should().Be(v);
    }

    [Fact]
    public void BuildPrimitivesThroughTypedFacade()
    {
        using var g = new Graph();

        g.MakeBox(1.0, 2.0, 3.0).IsInvalid.Should().BeFalse();
        g.MakeSphere(2.0).IsInvalid.Should().BeFalse();
        g.MakeCylinder(1.0, 4.0).IsInvalid.Should().BeFalse();
        g.MakeCone(2.0, 1.0, 5.0).IsInvalid.Should().BeFalse();
        g.MakeTorus(5.0, 1.0).IsInvalid.Should().BeFalse();
        g.MakeWedge(4.0, 3.0, 2.0, 1.0).IsInvalid.Should().BeFalse();

        g.SolidCount.Should().Be(6);
        g.CheckIssues().Should().BeEmpty();
        g.IsValid.Should().BeTrue();
    }

    [Fact]
    public void GraphBooleanExtensionsWork()
    {
        using var g = new Graph();
        NodeId boxA = g.MakeBox(10.0, 10.0, 10.0);
        NodeId boxB = g.MakeBox(10.0, 10.0, 10.0);
        NodeId fused = g.Fuse(new[] { boxA }, new[] { boxB });
        fused.IsInvalid.Should().BeFalse();
    }

    [Fact]
    public void BrepHelpersRoundTripBox()
    {
        string path = Path.Combine(Path.GetTempPath(), $"occtl-csharp-{Guid.NewGuid():N}.brep");
        try
        {
            using (var g = new Graph())
            {
                NodeId box = g.MakeBox(1.0, 2.0, 3.0);
                IoBrep.Write(g, box, path);
            }

            File.Exists(path).Should().BeTrue();
            using GraphRootResult result = IoBrep.Read(path);
            result.Root.IsInvalid.Should().BeFalse();
            result.Graph.IsValid.Should().BeTrue();
            result.Graph.SolidCount.Should().Be(1);
        }
        finally
        {
            if (File.Exists(path))
            {
                File.Delete(path);
            }
        }
    }

    [Fact]
    public void ErrorPathCarriesMessage()
    {
        Action act = () => Graph.FromPointerUnsafe(IntPtr.Zero);
        var ex = act.Should().Throw<InvalidHandleException>().Which;
        ex.Status.Should().Be(OcctlStatus.InvalidHandle);
        ex.Message.Should().NotBeNullOrEmpty();
    }
}
