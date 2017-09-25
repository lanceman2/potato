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

#include "debug.h"
#include "threadPool.h"


static void* task(size_t count)
{
    //fprintf(stderr, "running task %zu\n", count);
    //usleep(1000); // (1/1000) seconds
    ++count;
    return NULL;
}

int main(int argc, char **argv)
{
    poDebugInit();

    size_t runCount = 0;
    int i;

    struct POThreadPool *p;
    p = poThreadPool_create(true/*detach*/, true/*waitIfFull*/,
        21 /*maxQueueLength*/, 10 /*maxNumThreads*/,
        100 /*maxIdleTime milli-seconds 1s/1000*/);

    struct POThreadPool_tract tract[30];
    memset(tract, 0, sizeof(tract)*30);
    
    for(i=0;i<100;++i)
    {
        int j;
        for(j=0; j<30; ++j)
        {
            poThreadPool_runTask(p, &tract[j],
                (void *(*)(void *data)) task,
                (void *) runCount++);

        }

        for(j=0; j<0; ++j)
            poThreadPool_runTask(p, NULL,
                (void *(*)(void *data)) task,
                (void *) runCount++);
         
        usleep(10000);
        // Now all the tasks should be done.
        while(poThreadPool_checkIdleThreadTimeout(p));
        usleep(10000);
        // We should have one idle worker thread now.
    }

    poThreadPool_destroy(p);

    WARN("%s Successfully Exiting\n", argv[0]);
    return 0;
}
