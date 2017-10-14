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

/* This simple test code shows how tracts can be used to keep threads in a
 * tract from accessing counter at the same time.  Not only doe the tract
 * keep the counter from getting corrupted it also makes the calculation
 * run faster when TASKLOOPS is small, because there is contention between
 * threads. */


#define NTRACT  4
#define TASKLOOPS   300
const uint32_t N = 1000;


static uint32_t counter[NTRACT];

static void* task(uintptr_t index)
{
    //printf(".");
    //fflush(stdout);
    ASSERT(index < NTRACT);
    int i;
    for(i=0;i<TASKLOOPS;++i) {
        // Here we are accessing the same static variable in
        // many threads, unless we have tracts for a given
        // index.
        //++counter[index];
        --counter[index];
    }
    for(i=0;i<TASKLOOPS;++i) {
        // Here we are accessing the same static variable in
        // many threads, unless we have tracts for a given
        // index.
        ++counter[index];
        //--counter[index];
    }

    ++counter[index];
    return NULL;
}


void run(struct POThreadPool_tract *tracts)
{    
    memset(counter, 0, sizeof(counter));
    double t;
    t = poTime_getDouble();

    struct POThreadPool *p;
    p = poThreadPool_create(10*NTRACT /*maxNumThreads*/,
            NTRACT*N/*maxQueueLength*/,
            100 /*maxIdleTime milli-seconds 1s/1000*/);

    uint32_t i,j;

    for(j=0; j<N; ++j)
        for(i=0; i<NTRACT; ++i)
        {
            uintptr_t index;
            index = i;
            struct POThreadPool_tract *tract = 0;
            if(tracts) tract = &tracts[i];
            poThreadPool_runTask(p, true/*waitIfFull*/, tract,
                (void *(*)(void *)) task, (void *) index);
        }

    // This will block until all threads finish.
    poThreadPool_tryDestroy(p, PO_THREADPOOL_LONGTIME);

    t = poTime_getDouble() - t;

    uint32_t num_failures = 0;

    for(j=0; j<NTRACT; ++j)
        if(counter[j] != N)
        {
            ++num_failures;
            printf("Tract %"PRIu32" got wrong counter %"
                    PRIu32" instead of %"PRIu32"\n",
                    j, counter[j], N);
        }

    VASSERT(!num_failures || !tracts, "This test FAILED!");

    printf("\nfinished %s %"PRIu32" out of %"PRIu32" failures in %g seconds\n\n",
            (tracts?"with tracts":"WITHOUT tracts"),
            num_failures, NTRACT, t);
}


int main(int argc, char **argv)
{
    struct POThreadPool_tract tract[NTRACT];
    poDebugInit();

    run(0); // run without tracts

    memset(tract, 0, sizeof(tract));
    run(tract);  // run with tracts

    run(0); // run without tracts

    memset(tract, 0, sizeof(tract));
    run(tract);  // run with tracts

    run(0); // run without tracts

    memset(tract, 0, sizeof(tract));
    run(tract);  // run with tracts

    run(0); // run without tracts

    memset(tract, 0, sizeof(tract));
    run(tract);  // run with tracts

    printf("%s SUCCESS\n", argv[0]);

    return 0;
}

