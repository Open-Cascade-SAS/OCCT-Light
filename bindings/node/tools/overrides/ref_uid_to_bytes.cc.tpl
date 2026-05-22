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
    Napi::TypeError::New(env, "ref_uid_to_bytes: expected refUid").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  uint64_t bits = 0;
  Napi::Value value = args.Get(0u);
  if (value.IsBigInt()) {
    bool lossless = false;
    bits = value.As<Napi::BigInt>().Uint64Value(&lossless);
  } else if (value.IsNumber()) {
    bits = (uint64_t)value.ToNumber().Int64Value();
  } else if (value.IsObject() && value.As<Napi::Object>().Has("bits")) {
    Napi::Value bit_value = value.As<Napi::Object>().Get("bits");
    if (bit_value.IsBigInt()) {
      bool lossless = false;
      bits = bit_value.As<Napi::BigInt>().Uint64Value(&lossless);
    } else {
      bits = (uint64_t)bit_value.ToNumber().Int64Value();
    }
  } else {
    Napi::TypeError::New(env, "ref_uid_to_bytes: refUid must be a BigInt or {bits}").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  occtl_ref_uid_t uid = { bits };
  uint8_t out_bytes[OCCTL_REF_UID_WIRE_SIZE] = {};
  occtl_status_t status = occtl_ref_uid_to_bytes(uid, out_bytes);
  if (status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }
  return Napi::Buffer<uint8_t>::Copy(env, out_bytes, OCCTL_REF_UID_WIRE_SIZE);
