<details><summary>Table of Contents</summary>

<!-- toc -->
- [Overview](#overview)
	* [Features](#features)
	* [Performance](#performance)
- [Installation](#installation)
- [Usage](#usage)
	* [Auto-Eviction](#auto-eviction)
	* [Setting and Getting](#setting-and-getting)
		+ [Buffers](#buffers)
		+ [Strings](#strings)
		+ [Objects](#objects)
		+ [Numbers](#numbers)
		+ [BigInts](#bigints)
		+ [Booleans](#booleans)
		+ [Null](#null)
	* [Deleting and Clearing](#deleting-and-clearing)
	* [Iterating over Keys](#iterating-over-keys)
	* [Error Handling](#error-handling)
	* [Cache Stats](#cache-stats)
- [API](#api)
	* [set](#set)
	* [get](#get)
	* [has](#has)
	* [delete](#delete)
	* [clear](#clear)
	* [nextKey](#nextkey)
	* [prevKey](#prevkey)
	* [length](#length)
	* [stats](#stats)
- [Internals](#internals)
	* [Limits](#limits)
	* [Memory Overhead](#memory-overhead)
- [License](#license)

</details>

# Overview

**MegaCache** is a super-fast memory-based C++ [least recently used cache](https://en.wikipedia.org/wiki/Cache_replacement_policies#Least_recently_used_%28LRU%29).  It uses [MegaHash](https://github.com/jhuckaby/megahash) for the hash table, and a [doubly-linked list](https://en.wikipedia.org/wiki/Doubly_linked_list) for tracking key popularity.  Least popular keys can be auto-evicted based on total key count, or total RAM usage.  MegaCache was designed for large production workloads, with millions or even hundreds of millions of keys (tested up to 1 billion).

## Features

- Built using [MegaHash](https://github.com/jhuckaby/megahash).
- Nothing is stored on the V8 heap, everything is in C++ memory.
- No garbage collection delays or hiccups of any kind.
- Tested up to 1 billion keys.
- Can evict keys based on key count or memory usage.
- Low memory overhead (about 46 bytes per key).
- Consistent performance regardless of size.

## Performance

About 1,000,000 keys per second insert, and 1,200,000 keys per second read, but this depends greatly upon key and value size, and the hardware on which it is running.  Your mileage may vary.

# Installation

Use [npm](https://www.npmjs.com/) to install the module locally:

```
npm install pixl-megacache
```

You may need to install a C++ compiler toolchain to build the source into a shared library:

| Platform | Instructions |
|----------|--------------|
| **Linux** | Download and install [GCC](https://gcc.gnu.org).  On RedHat/CentOS, run `sudo yum install gcc gcc-c++ libstdc++-devel pkgconfig make`.  On Debian/Ubuntu, run `sudo apt-get install build-essential`. |
| **macOS** | Download and install [Xcode](https://developer.apple.com/xcode/download/).  You also need to install the `XCode Command Line Tools` by running `xcode-select --install`. Alternatively, if you already have the full Xcode installed, you can find them under the menu `Xcode -> Open Developer Tool -> More Developer Tools...`. This step will install `clang`, `clang++`, and `make`. |
| **Windows** | Install all the required tools and configurations using Microsoft's [windows-build-tools](https://github.com/felixrieseberg/windows-build-tools) using `npm install --global --production windows-build-tools` from an elevated PowerShell or CMD.exe (run as Administrator). |

Once you have that all setup, use `require()` to load MegaCache in your Node.js code:

```js
const MegaCache = require('pixl-megacache');
```

# Usage

Here is a simple example:

```js
let cache = new MegaCache();

cache.set( "hello", "there" );
console.log( cache.get("hello") );
cache.delete("hello");
cache.clear();
```

## Auto-Eviction

MegaCache can automatically evict the least popular keys (i.e. least recently used) based on rules that you specify.  You can set a maximum number of keys, and/or a maximum memory usage limit in bytes.  The first limit that is reached will trigger evictions.  Limits are specified as arguments to the class constructor, in this order:

```js
let cache = new MegaCache( MAX_KEYS, MAX_BYTES );
```

Note that bytes are computed as the total memory usage, including the memory used to store your keys and values, as well as the MegaCache indexing system (hash table and linked list overhead).

Set these to `0` to disable the limit (i.e. infinite), which is the default behavior.

## Setting and Getting

To add or replace a key in a hash, use the [set()](#set) method.  This accepts two arguments, a key and a value:

```js
cache.set( "hello", "there" );
cache.set( "hello", "REPLACED!" );
```

To fetch an existing value given a key, use the [get()](#get) method.  This accepts a single argument, the key:

```js
let value = cache.get("hello");
```

Each time you get or set a key, it is promoted to the head of the LRU list.

The following data types are supported for values:

### Buffers

Buffers are the internal type used by the hash, and will give you the best performance.  This is true for both keys and values, so if you can pass them in as Buffers, all the better.  All other data types besides buffers are auto-converted.  Example use:

```js
let buf = Buffer.allocSafe(32);
buf.write("Hi");
cache.set( "mybuf", buf );

let bufCopy = cache.get("mybuf");
```

It should be noted here that memory is **copied** when it enters and exits MegaCache from Node.js land.  So if you insert a buffer and then retrieve it, you'll get a brand new buffer with a fresh copy of the data.

### Strings

Strings are converted to buffers using UTF-8 encoding.  This includes both keys and values.  However, for values MegaCache remembers the original data type, and will reverse the conversion when getting keys, and return a proper string value to you.  Example:

```js
cache.set( "hello", "there" );
console.log( cache.get("hello") );
```

Keys are returned as strings when iterating using [nextKey()](#nextkey) and [prevKey()](#prevkey).

### Objects

Object values are automatically serialized to JSON, then converted to buffers using UTF-8 encoding.  The reverse procedure occurs when fetching keys, and your values will be returned as proper objects.  Example:

```js
cache.set( "user1", { name: "Joe", age: 43 } );

let user = cache.get("user1");
console.log( user.name, user.age );
```

### Numbers

Number values are auto-converted to double-precision floating point decimals, and stored as 64-bit buffers internally.  Number keys are converted to strings, then to UTF-8 buffers which are used internally.  Example:

```js
cache.set( 1, 9.99999999 );
let value = cache.get(1);
```

### BigInts

MegaCache has support for [BigInt](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/BigInt) numbers, which are automatically detected and converted to/from 64-bit signed integers.  Example:

```js
cache.set( "big", 9007199254740993n );
let value = cache.get("big");
```

### Booleans

Booleans are internally stored as a 1-byte buffer containing `0` or `1`.  These are auto-converted back to Booleans when you fetch keys.  Example:

```js
cache.set("bool1", true);
let test = cache.get("bool1");
```

### Null

You can specify `null` as a hash value, and it will be preserved as such.  Example:

```js
cache.set("nope", null);
```

You cannot, however, use `undefined` as a value.  Doing so will result in undefined behavior (get it?).

## Deleting and Clearing

To delete individual keys, use the [delete()](#delete) method.  Example:

```js
cache.delete("key1");
cache.delete("key2");
```

To delete **all** keys, call [clear()](#clear) (or just delete the cache object -- it'll be garbage collected like any normal Node.js object).  Example:

```js
cache.clear();
```

## Iterating over Keys

To iterate over keys in the hash, you can use the [nextKey()](#nextkey) method.  Without an argument, this will give you the "first" key in descending popular order (most popular first).  If you pass it the previous key, it will give you the next one, until finally `undefined` is returned.  Example:

```js
let key = cache.nextKey();
while (key) {
	// do something with key
	key = cache.nextKey(key);
}
```

Please note that if new keys are added to the cache while an iteration is in progress, it may *miss* some keys, due to indexing (i.e. reshuffling the position of keys).

You can also iterate over keys in *reverse* order (ascending popularity), by using [prevKey()](#prevkey).  Example:

```js
let key = cache.prevKey();
while (key) {
	// do something with key
	key = cache.prevKey(key);
}
```

## Error Handling

If a cache operation fails (i.e. out of memory), then [set()](#set) will return `0`.  You can check for this and bubble up your own error.  Example:

```js
let result = cache.set( "hello", "there" );
if (!result) {
	throw new Error("Failed to write to MegaCache: Out of memory");
}
```

## Cache Stats

To get current statistics about the cache, including the number of keys, raw data size, and other internals, call [stats()](#stats).  Example:

```js
let stats = cache.stats();
console.log(stats);
```

Example stats:

```js
{
	"numKeys": 10000,
	"dataSize": 217780,
	"indexSize": 87992,
	"metaSize": 300000,
	"numIndexes": 647,
	"numEvictions": 0
}
```

The response object will contain the following properties:

| Property Name | Description |
|---------------|-------------|
| `numKeys` | The total number of keys in the hash.  You can also get this by calling [length()](#length). |
| `dataSize` | The total data size in bytes (all of your raw keys and values). |
| `indexSize` | Internal memory usage by the MegaCache indexing system (i.e. overhead), in bytes. |
| `metaSize` | Internal metadata stored alongside your key/value pairs (more overhead), in bytes. |
| `numIndexes` | The number of internal indexes currently in use. |
| `numEvictions` | The number of keys that were kicked out based on your eviction rules, if applicable. |

To compute the total memory overhead, add `indexSize` to `metaSize`.  For total memory usage, add `dataSize` to that.  However, please note that the OS adds its own memory overhead on top of this (i.e. byte alignment, malloc overhead, etc.).

# API

Here is the API reference for the MegaCache instance methods:

## set

```
NUMBER set( KEY, VALUE )
```

Set or replace one key/value in the hash, and promote the key to the front of the LRU list.  Ideally both key and value are passed as Buffers, as this provides the highest performance.  Most built-in data types are supported of course, but they are converted to buffers one way or the other.  Example use:

```js
cache.set( "key1", "value1" );
```

The `set()` method actually returns a number, which will be `0`, `1` or `2`.  They each have a different meaning:

| Result | Description |
|--------|-------------|
| `0` | An error occurred (out of memory). |
| `1` | A key was added to the hash (i.e. unique key). |
| `2` | An existing key was replaced in the hash. |

Calling `set()` may trigger one or more key evictions, if you set any limits in the constructor (see [Auto-Eviction](#auto-eviction)).

## get

```
MIXED get( KEY )
```

Fetch a value given a key, and promote the key to the front of the LRU list.  Since the value data type is stored internally as a flag with the raw data, this is used to convert the buffer back to the original type when the key is fetched.  So if you store a string then fetch it, it'll come back as a string.  Example use:

```js
let value = cache.get("key1");
```

If the key is not found, `get()` will return `undefined`.

## peek

```
MIXED peek( KEY )
```

Fetch a value given a key, similar to [get()](#get), but do **not** promote the key to the front of the LRU list.  Since the value data type is stored internally as a flag with the raw data, this is used to convert the buffer back to the original type when the key is fetched.  So if you store a string then fetch it, it'll come back as a string.  Example use:

```js
let value = cache.peek("key1");
```

If the key is not found, `peek()` will return `undefined`.

## has

```
BOOLEAN has( KEY )
```

Check if a key exists in the hash.  This will **not** promote the key in the LRU list.  It returns `true` if found, `false` if not.  This is faster than a [get()](#get) as the value doesn't have to be serialized or sent over the wall between C++ and Node.js, and the LRU list isn't touched.  Example use:

```js
if (cache.has("key1")) console.log("Exists!");
```

## delete

```
BOOLEAN delete( KEY )
```

Delete one key/value pair from the cache, given the key.  Returns `true` if found, `false` if not.  Example use:

```js
cache.delete("key1");
```

## clear

```
VOID clear()
```

Delete *all* keys from the cache, effectively freeing all memory (except for indexes).  Example use:

```js
cache.clear();
```

You can also optionally pass in one or two 8-bit unsigned integers to this method, to clear an arbitrary "slice" of the total keys.  This is an advanced trick, only used for clearing out extremely large hashes in small chunks, as to not hang the main thread.  Pass in one number between 0 - 256 to clear a "thick slice" (approximately 1/256 of total keys).  Example:

```js
cache.clear( 34 );
```

Pass in *two* numbers between 0 - 256 to clear a "thin slice" (approximately 1/65536 of total keys).  Example:

```js
cache.clear( 84, 191 );
```

## nextKey

```
STRING nextKey()
STRING nextKey( KEY )
```

Without an argument, fetch the *first* key in the hash, i.e. the most popular key.  With a key specified, fetch the *next* key, with descending popularity.  Returns `undefined` when the end of the hash has been reached.  Example use:

```js
let key = cache.nextKey();
while (key) {
	// do something with key
	key = cache.nextKey(key);
}
```

## prevKey

```
STRING prevKey()
STRING prevKey( KEY )
```

Without an argument, fetch the *last* key in the hash, i.e. the least popular key.  With a key specified, fetch the next key with ascending popularity.  Returns `undefined` when the front of the hash has been reached.  Example use:

```js
let key = cache.prevKey();
while (key) {
	// do something with key
	key = cache.prevKey(key);
}
```

## length

```
NUMBER length()
```

Return the total number of keys currently in the cache.  This is very fast, as it does not have to iterate over the keys (an internal counter is kept up to date on each set/delete).  Example use:

```js
let numKeys = cache.length();
```

## stats

```
OBJECT stats()
```

Fetch statistics about the current cache, including the number of keys, total data size in memory, and more.  The return value is a native Node.js object with several properties populated.  Example use:

```js
let stats = cache.stats();

// Example stats:
{
	"numKeys": 10000,
	"dataSize": 217780,
	"indexSize": 87992,
	"metaSize": 300000,
	"numIndexes": 647,
	"numEvictions": 0
}
```

See [Hash Stats](#hash-stats) for more details about these properties.

# Internals

See [MegaHash Internals](https://github.com/jhuckaby/megahash#internals).

## Limits

- Keys can be up to 65,536 bytes each.
- Values can be up to 2 GB each (the size limit of Node.js buffers).
- There is no predetermined total key limit.
- All keys are buffers (strings are encoded with UTF-8), and must be non-zero length.
- Values may be zero length, and are also buffers internally.
- String values are automatically converted to/from UTF-8 buffers.
- Numbers are converted to/from double-precision floats.
- BigInts are converted to/from 64-bit signed integers.
- Object values are automatically serialized to/from JSON.

## Memory Overhead

Each MegaCache index record is 128 bytes (16 pointers, 64-bits each), and each bucket adds 40 bytes of overhead (16 more than MegaHash, to account for the linked list).  The tuple (key + value, along with lengths) is stored as a single blob (single `malloc()` call) to reduce memory fragmentation from allocating the key and value separately.

At 100 million keys, the total memory overhead is approximately 4.1 GB.  At 1 billion keys, it is 41 GB.  This equates to approximately 46 bytes per key.

# License

**The MIT License (MIT)**

*Copyright (c) 2023 Joseph Huckaby*

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
