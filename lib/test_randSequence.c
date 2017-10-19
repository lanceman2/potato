#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "murmurHash.h"
#include "debug.h"
#include "randSequence.h"

int main(void)
{
#define MIN 50
#define MAX 100
    char buf[2][MAX];
    memset(buf[0], 0, MAX);
    memset(buf[1], 0, MAX);


    int i, b1 = 0, b2 = 1;

    for(i=0; i<200; ++i) {
        printf("%s\n",
            poRandSequence_string(buf[b1], buf[b2],
                0, MIN, MAX));
    
        // Switch the buffers.
        if(b1) { b1 = 0; b2 = 1; }
        else   { b1 = 1; b2 = 0; }
    }

    return 0;
}
