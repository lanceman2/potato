/*RUN:

  PO_SPEW_LEVEL=info ./threadPool_checkIdle

*/
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <alloca.h>
#include <time.h>
#include <stdarg.h>
#include <inttypes.h>

#include "debug.h"
#include "tIme.h"
#include "threadPool.h"

/*RUN:

  PO_SPEW_LEVEL=info ./threadPool_checkIdle

*/

static void* task(void *p)
{
    usleep(30000); // microseconds sec/1,000,000
    return NULL;
}


int main(int argc, char **argv)
{    
    struct POThreadPool *p;
    p = poThreadPool_create(10 /*maxNumThreads*/,
            20/*maxQueueLength*/,
            1000 /*maxIdleTime milli-seconds 1s/1000*/);

    uint32_t i;

    for(i=0; i<10; ++i)
        poThreadPool_runTask(p, true/*waitIfFull*/, 0 /*tract*/, task, 0);


    for(i=0; i<10; ++i)
        // There should be no idle threads now.
        ASSERT(poThreadPool_checkIdleThreadTimeout(p) == false);

    usleep(900000); // microseconds sec/1,000,000
    usleep(900000); // microseconds sec/1,000,000

    for(i=10; i>1; --i) // All threads should be idle now.
    {
        bool ret;
        printf("poThreadPool_checkIdleThreadTimeout(p)=%"PRIu32"\n",
                ret=poThreadPool_checkIdleThreadTimeout(p));
        VASSERT(ret == true, "poThreadPool_checkIdleThreadTimeout(p) != true");

    }

    // This will block until all threads finish.
    ASSERT(poThreadPool_tryDestroy(p, 10) == 0);

    printf("%s SUCCESS\n", argv[0]);

    return 0;
}

