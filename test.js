// Unit tests for MegaCache
// Copyright (c) 2023 Joseph Huckaby

// Run via: npm test

const MegaCache = require('./');

module.exports = {
	tests: [
		
		function testBasicSet(test) {
			var hash = new MegaCache();
			hash.set("hello", "there");
			var stats = hash.stats();
			test.ok(stats.numKeys === 1, '1 key in stats');
			test.ok(stats.numIndexes === 1, '1 index in stats');
			test.ok(stats.dataSize === 10, '10 bytes in data store');
			test.ok(stats.indexSize > 0, 'Index size is non-zero');
			test.ok(stats.metaSize > 0, 'Meta size is non-zero');
			test.done();
		},
		
		function testString(test) {
			var hash = new MegaCache();
			hash.set("hello", "there");
			var value = hash.get("hello");
			test.ok(value === "there", 'Basic string set/get');
			test.done();
		},
		
		function testUnicodeValue(test) {
			var hash = new MegaCache();
			hash.set("hello", "😃");
			var value = hash.get("hello");
			test.ok(value === "😃", 'Unicode value set/get');
			test.done();
		},
		
		function testUnicodeKey(test) {
			var hash = new MegaCache();
			hash.set("😃", "there");
			var value = hash.get("😃");
			test.ok(value === "there", 'Unicode key set/get');
			test.done();
		},
		
		function testEmptyKeyError(test) {
			test.expect(4);
			var hash = new MegaCache();
			
			try {
				hash.set("", "there");
			}
			catch (err) {
				test.ok( !!err, "Expected error with zero-length key (set)" );
			}
			
			try {
				var value = hash.get("");
			}
			catch (err) {
				test.ok( !!err, "Expected error with zero-length key (get)" );
			}
			
			try {
				var value = hash.has("");
			}
			catch (err) {
				test.ok( !!err, "Expected error with zero-length key (has)" );
			}
			
			try {
				hash.remove("");
			}
			catch (err) {
				test.ok( !!err, "Expected error with zero-length key (remove)" );
			}
			
			test.done();
		},
		
		function testBufferValue(test) {
			var hash = new MegaCache();
			hash.set("hello", Buffer.from("there"));
			var value = hash.get("hello");
			test.ok( Buffer.isBuffer(value), "Value is buffer" );
			test.ok( value.toString() === "there", 'Buffer set/get');
			test.done();
		},
		
		function testBufferKey(test) {
			var hash = new MegaCache();
			var keyBuf = Buffer.from("hello");
			hash.set(keyBuf, "there");
			var value = hash.get(keyBuf);
			test.ok(value === "there", 'Correct value with buffer key');
			test.done();
		},
		
		function testZeroLengthBuffer(test) {
			var hash = new MegaCache();
			hash.set("hello", Buffer.alloc(0));
			var value = hash.get("hello");
			test.ok( Buffer.isBuffer(value), "Value is buffer" );
			test.ok( value.length === 0, 'Buffer has zero length');
			test.done();
		},
		
		function testNumbers(test) {
			var hash = new MegaCache();
			
			hash.set("hello", 1);
			test.ok(hash.get("hello") === 1, 'Positive int set/get');
			
			hash.set("hello", 0);
			test.ok(hash.get("hello") === 0, 'Zero set/get');
			
			hash.set("hello", -1);
			test.ok(hash.get("hello") === -1, 'Negative int set/get');
			
			hash.set("hello", 42.8);
			test.ok(hash.get("hello") === 42.8, 'Positive float set/get');
			
			hash.set("hello", -9999.55);
			test.ok(hash.get("hello") === -9999.55, 'Negative float set/get');
			
			test.done();
		},
		
		function testNumberKeys(test) {
			var hash = new MegaCache();
			
			hash.set( 4, 772 );
			hash.set( 5, 9.99999999 );
			hash.set( 6, -0.00000001 );
			
			test.ok(hash.get(4) === 772, 'Number key set/get 1');
			test.ok(hash.get(5) === 9.99999999, 'Number key set/get 2');
			test.ok(hash.get(6) === -0.00000001, 'Number key set/get 3');
			
			test.done();
		},
		
		function testBigInts(test) {
			var hash = new MegaCache();
			
			hash.set( "big", 9007199254740993n );
			test.ok( hash.get("big") === 9007199254740993n, "Positive BigInt came through unscathed" );
			
			hash.set( "big2", -9007199254740992n );
			test.ok( hash.get("big2") === -9007199254740992n, "Negative BigInt came through unscathed" );
			
			test.done();
		},
		
		function testNull(test) {
			var hash = new MegaCache();
			hash.set("nope", null);
			test.ok( hash.get("nope") === null, "Null is null" );
			test.done();
		},
		
		function testBooleans(test) {
			var hash = new MegaCache();
			hash.set("yes", true);
			hash.set("no", false);
			var yes = hash.get("yes");
			test.ok(yes === true, 'Boolean true');
			var no = hash.get("no");
			test.ok(no === false, 'Boolean false');
			test.done();
		},
		
		function testObjects(test) {
			var hash = new MegaCache();
			hash.set("obj", { hello: "there", num: 82 });
			var value = hash.get("obj");
			test.ok( !!value, "Got object value" );
			test.ok( typeof(value) == 'object', "Value has correct type" );
			test.ok( value.hello === "there", "String inside object is correct" );
			test.ok( value.num === 82, "Number inside object is correct" );
			test.done();
		},
		
		function testManyKeys(test) {
			var hash = new MegaCache();
			for (var idx = 0; idx < 1000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			for (var idx = 0; idx < 1000; idx++) {
				var value = hash.get( "key" + idx );
				test.ok( value === "value here " + idx );
			}
			test.ok( hash.get("noexist") === undefined, "Missing key" );
			test.done();
		},
		
		function testReplace(test) {
			var hash = new MegaCache();
			hash.set("hello", "there");
			hash.set("hello", "foobar");
			test.ok( hash.get("hello") === "foobar", "Replaced value is correct" );
			
			var stats = hash.stats();
			test.ok(stats.numKeys === 1, '1 key in stats');
			test.ok(stats.numIndexes === 1, '1 index in stats');
			test.ok(stats.dataSize === 11, '11 bytes in data store');
			test.ok(stats.indexSize > 0, 'Index size is non-zero');
			test.ok(stats.metaSize > 0, 'Meta size is non-zero');
			test.done();
		},
		
		function testSetReturnValue(test) {
			// make sure set() returns the expected return values
			var hash = new MegaCache();
			test.ok( hash.set("hello", "there") == 1, "Unique key returns 1 on set" );
			test.ok( hash.set("hello", "there") == 2, "Replaced key returns 2 on set" );
			test.done();
		},
		
		function testRemove(test) {
			var hash = new MegaCache();
			hash.set("key1", "value1");
			hash.set("key2", "value2");
			
			test.ok( !!hash.remove("key1"), "remove returned true for good key" );
			test.ok( !hash.remove("key3"), "remove returned false for missing key" );
			
			test.ok( !hash.has("key1"), "Key1 was removed" );
			test.ok( hash.has("key2"), "Key2 is still there" );
			
			var stats = hash.stats();
			test.ok(stats.numKeys === 1, '1 key in stats');
			test.ok(stats.dataSize === 10, '10 bytes in data store');
			test.ok(stats.numIndexes === 1, '1 index in stats');
			
			test.done();
		},
		
		function testRemoveMany(test) {
			var hash = new MegaCache();
			for (var idx = 0; idx < 1000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			for (var idx = 0; idx < 1000; idx += 2) {
				// remove even keys, leave odd ones
				hash.remove( "key" + idx );
			}
			for (var idx = 0; idx < 1000; idx++) {
				var result = hash.has( "key" + idx );
				// test.debug( '' + idx + ": " + result + "\n" );
				test.ok( (idx % 2 == 0) ? !result : result, "Odd keys should exist, even keys should not" );
			}
			
			var stats = hash.stats();
			test.ok( stats.numKeys == 500, "500 keys remain" );
			
			test.done();
		},
		
		function testClear(test) {
			var hash = new MegaCache();
			hash.set("key1", "value1");
			hash.set("key2", "value2");
			hash.clear();
			
			var stats = hash.stats();
			test.ok(stats.numKeys === 0, '0 keys in stats: ' + stats.numKeys);
			test.ok(stats.dataSize === 0, '0 bytes in data store: ' + stats.dataSize);
			test.ok(stats.metaSize === 0, '0 bytes in meta store: ' + stats.metaSize);
			test.ok(stats.numIndexes === 1, '1 index in stats');
			
			test.done();
		},
		
		function testClearMany(test) {
			var hash = new MegaCache();
			for (var idx = 0; idx < 1000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			hash.clear();
			
			var stats = hash.stats();
			test.ok(stats.numKeys === 0, '0 keys in stats');
			test.ok(stats.dataSize === 0, '0 bytes in data store');
			test.ok(stats.numIndexes === 1, '1 index in stats');
			
			test.done();
		},
		
		function testClearSlices(test) {
			var hash = new MegaCache();
			for (var idx = 0; idx < 1000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			
			var lastNumKeys = hash.stats().numKeys;
			for (var idx = 0; idx < 256; idx++) {
				hash.clear(idx);
				var numKeys = hash.stats().numKeys;
				test.ok( numKeys <= lastNumKeys, "numKeys is dropping" );
				lastNumKeys = numKeys;
			}
			
			var stats = hash.stats();
			test.ok(stats.numKeys === 0, '0 keys in stats: ' + stats.numKeys);
			test.ok(stats.dataSize === 0, '0 bytes in data store: ' + stats.dataSize);
			test.ok(stats.numIndexes === 1, '1 index in stats: ' + stats.numIndexes);
			
			test.done();
		},
		
		function testKeyIteration(test) {
			var hash = new MegaCache();
			hash.set("key1", "value1");
			hash.set("key2", "value2");
			
			var key = hash.nextKey();
			test.ok( !!key, "Got key from nextKey" );
			test.ok( key.match(/^(key1|key2)$/), "Key is one or the other (" + key + ")" );
			
			key = hash.nextKey(key);
			test.ok( !!key, "Got another key from nextKey" );
			test.ok( key.match(/^(key1|key2)$/), "Key is one or the other (" + key + ")" );
			
			key = hash.nextKey(key);
			test.ok( !key, "Nothing after 2nd key, end of list" );
			
			test.done();
		},
		
		function testKeyIteration1000(test) {
			var hash = new MegaCache();
			for (var idx = 0; idx < 1000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			
			var key = hash.nextKey();
			var idx = 0;
			var allKeys = new Map();
			
			while (key) {
				test.ok( !allKeys.has(key), "Key is unique" );
				allKeys.set( key, true );
				idx++;
				key = hash.nextKey(key);
			}
			
			test.ok( idx == 1000, "Iterated exactly 1000 times: " + idx );
			test.done();
		},
		
		function testKeyIterationRemoval500(test) {
			var hash = new MegaCache();
			for (var idx = 0; idx < 1000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			for (var idx = 0; idx < 1000; idx += 2) {
				// remove even keys, leave odd ones
				hash.remove( "key" + idx );
			}
			
			var key = hash.nextKey();
			var idx = 0;
			var allKeys = new Map();
			
			while (key) {
				test.ok( key.match(/^key\d*[13579]$/), "Key is odd: " + key );
				test.ok( !allKeys.has(key), "Key is unique" );
				allKeys.set( key, true );
				idx++;
				key = hash.nextKey(key);
				test.debug("KEY: '" + key + "'");
			}
			
			test.ok( idx == 500, "Iterated exactly 500 times: " + idx );
			test.done();
		},
		
		function testKeyIteration10000(test) {
			var hash = new MegaCache();
			for (var idx = 0; idx < 10000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			
			var key = hash.nextKey();
			var idx = 0;
			var allKeys = new Map();
			
			while (key) {
				test.ok( !allKeys.has(key), "Key is unique" );
				allKeys.set( key, true );
				idx++;
				key = hash.nextKey(key);
			}
			
			test.ok( idx == 10000, "Iterated exactly 10000 times: " + idx );
			test.done();
		},
		
		function testKeyIterationRemoval5000(test) {
			var hash = new MegaCache();
			for (var idx = 0; idx < 10000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			for (var idx = 0; idx < 10000; idx += 2) {
				// remove even keys, leave odd ones
				hash.remove( "key" + idx );
			}
			
			var key = hash.nextKey();
			var idx = 0;
			var allKeys = new Map();
			
			while (key) {
				test.ok( key.match(/^key\d*[13579]$/), "Key is odd: " + key );
				test.ok( !allKeys.has(key), "Key is unique" );
				allKeys.set( key, true );
				idx++;
				key = hash.nextKey(key);
			}
			
			test.ok( idx == 5000, "Iterated exactly 5000 times: " + idx );
			test.done();
		},
		
		function testKeyIterationEmpty(test) {
			var hash = new MegaCache();
			
			var key = hash.nextKey();
			test.ok( !key, "Nothing in empty hash" );
			
			test.done();
		},
		
		function testKeyIterationBad(test) {
			var hash = new MegaCache();
			hash.set("key1", "value1");
			hash.set("key2", "value2");
			
			var key = hash.nextKey("key3");
			test.ok( !key, "nextKey found nothing with missing key" );
			
			test.done();
		},
		
		function testReindex(test) {
			// add enough keys so we trigger at least one reindex
			var hash = new MegaCache();
			for (var idx = 0; idx < 10000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			for (var idx = 0; idx < 10000; idx++) {
				var value = hash.get("key" + idx);
				if (value !== "value here " + idx) {
					test.ok( false, "Key " + idx + " does not match spec after reindex: " + value );
				}
			}
			
			var stats = hash.stats();
			test.debug( JSON.stringify(stats) );
			test.ok(stats.numKeys === 10000, 'Correct keys in stats: ' + stats.numKeys);
			test.ok(stats.numIndexes > 1, 'More indexes in stats: ' + stats.numIndexes);
			test.done();
		},
		
		function testSimilarDigests(test) {
			// test two keys with similar computed digests
			var hash = new MegaCache();
			
			for (var idx = 0; idx < 100000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			
			var key1 = "4723640122";
			var key2 = "4723640123";
			
			hash.set(key1, "value1");
			hash.set(key2, "value2");
			
			test.ok( hash.get(key1) === "value1", "Key '" + key1 + "' does not equal expected value.");
			test.ok( hash.get(key2) === "value2", "Key '" + key2 + "' does not equal expected value.");
			test.done();
		},
		
		function testSimilarDigestsWithReplace(test) {
			// test two keys with similar computed digests, replacing one
			var hash = new MegaCache();
			
			for (var idx = 0; idx < 100000; idx++) {
				hash.set( "key" + idx, "value here " + idx );
			}
			
			var key1 = "4723640122";
			var key2 = "4723640123";
			
			hash.set(key1, "value1");
			hash.set(key2, "value2");
			hash.set(key1, "value1_REPLACED"); // replace first value after setting second
			
			test.ok( hash.get(key1) === "value1_REPLACED", "Key '" + key1 + "' does not equal expected value.");
			test.ok( hash.get(key2) === "value2", "Key '" + key2 + "' does not equal expected value.");
			test.done();
		},
		
		function LRU_basic(test) {
			var cache = new MegaCache();
			cache.set( 'key1', 'value1' );
			var value = cache.get('key1');
			var stats = cache.stats();
			
			test.ok( value === "value1", "Value for key1 is not correct: " + value );
			test.ok( stats.numKeys == 1, "Cache has incorrect number of keys: " + stats.numKeys );
			test.ok( stats.dataSize == 10, "Cache has incorrect number of bytes: " + stats.dataSize );
			
			cache.set( 'key1', 'value12345' );
			value = cache.get('key1');
			stats = cache.stats();
			
			test.ok( value === "value12345", "Value for key1 is not correct after replace: " + value );
			test.ok( stats.numKeys == 1, "Cache has incorrect number of keys after replace: " + stats.numKeys );
			test.ok( stats.dataSize == 14, "Cache has incorrect number of bytes after replace: " + stats.dataSize );
			
			cache.delete( 'key1' );
			value = cache.get('key1');
			stats = cache.stats();
			
			test.ok( !value, "Value for key1 is not correct after delete: " + value );
			test.ok( stats.numKeys == 0, "Cache has incorrect number of keys after delete: " + stats.numKeys );
			test.ok( stats.dataSize == 0, "Cache has incorrect number of bytes after delete: " + stats.dataSize );
			
			cache.set( 'key1', 'value12345' );
			cache.clear();
			value = cache.get('key1');
			stats = cache.stats();
			
			test.ok( !value, "Value for key1 is not correct after clear: " + value );
			test.ok( stats.numKeys == 0, "Cache has incorrect number of keys after clear: " + stats.numKeys );
			test.ok( stats.dataSize == 0, "Cache has incorrect number of bytes after clear: " + stats.dataSize );
			
			// try some other data types
			cache.set( 'key1', 12345 );
			value = cache.get('key1');
			test.ok( value === 12345, "Number value is incorrect: " + value );
			
			cache.set( 'key1z', 0 );
			value = cache.get('key1z');
			test.ok( value === 0, "Number value is incorrect: " + value );
			
			cache.set( 'key2', true );
			value = cache.get('key2');
			test.ok( value === true, "Boolean value is incorrect: " + value );
			
			cache.set( 'key2f', false );
			value = cache.get('key2f');
			test.ok( value === false, "Boolean value is incorrect: " + value );
			
			cache.set( 'key3', Buffer.alloc(10) );
			value = cache.get('key3');
			test.ok( !!value.fill, "Value is not a buffer: " + value );
			test.ok( value.length == 10, "Buffer length is incorrect: " + value.length );
			
			// must pass length metadata for null value
			cache.set( 'key4', null );
			value = cache.get('key4');
			test.ok( value === null, "Null value is incorrect: " + value );
			
			cache.set( 'key6', { foo: 'bar' } );
			value = cache.get('key6');
			test.ok( typeof(value) == 'object', "Object value is incorrect: " + value );
			
			test.done();
		},
		
		function LRU_fillItems(test) {
			var idx, key, value, item;
			var cache = new MegaCache( 10 );
			
			for (idx = 1; idx <= 10; idx++) {
				cache.set( 'key' + idx, 'value' + idx );
			}
			
			for (idx = 1; idx <= 10; idx++) {
				value = cache.get( 'key' + idx );
				test.ok( value === 'value' + idx, "Cache key" + idx + " has incorrect value: " + value );
			}
			
			var stats = cache.stats();
			test.ok( stats.numKeys == 10, "Cache has incorrect key count: " + stats.numKeys );
			
			// walk the linked list forwards (descending popularity)
			item = cache.nextKey();
			for (idx = 10; idx >= 1; idx--) {
				key = 'key' + idx;
				value = 'value' + idx;
				test.ok( !!item, "Item is falsey at idx " + idx );
				test.ok( item === key, "Item key is incorrect: " + item + " != " + key );
				test.ok( cache.has(item), "Cache does not have item: " + item );
				item = cache.nextKey(item);
			}
			test.ok( !item, "Item is not false at end of list" );
			
			// walk the linked list backwards (ascending popularity)
			item = cache.prevKey();
			for (idx = 1; idx <= 10; idx++) {
				key = 'key' + idx;
				value = 'value' + idx;
				test.ok( !!item, "Item is falsey at rev idx " + idx );
				test.ok( item === key, "Item key is incorrect: " + item.key + " != " + key );
				test.ok( cache.has(item), "Cache does not have item: " + item );
				item = cache.prevKey(item);
			}
			test.ok( !item, "Item is not false at start of list" );
			
			test.done();
		},
		
		function LRU_overflowItems(test) {
			var idx, key, value, item;
			var cache = new MegaCache( 10 );
			
			// add one too many, causing overflow and expunge
			for (idx = 1; idx <= 11; idx++) {
				cache.set( 'key' + idx, 'value' + idx );
			}
			
			var stats = cache.stats();
			test.ok( stats.numKeys == 10, "numKeys incorrect after overflow: " + stats.numKeys );
			test.ok( stats.numEvictions == 1, "numEvictions incorrect after overflow: " + stats.numEvictions );
			
			for (idx = 2; idx <= 11; idx++) {
				value = cache.get( 'key' + idx );
				test.ok( value === 'value' + idx, "Cache key key" + idx + " has incorrect value: " + value );
			}
			
			value = cache.get( 'key1' );
			test.ok( !value, "Expected undefined, got actual value for key1: " + value );
			test.done();
		},
		
		function LRU_fillBytes(test) {
			// {"indexSize":129,"metaSize":320,"dataSize":150,"numKeys":10,"numIndexes":1,"numEvictions":0}
			var idx, key, value, item;
			var cache = new MegaCache( 0, 129 + 320 + 150 );
			
			for (idx = 11; idx <= 20; idx++) {
				cache.set( 'key' + idx, 'ABCDEFGHIJ' );
			}
			
			var stats = cache.stats();
			// test.debug("Stats: ", stats);
			test.ok( stats.numKeys == 10, "numKeys incorrect after fill bytes: " + stats.numKeys );
			test.ok( stats.numEvictions == 0, "numEvictions incorrect after fill bytes: " + stats.numEvictions );
			test.ok( stats.dataSize == 150, "dataSize incorrect after fill bytes: " + stats.dataSize );
			
			for (idx = 11; idx <= 20; idx++) {
				value = cache.get( 'key' + idx );
				test.ok( value === 'ABCDEFGHIJ', "Cache key" + idx + " has incorrect value: " + value );
			}
			
			test.done();
		},
		
		function LRU_overflowBytes(test) {
			var idx, key, value, item;
			var cache = new MegaCache( 0, 600 );
			
			for (idx = 11; idx <= 21; idx++) {
				cache.set( 'key' + idx, 'ABCDEFGHIJ' );
			}
			
			var stats = cache.stats();
			// test.debug("Stats: ", stats);
			test.ok( stats.numKeys == 10, "numKeys incorrect after overflow bytes: " + stats.numKeys );
			test.ok( stats.numEvictions == 1, "numEvictions incorrect after overflow bytes: " + stats.numEvictions );
			test.ok( stats.dataSize == 150, "dataSize incorrect after fill bytes: " + stats.dataSize );
			
			for (idx = 12; idx <= 21; idx++) {
				value = cache.get( 'key' + idx );
				test.ok( value === 'ABCDEFGHIJ', "Cache key key" + idx + " has incorrect value: " + value );
			}
			
			value = cache.get( 'key11' );
			test.ok( !value, "Expected undefined, got actual value for key11: " + value );
			test.done();
		},
		
		function LRU_overflowBytesMultiple(test) {
			var idx, key, value, item;
			var cache = new MegaCache( 0, 600 );
			
			for (idx = 11; idx <= 20; idx++) {
				cache.set( 'key' + idx, 'ABCDEFGHIJ' );
			}
			for (idx = 11; idx <= 20; idx++) {
				value = cache.get( 'key' + idx );
				test.ok( value === 'ABCDEFGHIJ', "Cache key key" + idx + " has incorrect value: " + value );
			}
			
			var stats = cache.stats();
			// test.debug("Stats: ", stats);
			test.ok( stats.numKeys == 10, "numKeys incorrect: " + stats.numKeys );
			test.ok( stats.numEvictions == 0, "numEvictions incorrect: " + stats.numEvictions );
			test.ok( stats.dataSize == 150, "dataSize incorrect: " + stats.dataSize );
			
			// cause everything to be expunged at once and replaced with boom
			// (292 byte buf + `boom` key == 300 bytes exactly)
			var buf = Buffer.alloc( 435 );
			cache.set( 'boom', buf );
			
			value = cache.get('boom');
			test.ok( !!value, "Unable to fetch boom");
			test.ok( value.length == 435, "Boom has incorrect length: " + value.length );
			
			stats = cache.stats();
			// test.debug("Stats: ", stats);
			
			test.ok( stats.numKeys == 1, "Cache has incorrect count after boom: " + stats.numKeys );
			test.ok( stats.dataSize == 439, "Cache has incorrect dataSize after boom: " + stats.dataSize );
			test.ok( stats.numEvictions == 10, "numEvictions incorrect after boom: " + stats.numEvictions );
			
			// internal API checks
			var first_key = cache.nextKey();
			test.ok( first_key === "boom", "First list item is not boom: " + first_key );
			
			var last_key = cache.prevKey();
			test.ok( last_key === "boom", "Last list item is not boom: " + last_key );
			
			// now cause an implosion (cannot store > 600 bytes, will immediately be expunged)
			var buf2 = Buffer.alloc( 700 );
			cache.set( 'implode', buf2 );
			
			value = cache.get('implode');
			test.ok( !value, "Oops, implode should not be fetchable!  But it was!");
			
			stats = cache.stats();
			// test.debug("Stats: ", stats);
			
			test.ok( stats.numKeys == 0, "Cache has incorrect count after implosion: " + stats.numKeys );
			test.ok( stats.dataSize == 0, "Cache has incorrect dataSize after implosion: " + stats.dataSize );
			test.ok( stats.numEvictions == 12, "numEvictions incorrect after implosion: " + stats.numEvictions );
			
			test.done();
		},
		
		function LRU_promote(test) {
			var idx, key, value, item;
			var cache = new MegaCache();
			
			for (idx = 1; idx <= 10; idx++) {
				cache.set( 'key' + idx, 'value' + idx );
			}
			
			// head list item should be last key inserted
			key = cache.nextKey();
			test.ok( key === "key10", "First item in list has incorrect key: " + key );
			
			key = cache.nextKey(key);
			test.ok( key === "key9", "Second item in list has incorrect key: " + key );
			
			// promoting head key should have no effect
			cache.get( "key10" );
			
			key = cache.nextKey();
			test.ok( key === "key10", "First item in list has incorrect key: " + key );
			
			key = cache.nextKey(key);
			test.ok( key === "key9", "Second item in list has incorrect key: " + key );
			
			// promote key5 to head
			cache.get( "key5" );
			
			key = cache.nextKey();
			test.ok( key === "key5", "First item in list has incorrect key after promotion: " + key );
			
			key = cache.nextKey(key);
			test.ok( key === "key10", "Second item in list has incorrect key after promotion: " + key );
			
			key = cache.nextKey(key);
			test.ok( key === "key9", "Third item in list has incorrect key after promotion: " + key );
			
			// now go backwards!
			key = cache.prevKey(key);
			test.ok( key === "key10", "Reverse linking is incorrect for promoted head key5" );
			
			// and again!
			key = cache.prevKey(key);
			test.ok( key === "key5", "Reverse linking is incorrect for promoted head key5" );
			
			// make sure end of list is expected
			key = cache.prevKey();
			test.ok( key === "key1", "Last item in list is unexpected: " + key );
			
			// promote last key to head
			cache.get( "key1" );
			
			key = cache.nextKey();
			test.ok( key === "key1", "First item in list has incorrect key after promotion: " + key );
			
			key = cache.nextKey(key);
			test.ok( key === "key5", "Second item in list has incorrect key after promotion: " + key );
			
			key = cache.nextKey(key);
			test.ok( key === "key10", "Third item in list has incorrect key after promotion: " + key );
			
			// make sure end of list is expected
			key = cache.prevKey();
			test.ok(key === "key2", "Last item in list is unexpected: " + key );
			
			test.done();
		},
		
		function LRU_peek(test) {
			var idx, key, value, item;
			var cache = new MegaCache();
			
			for (idx = 1; idx <= 10; idx++) {
				cache.set( 'key' + idx, 'value' + idx );
			}
			
			// first key should be the latest one entered
			var key = cache.nextKey();
			test.ok( key === "key10", "First key is not the expected key10: " + key );
			
			// peek a different key, make sure we get the value
			var value = cache.peek('key5');
			test.ok( value === "value5", "key5 does not have the expected value5: " + value );
			
			// first key should be unchanged
			var key = cache.nextKey();
			test.ok( key === "key10", "First key is not the expected key10 after peek: " + key );
			
			test.done();
		},
		
		function LRU_keepAlive(test) {
			var idx, idy, key, value, item;
			var cache = new MegaCache( 10 );
			
			cache.set( 'special', "SPECIAL" );
			
			// flood cache with totally random keys, but make sure to fetch special key between batches
			for (idx = 0; idx < 100; idx++) {
				for (var idy = 0; idy < 9; idy++) {
					cache.set( 'random_' + idx + '_' + idy + '_' + Math.random(), 'RANDOM' + Math.random() );
				}
				cache.get( 'special' );
			}
			
			value = cache.get( 'special' );
			test.ok( value === "SPECIAL", "Special key has disappeared unexpectedly: " + value );
			
			// flood cache with semi-random keys (will replace each other)
			for (idx = 0; idx < 100; idx++) {
				for (var idy = 0; idy < 9; idy++) {
					cache.set( 'random_' + Math.floor(Math.random() * 9), 'RANDOM' + Math.random() );
				}
				cache.get( 'special' );
			}
			
			value = cache.get( 'special' );
			test.ok( value === "SPECIAL", "Special key has disappeared unexpectedly: " + value );
			
			// flood cache with too many keys per inner loop, should expunge special key
			for (idx = 0; idx < 100; idx++) {
				for (var idy = 0; idy < 10; idy++) {
					cache.set( 'random_' + idx + '_' + idy + '_' + Math.random(), 'RANDOM' + Math.random() );
				}
				cache.get( 'special' );
			}
			
			value = cache.get( 'special' );
			test.ok( !value, "Special key shuld be expunged, but is still here: " + value );
			
			test.done();
		}
		
	]
};
