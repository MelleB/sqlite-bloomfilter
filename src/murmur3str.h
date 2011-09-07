
#ifndef MURMUR3STR_H
#define MURMUR3STR_H

#include <stdint.h>
#include <stdlib.h>

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


// The seed value for the hash function
uint32_t murmur3_seed;


// Turn a key into a hash
char * murmur3str(const char *key, size_t len);


#endif // MURMUR3STR_H

