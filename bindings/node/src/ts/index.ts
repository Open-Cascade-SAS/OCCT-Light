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

export { ABI_VERSION, BINDING_VERSION, runtimeAbiVersion, init, STRUCT_VERSIONS, AVAILABLE_MODULES } from './abi.js';
export {
  Status, OcctLError,
  GenericError, InvalidArgumentError, InvalidHandleError, NotFoundError,
  OutOfMemoryError, OutOfRangeError, NotDoneError, GeometryInvalidError,
  TopologyInvalidError, IoError, FormatError, UnsupportedError,
  CancelledError, BufferTooSmallError, VersionMismatchError, InternalError,
  WrongKindError, AbiMismatchError,
} from './errors.js';
export type {
  NodeId, Uid, RefId, RefUid, RepId, RepUid,
} from './ids.js';
export {
  NODE_ID_INVALID, UID_INVALID, REF_ID_INVALID, REF_UID_INVALID, REP_ID_INVALID, REP_UID_INVALID,
} from './ids.js';
export { Graph } from './handles.js';

export * as core from './generated/core.js';
export * as geom from './generated/geom.js';
export * as topo from './generated/topo.js';
export * as prim from './generated/prim.js';
export * as text from './generated/text.js';
export * as bool_ from './bool_.js';
export * as mesh from './generated/mesh.js';
export * as de from './de.js';
export * as io_brep from './generated/io_brep.js';
export * as io_step from './generated/io_step.js';
export * as io_iges from './generated/io_iges.js';
export * as io_stl from './generated/io_stl.js';
export * as io_obj from './generated/io_obj.js';
export * as io_gltf from './generated/io_gltf.js';
export * as io_vrml from './generated/io_vrml.js';
export * as io_ply from './generated/io_ply.js';
export * as viz from './generated/viz.js';
export * as heal from './generated/heal.js';

export { EXPORTED_FUNCTIONS } from './generated/_raw.gen.js';
