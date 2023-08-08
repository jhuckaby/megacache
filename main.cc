// MegaCache v1.0
// Copyright (c) 2023 Joseph Huckaby

#include <napi.h>
#include "cache.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return MegaCache::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitAll)
