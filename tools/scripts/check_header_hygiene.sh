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

aRoot="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." 2>/dev/null && pwd)" || {
  echo "check_header_hygiene: could not determine source root — is the script directory missing?" >&2
  exit 1
}
aDir="${1:-${aRoot}/include/occtl}"

# Enforce repository-wide OCCT-Light license headers before public-header hygiene.
python3 "${aRoot}/tools/license_headers.py" --check

exec cmake \
  -DOCCTL_HYGIENE_DIR="${aDir}" \
  -P "${aRoot}/cmake/CheckHeaderHygieneDriver.cmake"
