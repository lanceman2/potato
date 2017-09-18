#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "murmurHash.h"
#include "random.h"

int main(void)
{
    struct PORandom r;

    poRandom_init(&r, 0245);

    int32_t i;

    for(i=0; i<20000; ++i)
        printf("%"PRIu32" %"PRIu32"\n", i, poRandom_get(&r));

    return 0;
}
