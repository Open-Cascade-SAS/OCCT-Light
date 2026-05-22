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

/**
 * @file occtl_core.h
 * @brief OCCT-Light: core public API.
 *
 * Status codes, thread-local error reporting, runtime lifecycle, version
 * queries. The base every other OCCT-Light module depends on.
 *
 * @see ../../docs/design/ABI_PATTERNS.md for ABI conventions.
 * @see ../../docs/design/ARCHITECTURE.md for the layered design.
 */

#ifndef OCCTL_CORE_H
#define OCCTL_CORE_H

#include <stddef.h>
#include <stdint.h>

/**
 * Mathematical constants shared across public C headers.
 *
 * These macros are used by options INIT defaults so callers and bindings can
 * refer to one named source of truth instead of repeating magic literals.
 */
#define OCCTL_PI 3.14159265358979323846

/** 2 * #OCCTL_PI. */
#define OCCTL_TWO_PI (2.0 * OCCTL_PI)

/** #OCCTL_PI / 2. */
#define OCCTL_PI_OVER_TWO (0.5 * OCCTL_PI)

/** Radians in one degree. */
#define OCCTL_RAD_PER_DEG (OCCTL_PI / 180.0)

/** 1 degree in radians. */
#define OCCTL_ANGLE_1_DEG_RAD OCCTL_RAD_PER_DEG

/** 5 degrees in radians. */
#define OCCTL_ANGLE_5_DEG_RAD (5.0 * OCCTL_RAD_PER_DEG)

/** 20 degrees in radians. */
#define OCCTL_ANGLE_20_DEG_RAD (20.0 * OCCTL_RAD_PER_DEG)

/** 30 degrees in radians. */
#define OCCTL_ANGLE_30_DEG_RAD (30.0 * OCCTL_RAD_PER_DEG)

/** 90 degrees in radians. */
#define OCCTL_ANGLE_90_DEG_RAD OCCTL_PI_OVER_TWO

/**
 * Library SemVer. Refer at compile time; for runtime checks use
 * #occtl_runtime_version.
 *
 * These are defined by the build system. Defaults are zero-or-fallback
 * if a non-CMake build forgets to set them.
 */
#ifndef OCCTL_VERSION_MAJOR
  #define OCCTL_VERSION_MAJOR 0
#endif
#ifndef OCCTL_VERSION_MINOR
  #define OCCTL_VERSION_MINOR 0
#endif
#ifndef OCCTL_VERSION_PATCH
  #define OCCTL_VERSION_PATCH 0
#endif

/**
 * ABI version. Bumped only on hard breakage. Independent of SemVer.
 *
 * Consumers may compare this against #occtl_runtime_abi_version at
 * load time and refuse to proceed on mismatch.
 */
#ifndef OCCTL_ABI_VERSION
  #define OCCTL_ABI_VERSION 1
#endif

/**
 * Symbol export macro. Applied to every public function declaration.
 *
 * - Building shared on Windows  : `__declspec(dllexport)`
 * - Consuming shared on Windows : `__declspec(dllimport)`
 * - Building or consuming static: empty
 * - All non-Windows platforms   : `__attribute__((visibility("default")))`
 */
#if defined(_WIN32) && !defined(OCCTL_STATIC_BUILD)
  #if defined(OCCTL_BUILD_SHARED)
    #define OCCTL_API __declspec(dllexport)
  #else
    #define OCCTL_API __declspec(dllimport)
  #endif
  #define OCCTL_CALL __cdecl
#else
  #if defined(__GNUC__) || defined(__clang__)
    #define OCCTL_API __attribute__((visibility("default")))
  #else
    #define OCCTL_API
  #endif
  #define OCCTL_CALL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Status code returned by every OCCT-Light public function.
 *
 * @c OCCTL_OK is 0; idiomatic checks `if (occtl_xxx(...) != OCCTL_OK)`
 * and `if (s) goto fail;` both work.
 *
 * The numeric values listed here are stable forever; new values may be
 * appended in future versions. Consumers that switch on the code must
 * default unknown values to a generic-failure code path.
 */
typedef enum occtl_status
{
  OCCTL_OK               = 0,  /**< Success. */
  OCCTL_ERROR            = 1,  /**< Generic failure; see #occtl_error_last. */
  OCCTL_INVALID_ARGUMENT = 2,  /**< A required pointer was NULL or an argument was malformed. */
  OCCTL_INVALID_HANDLE   = 3,  /**< A handle was NULL or had been freed. */
  OCCTL_NOT_FOUND        = 4,  /**< Requested entity does not exist. */
  OCCTL_OUT_OF_MEMORY    = 5,  /**< Allocation failed. */
  OCCTL_OUT_OF_RANGE     = 6,  /**< A numeric argument was outside the valid range. */
  OCCTL_NOT_DONE         = 7,  /**< An OCCT operation reported `IsDone()==false`. */
  OCCTL_GEOMETRY_INVALID = 8,  /**< Input geometry was not well-formed. */
  OCCTL_TOPOLOGY_INVALID = 9,  /**< Input topology was not well-formed. */
  OCCTL_IO_ERROR         = 10, /**< Filesystem or I/O failure. */
  OCCTL_FORMAT_ERROR     = 11, /**< File contents were malformed. */
  OCCTL_UNSUPPORTED      = 12, /**< Operation not supported in this build/configuration. */
  OCCTL_CANCELLED        = 13, /**< A cooperative cancellation was honoured. */
  OCCTL_BUFFER_TOO_SMALL = 14, /**< Two-call pattern: caller buffer was too small. */
  OCCTL_VERSION_MISMATCH = 15, /**< Options struct_version is not supported. */
  OCCTL_INTERNAL         = 16, /**< C++ exception caught at the ABI boundary. */
  OCCTL_WRONG_KIND       = 17, /**< Handle holds a different kind than the extractor expects. */

  /** Reserved sentinel forces the storage to int32_t and reserves space for new values. */
  OCCTL_STATUS_RESERVED_FUTURE = 0x7fffffff
} occtl_status_t;

/** @return non-zero if @p status indicates failure. */
#define OCCTL_FAILED(status) ((status) != OCCTL_OK)

/** @return non-zero if @p status indicates success. */
#define OCCTL_SUCCEEDED(status) ((status) == OCCTL_OK)

/**
 * Returns a human-readable name for a status code.
 *
 * Useful for logging. The string is library-owned and valid for the
 * lifetime of the process; do not free it.
 *
 * @param[in] status A status code.
 *
 * @return Borrowed pointer to a NUL-terminated UTF-8 string. Returns
 *         "OCCTL_UNKNOWN" if @p status is not a recognised value.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_error_last
 */
OCCTL_API const char* OCCTL_CALL occtl_status_to_string(occtl_status_t status);

/**
 * Node kind discriminator.
 *
 * Applies to #occtl_node_id_t, #occtl_uid_t, and #occtl_ref_id_t payloads.
 * Values are stable forever; new kinds are appended before
 * #OCCTL_NODE_KIND_RESERVED_FUTURE.  @c OCCTL_KIND_INVALID = 0 is the
 * zero sentinel so @c calloc / @c memset-zero IDs are naturally invalid.
 */
typedef enum occtl_node_kind
{
  OCCTL_KIND_INVALID              = 0,  /**< Invalid / zero sentinel. */
  OCCTL_KIND_SOLID                = 1,  /**< Solid. */
  OCCTL_KIND_SHELL                = 2,  /**< Shell. */
  OCCTL_KIND_FACE                 = 3,  /**< Face. */
  OCCTL_KIND_WIRE                 = 4,  /**< Wire. */
  OCCTL_KIND_EDGE                 = 5,  /**< Edge. */
  OCCTL_KIND_VERTEX               = 6,  /**< Vertex. */
  OCCTL_KIND_COMPOUND             = 7,  /**< Compound. */
  OCCTL_KIND_COMPSOLID            = 8,  /**< CompSolid. */
  OCCTL_KIND_COEDGE               = 9,  /**< CoEdge (half-edge). */
  OCCTL_KIND_PRODUCT              = 10, /**< Product (assembly root). */
  OCCTL_KIND_OCCURRENCE           = 11, /**< Occurrence (assembly instance). */
  OCCTL_NODE_KIND_RESERVED_FUTURE = 0x7fffffff
} occtl_node_kind_t;

/**
 * Reference-entry kind discriminator for #occtl_ref_id_t.
 *
 * Reference entries express *usages* of a node (e.g. a Face used inside
 * a Shell).  Zero is reserved as the universal invalid sentinel.
 */
typedef enum occtl_ref_kind
{
  OCCTL_REF_KIND_INVALID         = 0, /**< Invalid / zero sentinel. */
  OCCTL_REF_KIND_SHELL           = 1, /**< Usage of a Shell definition. */
  OCCTL_REF_KIND_FACE            = 2, /**< Usage of a Face definition. */
  OCCTL_REF_KIND_WIRE            = 3, /**< Usage of a Wire definition. */
  OCCTL_REF_KIND_COEDGE          = 4, /**< Usage of a CoEdge definition. */
  OCCTL_REF_KIND_VERTEX          = 5, /**< Usage of a Vertex definition. */
  OCCTL_REF_KIND_SOLID           = 6, /**< Usage of a Solid definition. */
  OCCTL_REF_KIND_CHILD           = 7, /**< Generic mixed-kind child reference. */
  OCCTL_REF_KIND_OCCURRENCE      = 8, /**< Usage of an Occurrence. */
  OCCTL_REF_KIND_RESERVED_FUTURE = 0x7fffffff
} occtl_ref_kind_t;

/**
 * Representation kind discriminator for #occtl_rep_id_t.
 *
 * Representations are the data attached to a node — geometry (Surface,
 * Curve3D, Curve2D) or mesh (Triangulation, Polygon3D, Polygon2D,
 * PolygonOnTri).  Unrecognised representation kinds are surfaced as
 * #OCCTL_REP_KIND_INVALID by the current ABI.
 */
typedef enum occtl_rep_kind
{
  OCCTL_REP_KIND_INVALID         = 0, /**< Invalid / zero sentinel. */
  OCCTL_REP_KIND_SURFACE         = 1, /**< 3D surface geometry. */
  OCCTL_REP_KIND_CURVE3D         = 2, /**< 3D curve geometry. */
  OCCTL_REP_KIND_CURVE2D         = 3, /**< 2D parametric curve on a face (PCurve). */
  OCCTL_REP_KIND_TRIANGULATION   = 4, /**< Surface triangulation. */
  OCCTL_REP_KIND_POLYGON3D       = 5, /**< 3D polyline along an edge. */
  OCCTL_REP_KIND_POLYGON2D       = 6, /**< 2D polyline. */
  OCCTL_REP_KIND_POLYGON_ON_TRI  = 7, /**< Polyline indexed onto a triangulation. */
  OCCTL_REP_KIND_RESERVED_FUTURE = 0x7fffffff
} occtl_rep_kind_t;

/**
 * Identity of a representation (geometry / mesh data) in the graph.
 *
 * The all-zero value (#OCCTL_REP_ID_INVALID) is the invalid sentinel.
 */
typedef struct occtl_rep_id
{
  uint64_t bits; /**< Opaque 64-bit representation identity. */
} occtl_rep_id_t;

#ifdef __cplusplus
  #define OCCTL_REP_ID_INVALID (occtl_rep_id_t{0})
#else
  #define OCCTL_REP_ID_INVALID ((occtl_rep_id_t){0})
#endif

/**
 * Persistent unique identity of a graph entity.
 *
 * Survives compact and most node removals, unlike #occtl_node_id_t which
 * is session-local.  The all-zero value (#OCCTL_UID_INVALID) is the
 * invalid sentinel — compare with @c uid.bits == 0.
 *
 * To query the kind of a UID, call @c occtl_graph_uid_kind() (declared in
 * @c occtl_topo.h when @c OCCTL_HAS_TOPO is defined).
 */
typedef struct occtl_uid
{
  uint64_t bits;
} occtl_uid_t;

/**
 * Invalid UID sentinel.  Equivalent to @c (occtl_uid_t){0}.
 *
 * Defined as a C99 compound literal in C and as a C++ braced
 * initialiser in C++; this avoids compound-literal warnings on MSVC
 * and pedantic C++ modes.
 */
#ifdef __cplusplus
  #define OCCTL_UID_INVALID (occtl_uid_t{0})
#else
  #define OCCTL_UID_INVALID ((occtl_uid_t){0})
#endif

/**
 * Thread-local description of the most recent failure.
 *
 * Populated automatically when any public function returns a non-OK
 * status. Read it with #occtl_error_last; clear it with
 * #occtl_error_clear.
 *
 * The @c message pointer is library-owned and is **only valid until
 * the next OCCT-Light call on this thread**. Copy it out if you need
 * to retain it.
 */
typedef struct occtl_error
{
  occtl_status_t status;   /**< Primary status code. */
  const char*    message;  /**< UTF-8, library-owned, see lifetime note above. */
  occtl_uid_t    source;   /**< UID of the offending entity, or all-zeros if not applicable. */
  uint32_t       extended; /**< Optional SQLite-style extended subcode. */
} occtl_error_t;

/**
 * Returns the thread-local last error.
 *
 * Always returns a non-NULL pointer; an all-zero @c status field means
 * "no error pending on this thread."
 *
 * @return Borrowed pointer to thread-local storage. Valid until the
 *         next OCCT-Light call on this thread.
 *
 * @threadsafe Yes (per-thread).
 *
 * @sa occtl_error_clear, occtl_status_to_string
 */
OCCTL_API const occtl_error_t* OCCTL_CALL occtl_error_last(void);

/**
 * Clears the thread-local last error.
 *
 * Calling this is optional — every public function that returns
 * @c OCCTL_OK already clears the slot. Use it explicitly when you want
 * to assert "no prior failure" before a sequence of calls.
 *
 * @threadsafe Yes (per-thread).
 *
 * @sa occtl_error_last
 */
OCCTL_API void OCCTL_CALL occtl_error_clear(void);

/**
 * Init info versioning constant. New fields are appended under new
 * version constants in future releases.
 */
#define OCCTL_RUNTIME_INIT_INFO_VERSION_1 1u

/**
 * Options for #occtl_runtime_init.
 *
 * Construct with #OCCTL_RUNTIME_INIT_INFO_INIT or by zero-initialising
 * and setting @c struct_version explicitly.
 */
typedef struct occtl_runtime_init_info
{
  uint32_t    struct_version; /**< Set to #OCCTL_RUNTIME_INIT_INFO_VERSION_1. */
  const void* p_next;         /**< Reserved for future extension chains; must be NULL. */
} occtl_runtime_init_info_t;

/** Static initializer for #occtl_runtime_init_info_t. */
#define OCCTL_RUNTIME_INIT_INFO_INIT {OCCTL_RUNTIME_INIT_INFO_VERSION_1, NULL}

/**
 * Initialises the OCCT-Light runtime.
 *
 * Calling this is **optional**: every entry point lazy-initialises with
 * defaults if needed.
 *
 * Calling more than once is a programming error; subsequent calls
 * return @c OCCTL_INVALID_ARGUMENT and have no effect.
 *
 * @param[in] info Borrows it. Optional. NULL = use defaults. When non-NULL,
 *                 @c info->struct_version must be a recognised value.
 *
 * @retval OCCTL_OK                On success or if defaults already applied.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version is unsupported.
 * @retval OCCTL_INVALID_ARGUMENT  Already initialised, or an invalid combination.
 *
 * @threadsafe No (call before any other thread issues OCCT-Light calls).
 *
 * @sa occtl_runtime_shutdown, occtl_runtime_version
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_runtime_init(const occtl_runtime_init_info_t* info);

/**
 * Initialises an #occtl_runtime_init_info_t to the default values.
 *
 * Equivalent to @c \#OCCTL_RUNTIME_INIT_INFO_INIT but callable from
 * binding languages that cannot use C macros.
 *
 * @param[out] info Owns it (caller-allocated). Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p info is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_runtime_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_runtime_init_info_init(occtl_runtime_init_info_t* info);

/**
 * Releases process-wide runtime state allocated by #occtl_runtime_init.
 *
 * After this returns, OCCT-Light entry points may still be called —
 * they will lazy-initialise again with defaults. Call this only at
 * process teardown when you want a clean state.
 *
 * @threadsafe No (no other thread may issue OCCT-Light calls during shutdown).
 *
 * @sa occtl_runtime_init
 */
OCCTL_API void OCCTL_CALL occtl_runtime_shutdown(void);

/**
 * Returns the runtime SemVer.
 *
 * Out parameters may individually be NULL if the caller doesn't want
 * a particular field; pass NULL for all three to get a no-op call.
 *
 * @param[out] out_major Borrows it.  Optional.
 * @param[out] out_minor Borrows it.  Optional.
 * @param[out] out_patch Borrows it.  Optional.
 *
 * @threadsafe Yes.
 *
 * @sa OCCTL_VERSION_MAJOR, occtl_runtime_abi_version
 */
OCCTL_API void OCCTL_CALL occtl_runtime_version(uint32_t* out_major,
                                                uint32_t* out_minor,
                                                uint32_t* out_patch);

/**
 * Returns the runtime ABI version.
 *
 * Compare to #OCCTL_ABI_VERSION at load time and refuse to load if
 * they differ — that protects against mixing a newer header with an
 * older binary.
 *
 * @return The ABI version compiled into this build.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_runtime_version
 */
OCCTL_API uint32_t OCCTL_CALL occtl_runtime_abi_version(void);

/**
 * Returns the OCCT version OCCT-Light was built against.
 *
 * Diagnostic only; not part of OCCT-Light's ABI promise.
 *
 * @return Borrowed pointer to a NUL-terminated UTF-8 string of the
 *         form `"7.x.y"`. Library-owned, valid for the process lifetime.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_runtime_version
 */
OCCTL_API const char* OCCTL_CALL occtl_runtime_occt_version(void);

/**
 * Number of bytes in the wire-format encoding of an #occtl_uid_t.
 *
 * The wire format is a fixed-width opaque blob; do not parse it.  The
 * top 8 bytes carry the current 64-bit packed identity in big-endian
 * order; the bottom 8 bytes are reserved (always zero today) so that
 * future widening of the kind tag or counter field can re-key existing
 * stored UIDs without breaking persistence.
 */
#define OCCTL_UID_WIRE_SIZE 16u

/**
 * Encodes a UID into its 16-byte wire format.
 *
 * Round-trips losslessly through #occtl_uid_from_bytes for any valid
 * or invalid UID.  Byte order is fixed at big-endian and stable across
 * future ABI versions.
 *
 * @param[in]  uid       UID to encode (may be #OCCTL_UID_INVALID).
 * @param[out] out_bytes Borrows it.  Must point to a writable buffer
 *                       of at least #OCCTL_UID_WIRE_SIZE bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_bytes is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_uid_from_bytes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_uid_to_bytes(occtl_uid_t uid, uint8_t* out_bytes);

/**
 * Decodes a 16-byte wire-format UID back into an #occtl_uid_t.
 *
 * Rejects payloads whose reserved bytes are non-zero (returns
 * #OCCTL_FORMAT_ERROR) — those signal a future wider encoding that
 * this build does not understand.  An all-zero input returns
 * #OCCTL_UID_INVALID with #OCCTL_OK.
 *
 * @param[in]  in_bytes Borrows it.  Must point to a readable buffer
 *                      of at least #OCCTL_UID_WIRE_SIZE bytes.
 * @param[out] out_uid  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p in_bytes or @p out_uid is NULL.
 * @retval OCCTL_FORMAT_ERROR      Reserved bytes are non-zero.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_uid_to_bytes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_uid_from_bytes(const uint8_t* in_bytes,
                                                         occtl_uid_t*   out_uid);

/**
 * Tests two UIDs for equality.
 *
 * @param[in] a  First UID.
 * @param[in] b  Second UID.
 * @retval 1  UIDs are equal.
 * @retval 0  UIDs differ.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_uid_to_bytes, occtl_uid_from_bytes
 */
OCCTL_API int32_t OCCTL_CALL occtl_uid_equal(occtl_uid_t a, occtl_uid_t b);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_CORE_H */
