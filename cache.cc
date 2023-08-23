// MegaCache v1.0
// Copyright (c) 2023 Joseph Huckaby

#include <stdio.h>
#include <stdint.h>
#include "cache.h"

Napi::FunctionReference MegaCache::constructor;

Napi::Object MegaCache::Init(Napi::Env env, Napi::Object exports) {
	// initialize class
	Napi::HandleScope scope(env);
	
	Napi::Function func = DefineClass(env, "MegaCache", {
		InstanceMethod("_set", &MegaCache::Set),
		InstanceMethod("_get", &MegaCache::Get),
		InstanceMethod("_peek", &MegaCache::Peek),
		InstanceMethod("_has", &MegaCache::Has),
		InstanceMethod("_remove", &MegaCache::Remove),
		InstanceMethod("clear", &MegaCache::Clear),
		InstanceMethod("stats", &MegaCache::Stats),
		InstanceMethod("_firstKey", &MegaCache::FirstKey),
		InstanceMethod("_nextKey", &MegaCache::NextKey),
		InstanceMethod("_lastKey", &MegaCache::LastKey),
		InstanceMethod("_prevKey", &MegaCache::PrevKey)
	});
	
	constructor = Napi::Persistent(func);
 	constructor.SuppressDestruct();
 	
	exports.Set("MegaCache", func);
	return exports;
}

MegaCache::MegaCache(const Napi::CallbackInfo& info) : Napi::ObjectWrap<MegaCache>(info) {
	// construct new hash table
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	// 8 buckets per list with 16 scatter is about the perfect balance of speed and memory
	// FUTURE: Make this configurable from Node.js side?
	this->hash = new Hash( 8, 16 );
	
	// allow maxKeys and maxBytes to be passed in as ctor args
	if (info.Length() > 0) {
		this->hash->maxKeys = (uint64_t)info[0].As<Napi::Number>().Uint32Value();
	}
	if (info.Length() > 1) {
		this->hash->maxBytes = (uint64_t)info[1].As<Napi::Number>().Uint32Value();
	}
}

MegaCache::~MegaCache() {
	// cleanup and free memory
	delete this->hash;
}

Napi::Value MegaCache::Set(const Napi::CallbackInfo& info) {
	// store key/value pair, no return value
	Napi::Env env = info.Env();
	
	Napi::Buffer<unsigned char> keyBuf = info[0].As<Napi::Buffer<unsigned char>>();
	unsigned char *key = keyBuf.Data();
	MH_KLEN_T keyLength = (MH_KLEN_T)keyBuf.Length();
	
	Napi::Buffer<unsigned char> valueBuf = info[1].As<Napi::Buffer<unsigned char>>();
	unsigned char *value = valueBuf.Data();
	MH_LEN_T valueLength = (MH_LEN_T)valueBuf.Length();
	
	unsigned char flags = 0;
	if (info.Length() > 2) {
		flags = (unsigned char)info[2].As<Napi::Number>().Uint32Value();
	}
	
	Response resp = this->hash->store( key, keyLength, value, valueLength, flags );
	return Napi::Number::New(env, (double)resp.result);
}

Napi::Value MegaCache::Get(const Napi::CallbackInfo& info) {
	// fetch value given key
	Napi::Env env = info.Env();
	
	Napi::Buffer<unsigned char> keyBuf = info[0].As<Napi::Buffer<unsigned char>>();
	unsigned char *key = keyBuf.Data();
	MH_KLEN_T keyLength = (MH_KLEN_T)keyBuf.Length();
	
	Response resp = this->hash->fetch( key, keyLength );
	
	if (resp.result == MH_OK) {
		Napi::Buffer<unsigned char> valueBuf = Napi::Buffer<unsigned char>::Copy( env, resp.content, resp.contentLength );
		if (!valueBuf) return env.Undefined();
		
		if (resp.flags) valueBuf.Set( "flags", (double)resp.flags );
		return valueBuf;
	}
	else return env.Undefined();
}

Napi::Value MegaCache::Peek(const Napi::CallbackInfo& info) {
	// fetch value given key, do not promote
	Napi::Env env = info.Env();
	
	Napi::Buffer<unsigned char> keyBuf = info[0].As<Napi::Buffer<unsigned char>>();
	unsigned char *key = keyBuf.Data();
	MH_KLEN_T keyLength = (MH_KLEN_T)keyBuf.Length();
	
	Response resp = this->hash->peek( key, keyLength );
	
	if (resp.result == MH_OK) {
		Napi::Buffer<unsigned char> valueBuf = Napi::Buffer<unsigned char>::Copy( env, resp.content, resp.contentLength );
		if (!valueBuf) return env.Undefined();
		
		if (resp.flags) valueBuf.Set( "flags", (double)resp.flags );
		return valueBuf;
	}
	else return env.Undefined();
}

Napi::Value MegaCache::Has(const Napi::CallbackInfo& info) {
	// see if a key exists, return boolean true/value
	Napi::Env env = info.Env();
	
	Napi::Buffer<unsigned char> keyBuf = info[0].As<Napi::Buffer<unsigned char>>();
	unsigned char *key = keyBuf.Data();
	MH_KLEN_T keyLength = (MH_KLEN_T)keyBuf.Length();
	
	Response resp = this->hash->peek( key, keyLength );
	return Napi::Boolean::New(env, (resp.result == MH_OK));
}

Napi::Value MegaCache::Remove(const Napi::CallbackInfo& info) {
	// remove key/value pair, free up memory
	Napi::Env env = info.Env();
	
	Napi::Buffer<unsigned char> keyBuf = info[0].As<Napi::Buffer<unsigned char>>();
	unsigned char *key = keyBuf.Data();
	MH_KLEN_T keyLength = (MH_KLEN_T)keyBuf.Length();
	
	Response resp = this->hash->remove( key, keyLength );
	return Napi::Boolean::New(env, (resp.result == MH_OK));
}

Napi::Value MegaCache::Clear(const Napi::CallbackInfo& info) {
	// delete some or all keys/values from hash, free all memory
	unsigned char slice1 = 0;
	unsigned char slice2 = 0;
	
	if (info.Length() == 2) {
		// clear thin slice
		slice1 = (unsigned char)info[0].As<Napi::Number>().Uint32Value();
		slice2 = (unsigned char)info[1].As<Napi::Number>().Uint32Value();
		this->hash->clear( slice1, slice2 );
	}
	else if (info.Length() == 1) {
		// clear thick slice
		slice1 = (unsigned char)info[0].As<Napi::Number>().Uint32Value();
		this->hash->clear( slice1 );
	}
	else {
		// clear all
		this->hash->clear();
	}
	
	return info.Env().Undefined();
}

Napi::Value MegaCache::Stats(const Napi::CallbackInfo& info) {
	// return stats as node object
	Napi::Env env = info.Env();
	
	Napi::Object obj = Napi::Object::New(env);
	obj.Set(Napi::String::New(env, "indexSize"), (double)this->hash->stats->indexSize);
	obj.Set(Napi::String::New(env, "metaSize"), (double)this->hash->stats->metaSize);
	obj.Set(Napi::String::New(env, "dataSize"), (double)this->hash->stats->dataSize);
	obj.Set(Napi::String::New(env, "numKeys"), (double)this->hash->stats->numKeys);
	obj.Set(Napi::String::New(env, "numIndexes"), (double)(this->hash->stats->indexSize / (int)sizeof(Index)));
	obj.Set(Napi::String::New(env, "numEvictions"), (double)this->hash->stats->numEvictions);
	
	return obj;
}

Napi::Value MegaCache::FirstKey(const Napi::CallbackInfo& info) {
	// return first key in hash (in descending popular order)
	Napi::Env env = info.Env();
	
	Response resp = this->hash->firstKey();
	if (resp.result == MH_OK) {
		return Napi::Buffer<unsigned char>::Copy( env, resp.content, resp.contentLength );
	}
	else return env.Undefined();
}

Napi::Value MegaCache::NextKey(const Napi::CallbackInfo& info) {
	// return next key in hash given any key (in descending popular order)
	Napi::Env env = info.Env();
	
	Napi::Buffer<unsigned char> keyBuf = info[0].As<Napi::Buffer<unsigned char>>();
	unsigned char *key = keyBuf.Data();
	MH_KLEN_T keyLength = (MH_KLEN_T)keyBuf.Length();
	
	Response resp = this->hash->nextKey( key, keyLength );
	if (resp.result == MH_OK) {
		return Napi::Buffer<unsigned char>::Copy( env, resp.content, resp.contentLength );
	}
	else return env.Undefined();
}

Napi::Value MegaCache::LastKey(const Napi::CallbackInfo& info) {
	// return last key in hash (in asending popular order)
	Napi::Env env = info.Env();
	
	Response resp = this->hash->lastKey();
	if (resp.result == MH_OK) {
		return Napi::Buffer<unsigned char>::Copy( env, resp.content, resp.contentLength );
	}
	else return env.Undefined();
}

Napi::Value MegaCache::PrevKey(const Napi::CallbackInfo& info) {
	// return previous key in hash given any key (in ascending popular order)
	Napi::Env env = info.Env();
	
	Napi::Buffer<unsigned char> keyBuf = info[0].As<Napi::Buffer<unsigned char>>();
	unsigned char *key = keyBuf.Data();
	MH_KLEN_T keyLength = (MH_KLEN_T)keyBuf.Length();
	
	Response resp = this->hash->prevKey( key, keyLength );
	if (resp.result == MH_OK) {
		return Napi::Buffer<unsigned char>::Copy( env, resp.content, resp.contentLength );
	}
	else return env.Undefined();
}
