
// Output murmur3 key + 128 bit hash value
// Code modified from murmur3/example.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "murmur3str.h"
#include "murmur3/murmur3.h"


int main(int argc, char ** argv) {

	if (argc < 2) {
		printf("Usage: %s [key]\n", argv[0]);
		return 1;
	}

	char * hash = murmur3str(argv[1], strlen(argv[1]));

	printf("Key:  %s\n", argv[1]);
	printf("Hash2: %s\n", hash);

	free(hash);

	return 0;
}

