/* From: https://en.wikipedia.org/wiki/MurmurHash
 *
 * This is totally thread safe.
 *
 * This was public domain code.
 *
 * MurmurHash is a non-cryptographic hash function suitable for general
 * hash-based lookup.  It was created by Austin Appleby in 2008.
 */
 
#include <inttypes.h>

#ifdef TEST
#  include <stdio.h>
#else
#  include "murmurHash.h"
#endif

#define C1 0xcc9e2d51
#define C2 0x1b873593
#define R1 15
#define R2 13

uint32_t poMurmurHash(const void *key, uint32_t len, uint32_t hash)
{
    const int nblocks = len / 4;
    int i;
    for (i = 0; i < nblocks; i++) {
    	uint32_t k = ((const uint32_t *) key)[i];
    	k *= C1;
	k = (k << R1) | (k >> (32 - R1));
	k *= C2;
	hash ^= k;
	hash = ((hash << R2) | (hash >> (32 - R2))) * 5 + 0xe6546b64;
    }
 
    const uint8_t *tail = (const uint8_t *)
            (((uint8_t *)key) + nblocks * 4);
    uint32_t k1 = 0;
 
    switch (len & 3) {
	case 3:
		k1 ^= tail[2] << 16;
	case 2:
		k1 ^= tail[1] << 8;
	case 1:
		k1 ^= tail[0];
		k1 *= C1;
		k1 = (k1 << R1) | (k1 >> (32 - R1));
		k1 *= C2;
		hash ^= k1;
    }
 
    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);
    return hash;
}

#ifdef TEST

// gcc -Wall -Werror -DTEST murmurHash.c -o x_z && ./x_z | quickplot -iP

#define LEN 10000

int main(void)
{
    uint32_t i;
    uint32_t num[LEN] = { 0 };

    for(i=0; i<1000000; ++i)
        ++num[poMurmurHash(&i, sizeof(i), 0x0)%LEN];

    for(i=0; i<LEN; ++i)
        printf("%"PRIu32" %"PRIu32"\n", i, num[i]);

    return 0;
}
#endif // #ifdef TEST

