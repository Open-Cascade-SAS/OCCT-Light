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

"""Strong nominal-typed 64-bit identity values for the occtl binding.

The C ABI uses five POD value-handle types:

- ``occtl_node_id_t``  — transient identity of a graph entity (vertex,
  edge, face, …). Re-used after compaction.
- ``occtl_uid_t``      — persistent identity that survives ``Compact``.
- ``occtl_ref_id_t``   — identity of a reference-entry (a usage of a node).
- ``occtl_ref_uid_t``  — persistent identity of a reference-entry.
- ``occtl_rep_id_t``   — identity of a representation (Surface, Curve3D, …).
- ``occtl_rep_uid_t``  — persistent identity of a representation.

Each maps to its own ``NamedTuple`` subclass in Python so static type checkers
(mypy / pyright) catch "you passed a face NodeId where the API expects a Uid".
At runtime the wrappers are still single-field tuples that round-trip through
the cffi layer cheaply.
"""

from __future__ import annotations

from typing import NamedTuple


class NodeId(NamedTuple):
    """Transient identity of a topology node within an ``occtl_graph_t``.

    Two ``NodeId`` values compare equal iff their ``bits`` are equal. The
    ``bits`` field is opaque — do **not** depend on its layout.

    Use ``Graph.uid_from_node_id`` to convert to a :class:`Uid` that survives
    graph compaction.
    """

    bits: int

    def __repr__(self) -> str:
        return f"NodeId(0x{self.bits:016x})"

    @property
    def is_valid(self) -> bool:
        """``True`` if ``bits`` is non-zero (the all-zero value is reserved)."""
        return self.bits != 0


class Uid(NamedTuple):
    """Persistent identity of a graph entity, survives ``Graph.compact()``.

    The all-zero value matches ``OCCTL_UID_INVALID`` in the C ABI.
    """

    bits: int

    def __repr__(self) -> str:
        return f"Uid(0x{self.bits:016x})"

    @property
    def is_valid(self) -> bool:
        return self.bits != 0


class RefId(NamedTuple):
    """Identity of a reference-entry (a usage of a node from another node).

    Returned by the topology graph's incidence-table accessors when the caller
    cares about the edge *as a usage* rather than the underlying definition.
    """

    bits: int

    def __repr__(self) -> str:
        return f"RefId(0x{self.bits:016x})"

    @property
    def is_valid(self) -> bool:
        return self.bits != 0


class RefUid(NamedTuple):
    """Persistent identity of a reference-entry, survives ``Graph.compact()``."""

    bits: int

    def __repr__(self) -> str:
        return f"RefUid(0x{self.bits:016x})"

    @property
    def is_valid(self) -> bool:
        return self.bits != 0


class RepId(NamedTuple):
    """Identity of a representation attached to a node (Surface, Curve, …)."""

    bits: int

    def __repr__(self) -> str:
        return f"RepId(0x{self.bits:016x})"

    @property
    def is_valid(self) -> bool:
        return self.bits != 0


class RepUid(NamedTuple):
    """Persistent identity of a representation entry, survives ``Graph.compact()``."""

    bits: int

    def __repr__(self) -> str:
        return f"RepUid(0x{self.bits:016x})"

    @property
    def is_valid(self) -> bool:
        return self.bits != 0


# Constants exported at the package level.
NODE_ID_INVALID = NodeId(0)
UID_INVALID = Uid(0)
REF_ID_INVALID = RefId(0)
REF_UID_INVALID = RefUid(0)
REP_ID_INVALID = RepId(0)
REP_UID_INVALID = RepUid(0)
