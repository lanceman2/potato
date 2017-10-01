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

/*  Run:
 
   PO_SPEW_LEVEL=WARN ./threadPool_sleep

*/

/* The objective is to have threads that sleep some time, where we total
 * the time sleep by all threads add to a fixed total.  We do this in the
 * shortest amount of time.  If we have too many threads the overhead will
 * add lots of time.  With just 3 threads it may be about 3 times faster
 * than 1 thread.  We start with the same sleep time for all threads. */

// Total sleep time in micro seconds
#define TOTAL_SLEEP (10000000) // 10,000,000 micro seconds = 10 seconds

static useconds_t threadSleepTime;

static void* task(void *ptr)
{
    usleep(threadSleepTime);
    // This printing does not appear to add an lot of overhead compared to
    // other things.  Try removing the next two lines and see.
    printf(".");
    fflush(stdout);
    return NULL;
}

int runScenario(uint32_t totalTasks, uint32_t maxNumThreads,
        uint32_t destroyTime/*milli-seconds*/)
{
    threadSleepTime = TOTAL_SLEEP/totalTasks;

    double tstart = poTime_getDouble();

    double timeEstmate;
    if(maxNumThreads >= totalTasks)
        timeEstmate = threadSleepTime/((double)1000000);
    else
    {
        timeEstmate = threadSleepTime/((double)1000000);
        if(totalTasks % maxNumThreads)
            timeEstmate *= (1 + totalTasks/maxNumThreads);
        else
            timeEstmate *= (totalTasks/maxNumThreads);
    }

    printf("a total of %"PRIu32" tasks sleeping %u micro seconds each "
            "with %"PRIu32" threads in pool\n"
            "Without overhead it should take %g seconds\n",
        totalTasks, threadSleepTime, maxNumThreads,
        timeEstmate);

    struct POThreadPool *p;
    p = poThreadPool_create(true/*waitIfFull*/,
        10 /*maxQueueLength*/, maxNumThreads /*maxNumThreads*/,
        100 /*maxIdleTime milli-seconds 1s/1000*/);

    uint32_t i;

    for(i=0; i<totalTasks; ++i)
        // this will wait then the pool is full
        poThreadPool_runTask(p, 0, task, 0);

    uint32_t ret = 1;

    while(ret)
        // This will block destroyTime unless all threads finish.
        // returns 0 when they all have finished.
        ret = poThreadPool_tryDestroy(p, destroyTime);

    printf("\nfinished in %g seconds\n\n", poTime_getDouble() - tstart);

    return 0;
}


int main(int argc, char **argv)
{
    poDebugInit();

    runScenario(10, 10, 200);
    runScenario(50, 50, 100);
    runScenario(200, 20, 100);
    runScenario(500, 100, 100);
    runScenario(200, 200, 10);
    runScenario(200, 100, 10);
    runScenario(500, 500, 10);
    runScenario(1000, 1000, 10);


    WARN("%s Successfully Exiting", argv[0]);
    return 0;
}
