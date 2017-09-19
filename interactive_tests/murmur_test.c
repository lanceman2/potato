/* run:
 
   apt-get install quickplot


  ./murmur_test | quickplot -iP

*/
#include <inttypes.h>
#include <stdio.h>
#include "murmurHash.h"


int main(void)
{
    uint32_t i;

    for(i=0; i<20000; ++i)
        printf("%"PRIu32" %"PRIu32"\n", i, poMurmurHash(&i, sizeof(i), 0));

    return 0;
}
