/**
 * TODO: Insert LICENSE REF here
 */


#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "sqlite/sqlite3ext.h"
#include "murmur3str.h"

SQLITE_EXTENSION_INIT1


//------------------------------------------------------------------------------
// The bloom_step method is executed for each row in the target set
static void bloom_step(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	assert(argc == 1);
	if (sqlite3_value_type(argv[0]) != SQLITE_NULL) {
		const char * key;
		uint64_t * bf = sqlite3_aggregate_context(context, sizeof(uint64_t) * 2);

		uint64_t hash[2];
		key = (char *)sqlite3_value_text(argv[0]);
		MURMURHASH(key, strlen(key), murmur3_seed, hash);

		// Calculate the modulus of the 128 bit integer using substeps
		uint64_t mid = (hash[1] >> 32);
		uint64_t tmp1 = mid + ((hash[0] % 128) << 32);
		uint64_t tmp2 = (hash[1] & UINT32_MAX) + ((tmp1 % 128) << 32);
		uint64_t mod = tmp2 % 128;

		*bf |= (1 << mod);
	}
}


//------------------------------------------------------------------------------
// Finally, the bloom filter is converted to a string representation
static void bloom_finalize(sqlite3_context *context)
{
	uint8_t * hash = sqlite3_aggregate_context(context, sizeof(uint8_t) * 16);

	char * out = (char *)malloc(32 + 1);
	sprintf(out, "%02X%02X%02X%02X%02X%02X%02X%02X"
			     "%02X%02X%02X%02X%02X%02X%02X%02X", 
			     hash[0], hash[1], hash[2], hash[3], 
			     hash[4], hash[5], hash[6], hash[7], 
			     hash[8], hash[9], hash[10], hash[11], 
			     hash[12], hash[13], hash[14], hash[15]);
	sqlite3_result_text(context, out, -1, free);
}


//------------------------------------------------------------------------------
// Regular hash function on value
static void murmur_func(sqlite3_context *context, int argc, sqlite3_value **argv) 
{
	const char *key = 0;  // input string
	char *hash = 0; // output string

	assert(argc == 1);

	// Return NULL hash for NULL values
	if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
		sqlite3_result_null(context);
		return;
	}

	key = (char *)sqlite3_value_text(argv[0]);
	hash = murmur3str(key, strlen(key));

	sqlite3_result_text(context, hash, -1, free);
}


//------------------------------------------------------------------------------
// Initalisation of the sqlite functions
int sqlite3_extension_init(
		sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) 
{
	SQLITE_EXTENSION_INIT2(pApi);

	// The murmur3 hash function
	sqlite3_create_function(db, "murmur3", 1, SQLITE_ANY, 0, murmur_func, 0, 0);
	// Bloom filter creation function
	sqlite3_create_function(db, "bloomfilter", 1, SQLITE_ANY, 0, 0, 
			bloom_step, bloom_finalize);

	return 0;
}
