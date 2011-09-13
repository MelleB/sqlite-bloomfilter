
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "sqlite/sqlite3ext.h"
#include "murmur3/murmur3.h"

SQLITE_EXTENSION_INIT1


// The number of uint64 blocks that should be used
// for the bloom filter.
#define SQLITE_BLOOM_DEFAULT_SIZE 8 * 64

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


// The structure that contains the description of the bloomfilter
typedef struct bf_t {
	uint32_t size; // number of buckets
	uint64_t *filter;
} bf_t;


//------------------------------------------------------------------------------
// Calculate the bitfield position in the bloomfilter given
// a key and the number of bytes in the bloom filter
static uint64_t bloom_key2bf(const char *key, uint32_t nbbf)
{
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
// First parameter contains the value to hash
// Optional second parameter contains the size of the bloom filter (no. of bits)
static void bloom_step(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	// Skip NULL values
	if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
		return;
	}

	// Argument count should be 1 or 2
	if (argc != 1 && argc != 2) {
		sqlite3_result_error(context, "Invalid argument count.", -1);
	}

	uint32_t nbbf;
	uint64_t mod;
	const char *key;
	bf_t *bf;

	if (argc == 2) {
		int64_t sz = sqlite3_value_int64(argv[1]);
		if (sz < 1) {
			sqlite3_result_error(context, "Invalid bloom filter size", -1);
		} else {
			// Round up the number of buckets: nbbf = ((sz + (64 -1)) / 64)*64;
			nbbf = (uint32_t)(sz + (64 - sz % 64));
		}
	} else {
		nbbf = SQLITE_BLOOM_DEFAULT_SIZE;
	}

	// The container for the bloom filter
	bf = sqlite3_aggregate_context(context, sizeof(*bf));
	if (bf->filter == 0) {
		bf->filter = (uint64_t *)malloc(nbbf);
		if (bf->filter == 0) {
			sqlite3_result_error_nomem(context);
			return;
		} 
		memset(bf->filter, '\0', nbbf);

		bf->size = nbbf / 64;
	}

	key = (char *)sqlite3_value_text(argv[0]);
	mod = bloom_key2bf(key, nbbf);

	// Note: Explicit cast of 1 to uint64 is required.
	bf->filter[mod/64] |= (uint64_t)1 << (mod % 64);
}


//------------------------------------------------------------------------------
// Finally, the bloom filter is converted to a string representation
static void bloom_finalize(sqlite3_context *context)
{
	bf_t *bf;
	// The container for the bloom filter
	bf = sqlite3_aggregate_context(context, sizeof(*bf));

	char *out = (char *)malloc(16 * bf->size + 1);
	if (out == 0) {
		sqlite3_result_error_nomem(context);
		return;
	}
	memset(out, '\0', 16 * bf->size + 1);

	int i;
	for (i = 0; i < bf->size; i++) {
		// Note, buckets are placed from left to right.
		sprintf(out + i*16, "%016lX", bf->filter[i]);
	}

	free(bf->filter);

	sqlite3_result_text(context, out, 16 * bf->size, free);
}


//------------------------------------------------------------------------------
// Check if a value is contained within a bloom filter
static void in_bloom(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	const char *key;
	const char *bf_txt;
	bf_t bf;
	uint64_t mod;

	key = (char *)sqlite3_value_text(argv[0]);
	bf_txt = (char *)sqlite3_value_text(argv[1]);

	// Check if the length of the filter is correct
	// and consists of the correct values
	size_t bf_len = strlen(bf_txt);
	const char *hexchars = "01234567890ABCDEF";
	if (bf_len % 16 != 0) {
		sqlite3_result_error(context, "Bloomfilter of invalid size.", -1);
		return;
	} else if (strspn(bf_txt, hexchars) != bf_len) {
		sqlite3_result_error(context, 
				"Bloomfilter contains invalid characters.", -1);
		return;
	} 

	bf.filter = (uint64_t *)malloc(64 * bf_len / 16);
	if (bf.filter == 0) {
		sqlite3_result_error_nomem(context);
		return;
	} 
	memset(bf.filter, '\0', bf_len);
	bf.size = bf_len / 16;


	int i;
	for (i = 0; i < bf.size; i++) {
		sscanf(bf_txt + i*16, "%016lX", &(bf.filter[i]));
	}
	
	mod = bloom_key2bf(key, bf.size * 64);

	int rc = 0;
	if (bf.filter[mod/64] & (uint64_t)1 << (mod % 64)) {
		rc = 1;
	}

	free(bf.filter);

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
	// Bloom filter creation function (1 argument, [string])
	sqlite3_create_function(db, "bloomfilter", 1, SQLITE_ANY, 0, 0, 
			bloom_step, bloom_finalize);
	// Bloom filter creation function (2 arguments, [string], [size])
	sqlite3_create_function(db, "bloomfilter", 2, SQLITE_ANY, 0, 0, 
			bloom_step, bloom_finalize);
	// Bloom filter inclusion test
	sqlite3_create_function(db, "in_bloom", 2, SQLITE_ANY, 0, in_bloom, 0, 0);

	return 0;
}
