
// Output murmur3 key + 128 bit hash value
// Code modified from murmur3/example.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "murmur3/murmur3.h"

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

int main(int argc, char ** argv) {

	if (argc < 2) {
		printf("Usage: %s [key]\n", argv[0]);
		return 1;
	}

	uint64_t hash[2] = {0}; // container for the hash 
	MURMURHASH(argv[1], strlen(argv[1]), MURMUR3_SEED, hash);

	printf("Key:  %s\n", argv[1]);
	printf("Hash: %016lX%016lX\n", hash[0], hash[1]);

	return 0;
}

