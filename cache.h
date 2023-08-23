// MegaCache v1.0
// Copyright (c) 2023 Joseph Huckaby

#ifndef MEGACACHE_H
#define MEGACACHE_H

#include <napi.h>
#include "MegaCache.h"

class MegaCache : public Napi::ObjectWrap<MegaCache> {
public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);
	MegaCache(const Napi::CallbackInfo& info);
	~MegaCache();

private:
	static Napi::FunctionReference constructor;

	Napi::Value Set(const Napi::CallbackInfo& info);
	Napi::Value Get(const Napi::CallbackInfo& info);
	Napi::Value Peek(const Napi::CallbackInfo& info);
	Napi::Value Has(const Napi::CallbackInfo& info);
	Napi::Value Remove(const Napi::CallbackInfo& info);
	Napi::Value Clear(const Napi::CallbackInfo& info);
	Napi::Value Stats(const Napi::CallbackInfo& info);
	Napi::Value FirstKey(const Napi::CallbackInfo& info);
	Napi::Value NextKey(const Napi::CallbackInfo& info);
	Napi::Value LastKey(const Napi::CallbackInfo& info);
	Napi::Value PrevKey(const Napi::CallbackInfo& info);

	Hash *hash;
};

#endif
