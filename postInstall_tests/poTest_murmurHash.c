/* run:
 
   apt-get install quickplot


  ./poTest_murmurHash | quickplot -iP

*/
#include <inttypes.h>
#include <stdio.h>

#include <potato.h>


int main(void)
{
    uint32_t i;

    for(i=0; i<20000; ++i)
        printf("%"PRIu32" %"PRIu32"\n", i, poMurmurHash(&i, sizeof(i), 0));

    return 0;
}
