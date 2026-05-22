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

#nullable enable

using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace OcctL.Native;

public static partial class NativeMethods
{
    static NativeMethods()
    {
        NativeLibraryResolver.Init();
    }
}

internal static class NativeLibraryResolver
{
    internal static void Init()
    {
        NativeLibrary.SetDllImportResolver(typeof(NativeMethods).Assembly, Resolve);
    }

    private static IntPtr Resolve(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
    {
        if (!string.Equals(libraryName, LibraryName.Value, StringComparison.Ordinal))
        {
            return IntPtr.Zero;
        }

        string? requestedPath = Environment.GetEnvironmentVariable("OCCTL_LIBRARY_PATH");
        string requestedName = ResolveLibraryName(libraryName, requestedPath);

        if (!string.IsNullOrWhiteSpace(requestedPath))
        {
            foreach (string candidate in CandidatePaths(requestedPath, requestedName))
            {
                if (NativeLibrary.TryLoad(candidate, out IntPtr handle))
                {
                    return handle;
                }
            }
        }

        return NativeLibrary.TryLoad(requestedName, assembly, searchPath, out IntPtr fallback)
            ? fallback
            : IntPtr.Zero;
    }

    private static string ResolveLibraryName(string fallbackName, string? requestedPath)
    {
        string? explicitName = Environment.GetEnvironmentVariable("OCCTL_LIBRARY_NAME");
        if (!string.IsNullOrWhiteSpace(explicitName))
        {
            return NormalizeLibraryName(explicitName);
        }

        foreach (string manifest in CandidateFeatureManifestPaths(requestedPath))
        {
            try
            {
                if (!File.Exists(manifest))
                {
                    continue;
                }
                using var doc = JsonDocument.Parse(File.ReadAllText(manifest));
                if (doc.RootElement.TryGetProperty("library_name", out JsonElement libraryName)
                    && libraryName.ValueKind == JsonValueKind.String)
                {
                    string? value = libraryName.GetString();
                    if (!string.IsNullOrWhiteSpace(value))
                    {
                        return NormalizeLibraryName(value);
                    }
                }
            }
            catch
            {
                // Ignore malformed manifests; fall back to default resolver behavior.
            }
        }

        return NormalizeLibraryName(fallbackName);
    }

    private static string NormalizeLibraryName(string rawName)
    {
        string name = rawName.Trim();
        if (name.EndsWith(".dylib", StringComparison.OrdinalIgnoreCase))
        {
            name = name[..^6];
        }
        else if (name.EndsWith(".so", StringComparison.OrdinalIgnoreCase))
        {
            name = name[..^3];
        }
        else if (name.EndsWith(".dll", StringComparison.OrdinalIgnoreCase))
        {
            name = name[..^4];
        }
        else if (name.EndsWith(".lib", StringComparison.OrdinalIgnoreCase))
        {
            name = name[..^4];
        }

        if (name.StartsWith("lib", StringComparison.OrdinalIgnoreCase))
        {
            name = name[3..];
        }
        if (!name.StartsWith("occtl-", StringComparison.OrdinalIgnoreCase))
        {
            name = "occtl-" + name;
        }
        return name;
    }

    private static string[] CandidateFeatureManifestPaths(string? requestedPath)
    {
        string? explicitManifest = Environment.GetEnvironmentVariable("OCCTL_FEATURES_PATH");
        if (!string.IsNullOrWhiteSpace(explicitManifest))
        {
            return new[] { explicitManifest };
        }

        if (!string.IsNullOrWhiteSpace(requestedPath))
        {
            return new[]
            {
                Path.Combine(requestedPath, "OCCTLFeatures.json"),
                Path.Combine(Path.GetDirectoryName(requestedPath) ?? requestedPath, "OCCTLFeatures.json"),
            };
        }

        return Array.Empty<string>();
    }

    private static string[] CandidatePaths(string directory, string libraryName)
    {
        if (OperatingSystem.IsWindows())
        {
            return new[]
            {
                Path.Combine(directory, libraryName + ".dll"),
                Path.Combine(directory, libraryName + ".lib"),
                Path.Combine(directory, libraryName),
            };
        }

        if (OperatingSystem.IsMacOS())
        {
            return new[]
            {
                Path.Combine(directory, "lib" + libraryName + ".dylib"),
                Path.Combine(directory, libraryName + ".dylib"),
                Path.Combine(directory, libraryName),
            };
        }

        return new[]
        {
            Path.Combine(directory, "lib" + libraryName + ".so"),
            Path.Combine(directory, libraryName + ".so"),
            Path.Combine(directory, libraryName),
        };
    }
}
