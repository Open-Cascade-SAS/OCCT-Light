#!/usr/bin/env bash
# Copyright (c) 2026 Capgemini Engineering Research and Development.
#
# This file is part of OCCT-Light software library.
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License version 3 as published
# by the Free Software Foundation, with an option to use any later version.
# Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
# for complete text of the license and disclaimer of any warranty.
#
# Alternatively, this file may be used under the terms of a commercial
# license or contractual agreement.
#
# SPDX-License-Identifier: AGPL-3.0-or-later

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DOXYFILE="${ROOT_DIR}/docs/Doxyfile"
DOCS_API_DIR="${ROOT_DIR}/docs/api"

if ! command -v doxygen >/dev/null 2>&1; then
  echo "error: doxygen is not installed" >&2
  exit 1
fi

cd "${ROOT_DIR}"
rm -rf "${DOCS_API_DIR}/html" "${DOCS_API_DIR}/xml"
rm -f "${DOCS_API_DIR}/doxygen-warnings.log"
doxygen "${DOXYFILE}"

echo "Doxygen HTML:"
echo "  ${ROOT_DIR}/docs/api/html/index.html"
echo "Doxygen warnings log:"
echo "  ${ROOT_DIR}/docs/api/doxygen-warnings.log"
