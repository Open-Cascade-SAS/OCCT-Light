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

namespace OcctL.Native;

/// <summary>
/// Name of the OCCT-Light native shared library. The runtime resolves this via the
/// platform's library-load rules (DYLD_LIBRARY_PATH / LD_LIBRARY_PATH / PATH plus
/// NuGet's runtimes/&lt;rid&gt;/native/ layout when packaged).
/// </summary>
internal static class LibraryName
{
    /// <summary>
    /// Base name passed to <see cref="System.Runtime.InteropServices.LibraryImportAttribute"/>.
    /// </summary>
    internal const string Value = "occtl-full";
}
