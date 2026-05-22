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

"""Static checks for the Python sample registry."""

from __future__ import annotations

import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
if str(_REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(_REPO_ROOT))

from samples.python.occtl_samples.registry import SAMPLE_SPECS


def test_registry_has_at_least_50_samples():
    assert len(SAMPLE_SPECS) >= 100


def test_registry_names_are_unique_and_slugged():
    names = [spec.name for spec in SAMPLE_SPECS]
    assert len(names) == len(set(names))
    assert all(name == name.lower() for name in names)
    assert all(name.replace("_", "").isalnum() for name in names)


def test_registry_covers_expected_model_families():
    categories = {spec.category for spec in SAMPLE_SPECS}
    assert {"box", "sphere", "cylinder", "cone", "torus", "wedge", "compound", "boolean", "complex"} <= categories
    assert sum(1 for spec in SAMPLE_SPECS if spec.category == "complex") >= 50
    complex_descriptions = " ".join(spec.description.lower() for spec in SAMPLE_SPECS if spec.category == "complex")
    assert "offset" in complex_descriptions
    assert "thick" in complex_descriptions
    assert "chamfer" in complex_descriptions
    assert "fillet" in complex_descriptions
    assert "boolean" in complex_descriptions
