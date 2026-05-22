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

  if (args.Length() < 1) {
    Napi::TypeError::New(env, "ref_uid_from_bytes: expected bytes").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const uint8_t* data = nullptr;
  size_t size = 0;
  Napi::Value value = args.Get(0u);
  if (value.IsBuffer()) {
    Napi::Buffer<uint8_t> buffer = value.As<Napi::Buffer<uint8_t>>();
    data = buffer.Data();
    size = buffer.Length();
  } else if (value.IsTypedArray()) {
    Napi::TypedArray array = value.As<Napi::TypedArray>();
    if (array.TypedArrayType() != napi_uint8_array && array.TypedArrayType() != napi_uint8_clamped_array) {
      Napi::TypeError::New(env, "ref_uid_from_bytes: typed array must be Uint8Array").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    Napi::ArrayBuffer ab = array.ArrayBuffer();
    data = static_cast<const uint8_t*>(ab.Data()) + array.ByteOffset();
    size = array.ByteLength();
  } else if (value.IsArrayBuffer()) {
    Napi::ArrayBuffer ab = value.As<Napi::ArrayBuffer>();
    data = static_cast<const uint8_t*>(ab.Data());
    size = ab.ByteLength();
  } else {
    Napi::TypeError::New(env, "ref_uid_from_bytes: expected Buffer, Uint8Array, or ArrayBuffer").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (size < OCCTL_REF_UID_WIRE_SIZE) {
    Napi::RangeError::New(env, "ref_uid_from_bytes: buffer is smaller than OCCTL_REF_UID_WIRE_SIZE").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  occtl_ref_uid_t uid = OCCTL_REF_UID_INVALID;
  occtl_status_t status = occtl_ref_uid_from_bytes(data, &uid);
  if (status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }
  return Napi::BigInt::New(env, (uint64_t)uid.bits);
