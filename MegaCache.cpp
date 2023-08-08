// MegaCache v1.0
// Copyright (c) 2023 Joseph Huckaby

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "MegaCache.h"

Response Hash::store(unsigned char *key, MH_KLEN_T keyLength, unsigned char *content, MH_LEN_T contentLength, unsigned char flags) {
	// store key/value pair in hash, promote to LRU head, expunge old if needed
	unsigned char digest[MH_DIGEST_SIZE];
	Response resp;
	
	// first digest key
	digestKey(key, keyLength, digest);
	
	// combine key and content together, with length prefixes, into single blob
	// this reduces malloc bashing and memory frag
	MH_LEN_T payloadSize = sizeof(Bucket) + MH_KLEN_SIZE + keyLength + MH_LEN_SIZE + contentLength;
	MH_LEN_T offset = sizeof(Bucket);
	unsigned char *payload = (unsigned char *)malloc(payloadSize);
	
	// check for malloc error here
	if (!payload) {
		resp.result = MH_ERR;
		return resp;
	}
	
	memcpy( (void *)&payload[offset], (void *)&keyLength, MH_KLEN_SIZE ); offset += MH_KLEN_SIZE;
	memcpy( (void *)&payload[offset], (void *)key, keyLength ); offset += keyLength;
	memcpy( (void *)&payload[offset], (void *)&contentLength, MH_LEN_SIZE ); offset += MH_LEN_SIZE;
	memcpy( (void *)&payload[offset], (void *)content, contentLength ); offset += contentLength;
	
	unsigned char digestIndex = 0;
	unsigned char ch;
	unsigned char bucketIndex = 0;
	Tag *tag = (Tag *)index;
	Index *level, *newLevel;
	Bucket *bucket, *newBucket, *lastBucket;
	
	while (tag && (tag->type == MH_SIG_INDEX)) {
		level = (Index *)tag;
		ch = digest[digestIndex];
		tag = level->data[ch];
		if (!tag) {
			// create new bucket list here
			bucket = (Bucket *)payload;
			bucket->init();
			bucket->flags = flags;
			level->data[ch] = (Tag *)bucket;
			
			// add new bucket as new LRU head
			bucket->cachePrev = NULL;
			bucket->cacheNext = cacheFirst;
			if (cacheFirst) cacheFirst->cachePrev = bucket;
			cacheFirst = bucket;
			if (!cacheLast) cacheLast = bucket;
			// end LRU section
			
			resp.result = MH_ADD;
			stats->dataSize += keyLength + contentLength;
			stats->metaSize += sizeof(Bucket) + MH_KLEN_SIZE + MH_LEN_SIZE;
			stats->numKeys++;
			tag = NULL; // break
		}
		else if (tag->type == MH_SIG_BUCKET) {
			// found bucket list, append
			bucket = (Bucket *)tag;
			lastBucket = NULL;
			
			while (bucket) {
				if (bucketKeyEquals(bucket, key, keyLength)) {
					// replace
					newBucket = (Bucket *)payload;
					newBucket->init();
					newBucket->flags = flags;
					newBucket->next = bucket->next;
					
					// manage LRU linked list
					if (bucket->cachePrev) bucket->cachePrev->cacheNext = bucket->cacheNext;
					if (bucket->cacheNext) bucket->cacheNext->cachePrev = bucket->cachePrev;
					if (bucket == cacheFirst) cacheFirst = bucket->cacheNext;
					if (bucket == cacheLast) cacheLast = bucket->cachePrev;
					
					newBucket->cachePrev = NULL;
					newBucket->cacheNext = cacheFirst;
					if (cacheFirst) cacheFirst->cachePrev = newBucket;
					cacheFirst = newBucket;
					if (!cacheLast) cacheLast = newBucket;
					// end LRU section
					
					if (lastBucket) lastBucket->next = newBucket;
					else level->data[ch] = (Tag *)newBucket;
					
					resp.result = MH_REPLACE;
					stats->dataSize -= (bucketGetKeyLength(bucket) + bucketGetContentLength(bucket));
					stats->dataSize += keyLength + contentLength;
					
					free((void *)bucket);
					bucket = NULL; // break
				}
				else if (!bucket->next) {
					// append here
					newBucket = (Bucket *)payload;
					newBucket->init();
					newBucket->flags = flags;
					bucket->next = newBucket;
					resp.result = MH_ADD;
					
					// add new bucket as new LRU head
					newBucket->cachePrev = NULL;
					newBucket->cacheNext = cacheFirst;
					if (cacheFirst) cacheFirst->cachePrev = newBucket;
					cacheFirst = newBucket;
					if (!cacheLast) cacheLast = newBucket;
					// end LRU section
					
					stats->dataSize += keyLength + contentLength;
					stats->metaSize += sizeof(Bucket) + MH_KLEN_SIZE + MH_LEN_SIZE;
					stats->numKeys++;
					bucket = NULL; // break
					
					// possibly reindex here
					if ((bucketIndex >= maxBuckets + (ch % reindexScatter)) && (digestIndex < MH_DIGEST_SIZE - 1)) {
						// deeper we go
						digestIndex++;
						newLevel = new Index();
						
						// check for malloc error here
						if (!newLevel) {
							resp.result = MH_ERR;
							return resp;
						}
						
						stats->indexSize += sizeof(Index);
						
						bucket = (Bucket *)tag;
						level->data[ch] = (Tag *)newLevel;
						
						while (bucket) {
							lastBucket = bucket;
							bucket = bucket->next;
							reindexBucket(lastBucket, newLevel, digestIndex);
						}
					} // reindex
				}
				else {
					lastBucket = bucket;
					bucket = bucket->next;
					bucketIndex++;
				}
			} // while bucket
			
			tag = NULL; // break
		}
		else {
			digestIndex++;
		}
	} // while tag
	
	// LRU space management
	while ((maxKeys && (stats->numKeys > maxKeys)) || (maxBytes && (stats->dataSize + stats->indexSize + stats->metaSize > maxBytes))) {
		remove( bucketGetKey(cacheLast), bucketGetKeyLength(cacheLast) );
		stats->numEvictions++;
	}
	
	return resp;
}

void Hash::reindexBucket(Bucket *bucket, Index *index, unsigned char digestIndex) {
	// reindex existing bucket into new subindex level
	unsigned char digest[MH_DIGEST_SIZE];
	MH_KLEN_T keyLength = bucketGetKeyLength(bucket);
	unsigned char *key = bucketGetKey(bucket);
	digestKey(key, keyLength, digest);
	unsigned char ch = digest[digestIndex];
	
	Tag *tag = index->data[ch];
	if (!tag) {
		// create new bucket list here
		index->data[ch] = (Tag *)bucket;
		bucket->next = NULL;
	}
	else {
		// traverse list, append to end
		Bucket *current = (Bucket *)tag;
		while (current->next) {
			current = current->next;
		}
		current->next = bucket;
		bucket->next = NULL;
	}
}

Response Hash::fetch(unsigned char *key, MH_KLEN_T keyLength) {
	// fetch value given key, LRU promote to head
	unsigned char digest[MH_DIGEST_SIZE];
	Response resp;
	
	// first digest key
	digestKey(key, keyLength, digest);
	
	unsigned char digestIndex = 0;
	unsigned char ch;
	
	Tag *tag = (Tag *)index;
	Index *level;
	Bucket *bucket;
	
	unsigned char *bucketData;
	unsigned char *tempCL;
	
	while (tag && (tag->type == MH_SIG_INDEX)) {
		level = (Index *)tag;
		ch = digest[digestIndex];
		tag = level->data[ch];
		if (!tag) {
			// not found
			resp.result = MH_ERR;
			tag = NULL; // break
		}
		else if (tag->type == MH_SIG_BUCKET) {
			// found bucket list, append
			bucket = (Bucket *)tag;
			
			while (bucket) {
				if (bucketKeyEquals(bucket, key, keyLength)) {
					// found!
					bucketData = ((unsigned char *)bucket) + sizeof(Bucket);
					tempCL = bucketData + MH_KLEN_SIZE + keyLength;
					
					resp.result = MH_OK;
					resp.contentLength = ((MH_LEN_T *)tempCL)[0];
					resp.content = bucketData + MH_KLEN_SIZE + keyLength + MH_LEN_SIZE;
					resp.flags = bucket->flags;
					resp.bucket = bucket;
					
					// LRU promote to head
					if (bucket != cacheFirst) {
						if (bucket->cachePrev) bucket->cachePrev->cacheNext = bucket->cacheNext;
						if (bucket->cacheNext) bucket->cacheNext->cachePrev = bucket->cachePrev;
						if (bucket == cacheFirst) cacheFirst = bucket->cacheNext;
						if (bucket == cacheLast) cacheLast = bucket->cachePrev;
						
						bucket->cachePrev = NULL;
						bucket->cacheNext = cacheFirst;
						if (cacheFirst) cacheFirst->cachePrev = bucket;
						cacheFirst = bucket;
						if (!cacheLast) cacheLast = bucket;
					}
					// end LRU section
					
					bucket = NULL; // break
				}
				else if (!bucket->next) {
					// not found
					resp.result = MH_ERR;
					bucket = NULL; // break
				}
				else {
					bucket = bucket->next;
				}
			} // while bucket
			
			tag = NULL; // break
		}
		else {
			digestIndex++;
		}
	} // while tag
	
	return resp;
}

Response Hash::peek(unsigned char *key, MH_KLEN_T keyLength) {
	// fetch value given key, without LRU promotion
	unsigned char digest[MH_DIGEST_SIZE];
	Response resp;
	
	// first digest key
	digestKey(key, keyLength, digest);
	
	unsigned char digestIndex = 0;
	unsigned char ch;
	
	Tag *tag = (Tag *)index;
	Index *level;
	Bucket *bucket;
	
	unsigned char *bucketData;
	unsigned char *tempCL;
	
	while (tag && (tag->type == MH_SIG_INDEX)) {
		level = (Index *)tag;
		ch = digest[digestIndex];
		tag = level->data[ch];
		if (!tag) {
			// not found
			resp.result = MH_ERR;
			tag = NULL; // break
		}
		else if (tag->type == MH_SIG_BUCKET) {
			// found bucket list, append
			bucket = (Bucket *)tag;
			
			while (bucket) {
				if (bucketKeyEquals(bucket, key, keyLength)) {
					// found!
					bucketData = ((unsigned char *)bucket) + sizeof(Bucket);
					tempCL = bucketData + MH_KLEN_SIZE + keyLength;
					
					resp.result = MH_OK;
					resp.contentLength = ((MH_LEN_T *)tempCL)[0];
					resp.content = bucketData + MH_KLEN_SIZE + keyLength + MH_LEN_SIZE;
					resp.flags = bucket->flags;
					resp.bucket = bucket;
					
					bucket = NULL; // break
				}
				else if (!bucket->next) {
					// not found
					resp.result = MH_ERR;
					bucket = NULL; // break
				}
				else {
					bucket = bucket->next;
				}
			} // while bucket
			
			tag = NULL; // break
		}
		else {
			digestIndex++;
		}
	} // while tag
	
	return resp;
}

Response Hash::remove(unsigned char *key, MH_KLEN_T keyLength) {
	// remove bucket given key
	unsigned char digest[MH_DIGEST_SIZE];
	Response resp;
	
	// first digest key
	digestKey(key, keyLength, digest);
	
	unsigned char digestIndex = 0;
	unsigned char ch;
	
	Tag *tag = (Tag *)index;
	Index *level;
	Bucket *bucket, *lastBucket;
	
	while (tag && (tag->type == MH_SIG_INDEX)) {
		level = (Index *)tag;
		ch = digest[digestIndex];
		tag = level->data[ch];
		if (!tag) {
			// not found
			resp.result = MH_ERR;
			tag = NULL; // break
		}
		else if (tag->type == MH_SIG_BUCKET) {
			// found bucket list, traverse
			bucket = (Bucket *)tag;
			lastBucket = NULL;
			
			while (bucket) {
				if (bucketKeyEquals(bucket, key, keyLength)) {
					// found!
					stats->dataSize -= (bucketGetKeyLength(bucket) + bucketGetContentLength(bucket));
					stats->metaSize -= (sizeof(Bucket) + MH_KLEN_SIZE + MH_LEN_SIZE);
					stats->numKeys--;
					
					if (lastBucket) lastBucket->next = bucket->next;
					else level->data[ch] = bucket->next;
					
					// LRU remove from linked list
					if (bucket->cachePrev) bucket->cachePrev->cacheNext = bucket->cacheNext;
					if (bucket->cacheNext) bucket->cacheNext->cachePrev = bucket->cachePrev;
					if (bucket == cacheFirst) cacheFirst = bucket->cacheNext;
					if (bucket == cacheLast) cacheLast = bucket->cachePrev;
					// end LRU section
					
					resp.result = MH_OK;
					free((void *)bucket);
					bucket = NULL; // break
				}
				else if (!bucket->next) {
					// not found
					resp.result = MH_ERR;
					bucket = NULL; // break
				}
				else {
					lastBucket = bucket;
					bucket = bucket->next;
				}
			} // while bucket
			
			tag = NULL; // break
		}
		else {
			digestIndex++;
		}
	} // while tag
	
	return resp;
}

void Hash::clear() {
	// clear ALL keys/values
	for (int idx = 0; idx < MH_INDEX_SIZE; idx++) {
		if (index->data[idx]) {
			clearTag( index->data[idx] );
			index->data[idx] = NULL;
		}
	}
	
	cacheFirst = NULL;
	cacheLast = NULL;
}

void Hash::clear(unsigned char slice) {
	// clear one "thick slice" from main index (about 1/256 of total keys)
	// this is so you can split up the job into pieces and not hang the CPU for too long
	unsigned char slice1 = slice / 16;
	unsigned char slice2 = slice % 16;
	
	// since our index system is 4-bit, we split the uchar into two pieces
	// and traverse into a nested index for the 2nd piece, if applicable
	if (index->data[slice1]) {
		Tag *tag = index->data[slice1];
		if (tag->type == MH_SIG_INDEX) {
			// nested index, use idx2
			Index *level = (Index *)tag;
			if (level->data[slice2]) {
				clearTag( level->data[slice2] );
				level->data[slice2] = NULL;
			}
			
			// also clear top-level index if it is now empty
			int empty = 1;
			for (int idx = 0; idx < MH_INDEX_SIZE; idx++) {
				if (level->data[idx]) { empty = 0; idx = MH_INDEX_SIZE; }
			}
			if (empty) {
				clearTag( tag );
				index->data[slice1] = NULL;
			}
		}
		else if (tag->type == MH_SIG_BUCKET) {
			clearTag( tag );
			index->data[slice1] = NULL;
		}
	}
}

void Hash::clear(unsigned char char1, unsigned char char2) {
	// clear one "thin slice" from main index (about 1/65536 of total keys)
	// this is so you can split up the job into pieces and not hang the CPU for too long
	unsigned char slices[4];
	slices[0] = char1 / 16;
	slices[1] = char1 % 16;
	slices[2] = char2 / 16;
	slices[3] = char2 % 16;
	
	clearSlice( index, slices, 0 );
}

void Hash::clearSlice(Index *level, unsigned char *slices, unsigned char idx) {
	// traverse one 4-bit slice and traverse if we have more slices to go
	unsigned char slice = slices[idx];
	
	if (level->data[slice]) {
		Tag *tag = level->data[slice];
		if (tag->type == MH_SIG_INDEX) {
			if (idx < 3) clearSlice( (Index *)tag, slices, idx + 1 );
			else {
				clearTag( tag );
				level->data[slice] = NULL;
			}
		}
		else if (tag->type == MH_SIG_BUCKET) {
			clearTag( tag );
			level->data[slice] = NULL;
		}
	}
}

void Hash::clearTag(Tag *tag) {
	// internal method: clear one tag (index or bucket)
	// traverse lists, recurse for nested indexes
	if (tag->type == MH_SIG_INDEX) {
		// traverse index
		Index *level = (Index *)tag;
		
		for (int idx = 0; idx < MH_INDEX_SIZE; idx++) {
			if (level->data[idx]) {
				clearTag( level->data[idx] );
				level->data[idx] = NULL;
			}
		}
		
		// kill index
		delete level;
		stats->indexSize -= sizeof(Index);
	}
	else if (tag->type == MH_SIG_BUCKET) {
		// delete all buckets in list
		Bucket *bucket = (Bucket *)tag;
		Bucket *lastBucket;
		
		while (bucket) {
			lastBucket = bucket;
			bucket = bucket->next;
			
			stats->dataSize -= (bucketGetKeyLength(lastBucket) + bucketGetContentLength(lastBucket));
			stats->metaSize -= (sizeof(Bucket) + MH_KLEN_SIZE + MH_LEN_SIZE);
			stats->numKeys--;
			
			// LRU remove bucket from linked list
			if (lastBucket->cachePrev) lastBucket->cachePrev->cacheNext = lastBucket->cacheNext;
			if (lastBucket->cacheNext) lastBucket->cacheNext->cachePrev = lastBucket->cachePrev;
			if (lastBucket == cacheFirst) cacheFirst = lastBucket->cacheNext;
			if (lastBucket == cacheLast) cacheLast = lastBucket->cachePrev;
			// end LRU section
			
			free((void *)lastBucket);
		}
	}
}

Response Hash::firstKey() {
	// return first key (in descending popular order)
	Bucket *bucket = cacheFirst;
	Response resp;
	
	if (!bucket) return resp;
	
	resp.result = MH_OK;
	resp.content = bucketGetKey(bucket);
	resp.contentLength = bucketGetKeyLength(bucket);
	
	return resp;
}

Response Hash::nextKey(unsigned char *key, MH_KLEN_T keyLength) {
	// return next key given previous key (in descending popular order)
	Response resp = peek(key, keyLength);
	if (resp.result != MH_OK) return resp;
	
	Bucket *bucket = resp.bucket;
	if (!bucket || !bucket->cacheNext) {
		resp.result = MH_ERR;
		resp.content = NULL;
		resp.contentLength = 0;
		resp.flags = 0;
		resp.bucket = NULL;
		return resp;
	}
	
	bucket = bucket->cacheNext;
	
	resp.result = MH_OK;
	resp.content = bucketGetKey(bucket);
	resp.contentLength = bucketGetKeyLength(bucket);
	
	return resp;
}

Response Hash::lastKey() {
	// return last key (in ascending popular order)
	Bucket *bucket = cacheLast;
	Response resp;
	
	if (!bucket) return resp;
	
	resp.result = MH_OK;
	resp.content = bucketGetKey(bucket);
	resp.contentLength = bucketGetKeyLength(bucket);
	
	return resp;
}

Response Hash::prevKey(unsigned char *key, MH_KLEN_T keyLength) {
	// return previous key given any key (in ascending popular order)
	Response resp = peek(key, keyLength);
	if (resp.result != MH_OK) return resp;
	
	Bucket *bucket = resp.bucket;
	if (!bucket || !bucket->cachePrev) {
		resp.result = MH_ERR;
		resp.content = NULL;
		resp.contentLength = 0;
		resp.flags = 0;
		resp.bucket = NULL;
		return resp;
	}
	
	bucket = bucket->cachePrev;
	
	resp.result = MH_OK;
	resp.content = bucketGetKey(bucket);
	resp.contentLength = bucketGetKeyLength(bucket);
	
	return resp;
}
