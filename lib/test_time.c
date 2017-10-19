/* run:
 
   ./test_time | quickplot -P

*/

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "debug.h"
#include "tIme.h"

int main(void)
{
    int i;
    double dt_offset;
    struct timespec t = { 0, 1000000 };
    dt_offset = poTime_getDouble();

    for(i=0;i<1000;++i)
    {
        printf("%d %g\n", i, poTime_getDouble() - dt_offset);
        ASSERT(nanosleep(&t, 0) == 0);
    }

    char buf[512];

    for(;i<0;++i)
    {
        printf("%d %s\n", i, poTime_get(buf, 512));
        ASSERT(nanosleep(&t, 0) == 0);
    }

    return 0;
}
