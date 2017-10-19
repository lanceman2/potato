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

#if 1
static void* fasttask(void *p)
{
    int i = 0;
    ++i;
    i -= 3;
    return NULL;
}
#endif


static void* task(void *p)
{
    usleep(300000); // microseconds sec/1,000,000
    return NULL;
}

#define TOTAL_TASKS 30
#define MAX_WORKERS 10
#define QUEUE_MAX   (TOTAL_TASKS - MAX_WORKERS)


int main(int argc, char **argv)
{
    ASSERT(TOTAL_TASKS > 2);

    struct POThreadPool *p;
    p = poThreadPool_create(MAX_WORKERS /*maxNumThreads*/,
            QUEUE_MAX /*maxQueueLength*/,
            // With this short maxIdleTime we should see the number of
            // worker thread ramp up and down.
            100 /*maxIdleTime milli-seconds 1s/1000*/);

    uint32_t i, j;
    const uint32_t NLOOPS = 10;

    for(j=0; j<NLOOPS; ++j)
    {
        if(j)
            sleep(1);

        printf("loop %"PRIu32" of %"PRIu32"\n", j+1, NLOOPS);

        ASSERT(!poThreadPool_runTask(p, 0, 0 /*tract*/, fasttask, 0));
        ASSERT(!poThreadPool_runTask(p, 10, 0 /*tract*/, fasttask, 0));
        ASSERT(!poThreadPool_runTask(p, 0, 0 /*tract*/, fasttask, 0));
        ASSERT(!poThreadPool_runTask(p, 10, 0 /*tract*/, fasttask, 0));

        for(i=0; i<TOTAL_TASKS-2; ++i)
            ASSERT(!poThreadPool_runTask(p, PO_LONGTIME, 0 /*tract*/, task, 0));
        ASSERT(!poThreadPool_runTask(p, 100, 0 /*tract*/, task, 0));
        ASSERT(!poThreadPool_runTask(p, 100, 0 /*tract*/, task, 0));

        for(i=0; i<4; ++i)
            ASSERT(PO_ERROR_TIMEOUT ==
                poThreadPool_runTask(p, 10/*timeOut milliseconds*/,
                    0 /*tract*/, task, 0));
    }


    // These 4 calls should timeout.
    ASSERT(poThreadPool_tryDestroy(p, 10/*timeOut milliseconds*/) == TOTAL_TASKS);
    ASSERT(poThreadPool_tryDestroy(p, 10/*timeOut milliseconds*/) == TOTAL_TASKS);
    ASSERT(poThreadPool_tryDestroy(p, 10/*timeOut milliseconds*/) == TOTAL_TASKS);
    ASSERT(poThreadPool_tryDestroy(p, 10/*timeOut milliseconds*/) == TOTAL_TASKS);

    // This call should not timeout.
    ASSERT(poThreadPool_tryDestroy(p, 100000/*timeOut milliseconds*/) == 0);

    printf("%s SUCCESS\n", argv[0]);

    return 0;
}
