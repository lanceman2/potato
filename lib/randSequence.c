#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "murmurHash.h"
#include "debug.h"
#include "randSequence.h"


// Hex encode.
static inline
char int2char(int i)
{
    if(i < 10)
        return (char) (i + 48);
    return (char) (i + 55);
}


// These numbers, R1 and R2, are nothing special.
#define R1 0x324A5231
#define R2 0xF3245233

// rlen = length of string returned without '/0' terminator.
// TODO: This is inefficient.  There must be a way to do this with less
// variables and less computation.
//
// This returns a string that is just a function of the what the input buf
// string is.  The sequence is hex encoded.
char *poRandSequence_string(const char *ibuf, char *buf,
        size_t *rlen/*returned*/, size_t minLen, size_t maxLen)
{
    DASSERT(minLen <= maxLen);
    DASSERT(minLen > 8);

    uint32_t seed[2];
    unsigned short int xsubi[3];

    seed[0] = poMurmurHash(ibuf, 4, R1);
    seed[1] = poMurmurHash(&ibuf[4], 4, R2);
    memcpy(xsubi, seed, sizeof(xsubi));

    int len;
    char *ret;
    ret = buf;
    len = minLen + nrand48(xsubi) % (maxLen - minLen);
    if(rlen)
        *rlen = len;
    while(len)
    {
        int j = 0;
        uint64_t val;
        val = nrand48(xsubi);
        //val = val << 16;
        unsigned char *ptr;
        ptr = (unsigned char *) &val;
        while(len && j < 6)
        {
            *buf = int2char((ptr[j/2] >> (4 * (j%2))) & 0x0F);
            ++buf;
            --len;
            ++j;
        }
    }
    *buf = '\0';
    return ret;
}
