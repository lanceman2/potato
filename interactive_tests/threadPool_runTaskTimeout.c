/*RUN:

  PO_SPEW_LEVEL=info ./threadPool_runTaskTimeout

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
#include "define.h"
#include "threadPool.h"


static void* task(void *p)
{
    usleep(300000); // microseconds sec/1,000,000
    return NULL;
}


int main(int argc, char **argv)
{    
    struct POThreadPool *p;
    p = poThreadPool_create(10 /*maxNumThreads*/,
            20/*maxQueueLength*/,
            1000 /*maxIdleTime milli-seconds 1s/1000*/);

    uint32_t i;

    for(i=0; i<30; ++i)
        ASSERT(!poThreadPool_runTask(p, PO_LONGTIME, 0 /*tract*/, task, 0));

    for(i=0; i<10; ++i)
        ASSERT(PO_ERROR_TIMEOUT ==
            poThreadPool_runTask(p, 0/*timeOut milliseconds*/, 0 /*tract*/, task, 0));

    ASSERT(poThreadPool_tryDestroy(p, 10/*timeOut milliseconds*/) != 0);
    ASSERT(poThreadPool_tryDestroy(p, 10/*timeOut milliseconds*/) != 0);

    ASSERT(poThreadPool_tryDestroy(p, 100000/*timeOut milliseconds*/) == 0);

    printf("%s SUCCESS\n", argv[0]);

    return 0;
}
