
#include "murmur3str.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "murmur3/murmur3.h"


// The seed value for the hash function
uint32_t murmur3_seed = 42;


// Turn a key into a hash
char * murmur3str(const char *key, size_t len) 
{

	uint8_t hash[16] = {0}; 
	char * out = (char *)malloc(32 + 1);
	assert(out != 0);

	MURMURHASH(key, len, murmur3_seed, hash);
	sprintf(out, "%02X%02X%02X%02X%02X%02X%02X%02X"
			     "%02X%02X%02X%02X%02X%02X%02X%02X", 
			     hash[0], hash[1], hash[2], hash[3], 
			     hash[4], hash[5], hash[6], hash[7], 
			     hash[8], hash[9], hash[10], hash[11], 
			     hash[12], hash[13], hash[14], hash[15]);

	out[32] = '\0';

	return out;
}

