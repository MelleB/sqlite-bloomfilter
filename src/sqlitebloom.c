//
// TODO: Insert LICENSE REF here
//

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "sqlite/sqlite3ext.h"
#include "murmur3/murmur3.h"
// #include "murmur3str.h"

SQLITE_EXTENSION_INIT1


// The number of uint64 blocks that should be used
// for the bloom filter.
#define SQLITE_BLOOM_DEFAULT_SIZE 8

// Determine which MurMurHash function to use based on the target architecture
// Depending on target architecture, this needs to be expanded
#ifdef __i386__
	#define MURMURHASH MurmurHash3_x86_128
#elif defined __x86_64__ || __X86__
	#define MURMURHASH MurmurHash3_x64_128
#else
	#error Architecture not defined.
#endif

// The seed for the murmur3 hash function
#define MURMUR3_SEED 42


//------------------------------------------------------------------------------
// Calculate the bitfield position in the bloomfilter
static uint64_t bloom_key2bf(const char * key)
{
	// The number of bits in the bloom filter
	uint32_t nbbf = 64 * SQLITE_BLOOM_DEFAULT_SIZE;

	uint64_t hash[2];
	MURMURHASH(key, strlen(key), MURMUR3_SEED, hash);

	// Calculate the modulus of the 128 bit integer using substeps
	uint64_t mid = (hash[1] >> 32);
	uint64_t tmp1 = mid + ((hash[0] % nbbf) << 32);
	uint64_t tmp2 = (hash[1] & UINT32_MAX) + ((tmp1 % nbbf) << 32);
	uint64_t mod = tmp2 % nbbf;

	return mod;
}


//------------------------------------------------------------------------------
// The bloom_step method is executed for each row in the target set
static void bloom_step(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	// Skip NULL values
	if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
		return;
	}

	uint64_t mod;
	const char *key;

	// The container for the bloom filter
	uint64_t *bf = sqlite3_aggregate_context(context, 
			sizeof(uint64_t) * SQLITE_BLOOM_DEFAULT_SIZE);

	key = (char *)sqlite3_value_text(argv[0]);
	mod = bloom_key2bf(key);

	// Note: Explicit cast to uint64 is required.
	bf[mod/64] |= (uint64_t)1 << (mod % 64);
}


//------------------------------------------------------------------------------
// Finally, the bloom filter is converted to a string representation
static void bloom_finalize(sqlite3_context *context)
{
	// The container for the bloom filter
	uint64_t *bf = sqlite3_aggregate_context(context, 
			sizeof(uint64_t) * SQLITE_BLOOM_DEFAULT_SIZE);

	char *out = (char *)malloc(16 * SQLITE_BLOOM_DEFAULT_SIZE + 1);
	if (out == 0) {
		sqlite3_result_error_nomem(context);
		return;
	}
	memset(out, '\0', 16 * SQLITE_BLOOM_DEFAULT_SIZE + 1);

	int i;
	for (i = 0; i < SQLITE_BLOOM_DEFAULT_SIZE; i++) {
		// Note, buckets are placed from left to right.
		sprintf(out + i*16, "%016lX", bf[i]);
	}

	sqlite3_result_text(context, out, 16 * SQLITE_BLOOM_DEFAULT_SIZE, free);
}


//------------------------------------------------------------------------------
// Check if a value is contained within a bloom filter
static void in_bloom(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	const char *key;
	const char *bf_txt;
	uint64_t bf[SQLITE_BLOOM_DEFAULT_SIZE] = {0};
	uint64_t mod;

	key = (char *)sqlite3_value_text(argv[0]);
	bf_txt = (char *)sqlite3_value_text(argv[1]);

	// Check if the length of the filter is correct
	size_t bf_len = strlen(bf_txt);
	const char *hexchars = "01234567890ABCDEF";
	if (bf_len != 16 * SQLITE_BLOOM_DEFAULT_SIZE) {
		sqlite3_result_error(context, "Bloomfilter of invalid size.", -1);
		return;
	} else if (strspn(bf_txt, hexchars) != bf_len) {
		sqlite3_result_error(context, 
				"Bloomfilter contains invalid characters.", -1);
		return;
	} 

	int i;
	for (i = 0; i < SQLITE_BLOOM_DEFAULT_SIZE; i++) {
		sscanf(bf_txt + i*16, "%016lX", &bf[i]);
	}
	
	mod = bloom_key2bf(key);

	int rc = 0;
	if (bf[mod/64] & (uint64_t)1 << (mod % 64)) {
		rc = 1;
	}

	sqlite3_result_int(context, rc);
}


//------------------------------------------------------------------------------
// Regular hash function on value
static void murmur3(sqlite3_context *context, int argc, sqlite3_value **argv) 
{
	const char *key = 0;  // input string
	uint64_t hash[2] = {0}; // container for the hash 
	char *out = 0; // output string

	// Return NULL hash for NULL values
	if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
		sqlite3_result_null(context);
		return;
	}

	out = (char *)malloc(32 + 1);

	key = (char *)sqlite3_value_text(argv[0]);
	MURMURHASH(key, strlen(key), MURMUR3_SEED, hash);
	sprintf(out, "%016lX%016lX", hash[0], hash[1]);
	out[32] = '\0';

	sqlite3_result_text(context, out, -1, free);
}


//------------------------------------------------------------------------------
// Initalisation of the sqlite functions
int sqlite3_extension_init(
		sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) 
{
	SQLITE_EXTENSION_INIT2(pApi);

	// The murmur3 hash function
	sqlite3_create_function(db, "murmur3", 1, SQLITE_ANY, 0, murmur3, 0, 0);
	// Bloom filter creation function
	sqlite3_create_function(db, "bloomfilter", 1, SQLITE_ANY, 0, 0, 
			bloom_step, bloom_finalize);
	// Bloom filter inclusion test
	sqlite3_create_function(db, "in_bloom", 2, SQLITE_ANY, 0, in_bloom, 0, 0);

	return 0;
}
