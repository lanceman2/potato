#define _GNU_SOURCE
#include <stdio.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>


// A tract is a group of associated tasks that run in order and do not run
// simultaneously.  Tasks are not required to be associated with a tract.
#include "debug.h"
#include "tIme.h"
#include "_pthreadWrap.h" // mutexInit() mutexLock() etc...
#include "threadPool.h"
#include "define.h"

/* This is a bunch of linked lists, queues and stacks, that may be singly
 * linked (simple stack) or doubly linked (because we need to pull from
 * any point in the list).  The doubly linked lists use a minimal amount
 * of memory needed to keep us from having to do a O(N) search on the
 * static arrays of data. */

/////////////////////////////////////////////////////////////////////
// We must have a pool mutex lock to access any of this struct data
// That's in POThreadPool::mutex.
/////////////////////////////////////////////////////////////////////

struct POThreadPool_worker;
struct POThreadPool_task;


// This struct gets queued or stacked in many lists in POThreadPool_tasks
// It's requested task from the user.
struct POThreadPool_task
{
    // What a task will do for the user:
    void *(*userCallback)(void *);
    void *userData;

    // For putting task in a list in:
    //
    //  * POThreadPool_tasks::front, POThreadPool::tasks::back
    //          General queue
    //  * POThreadPool_tract::task
    //          Tract queue   added only when General queue
    //          is empty, so one worker gets particular tract
    //          tasks.
    //  * POThreadPool_tasks::unused
    //          Unused
    //
    struct POThreadPool_task *next;

    // tract is set if this is part of a tract.
    struct POThreadPool_tract *tract;
};


// List the tasks in this struct
struct POThreadPool_tasks
{
    // There are 3 kinds of task lists
    //
    //   General: General Queue Waiting to be run. Kept in back and front
    //            here in this structure.  There is just one list like
    //            this. New tasks go in the back.  This queue is used when
    //            we have p->maxNumThreads worker threads busy and we
    //            get call to poThreadPool_runTask(), so long as we have
    //            not used all the queue memory in p->tasks.
    //
    //
    //   Unused:  Memory that is not used at this time. Kept in unused in
    //            this structure.  Just one list like this.
    //
    //   Tract:   Waiting for a task with a given tract ID to finish so
    //            that it may run. Kept in tract task lists of a thread
    //            running worker.  This queue starts at a worker/tract.
    //            This queue is only added to when we have threads
    //            available but we are keeping the tasks of a tract from
    //            running concurrently.  It is to keep no more than one
    //            worker from working on a tract a given at a time.
    //
    //
    //  We only track the General and the Unused tasks here in this
    //  struct.  The Tract tasks gets tracked by the tracts, but just
    //  when the task can't be put in the General queue because the
    //  running of the task is blocked by a worker already working on the
    //  task.
    //
    //  We keep the order of tasks the same as the order that the tasks
    //  are requested via poThreadPool_runTask(), unless a task must cut
    //  in front of a tract task in other to keep more than one tracts
    //  task from running at the same time.

    // We think of it as a line at the DMV (department of motor vehicles)
    // task requests are added to the back and the front is where we get
    // the next task to act on.  The nag at the counter says: "Start at
    // the back of the line PLEASE!"
    struct POThreadPool_task *back, *front, // General queue
        // back->next == NULL

        // The unused task structs for this queue.  Keeps tabs on the
        // memory that is not counted in the list in "front" and "back" or
        // are in the tract structs.  Singly linked stack list.
        *unused; // this is a stack top
    // Others lists for in each tract

#ifdef DEBUG
    // These lengths must add to maxQueueLength
    uint32_t queueLength, unusedLength; 
#endif
};


struct POThreadPool_worker
{
    void *(*userCallback)(void *);
    void *userData;

    pthread_t pthread;

    // If there is no queue and there is a worker working on a task with
    // the same ID than poThreadPool_runTask() will block until this
    // worker is done, so it may be best to have at least a small queue
    // if you know that there will be some tasks that need to be
    // serialized.

    // Top class object data struct.
    struct POThreadPool *pool;

    // next and prev is for idle list (thread that exists but is not
    // working on a task) or unused (does not have a thread in existence)
    // list. 
    struct POThreadPool_worker *next, *prev;

    // Used to measure timeouts like: idle thread time out.
    double lastWorkTime;

    // Used to control the thread by the master thread.
    pthread_cond_t cond;

    // associated tract (if present)
    struct POThreadPool_tract *tract;

    // isWorking is set when this worker struct has a running thread that
    // is working on a user task and not in the idle thread list or the
    // unused worker list.  There is no list of working threads. The
    // working threads manage themselves.
    bool isWorking;
};


struct POThreadPool_workers
{
    // There are three kinds of workers in worker[] array:
    //    - idle (has a pthread that is blocked),
    //    - unused (have no thread or task), and
    //    - working (has a thread and is working on a task)
    //          and is not in a POThreadPool_workers list,
    //          but is part of the POThreadPool_worker array
    //          in struct POThreadPool

    struct POThreadPool_worker
        // A doubly listed list of workers with threads that are not
        // working on a task.  Like a queue line at the DMV.
        *idleBack, *idleFront, // idle list youngest at the back
        // We define this list like:
        // idleFront->next = NULL and idleBack->prev = NULL
        // We get fresher workers from the back of the line, and old
        // workers in the front get retired first.
        *unused;

        // The working workers are not keep in a list ... yet.  We may
        // need them listed so we can detect hung threads without doing a
        // O(N) search.  We'd order the working threads from shortest time
        // working to longest time working.

#ifdef DEBUG
    // maxNumThreads = 
    // unusedLength + idleLength + "working threads" =
    // unusedLength + POThreadPool::numThreads
    uint32_t idleLength, unusedLength;
#endif
};


struct POThreadPool
{
#ifdef DEBUG
    // The thread that calls poThreadPool_create() is the only thread
    // that may call poThreadPool_runTask()
    pthread_t master; // The thread that called poThreadPool_create().
#endif

    uint32_t numThreads; // number of pthreads that now exist
    // numThreads = (idle threads) + (working threads)

    struct POThreadPool_tasks tasks; // lists of tasks
    struct POThreadPool_task *task; // allocated memory for tasks

    // Their are three kinds of workers in worker[] array:
    // idle, unused, and working.

    // List of unused worker structs that have no thread in existence.
    // idle and unused list
    struct POThreadPool_workers workers;
    struct POThreadPool_worker *worker; // allocated memory for tasks
    // and working threads are not in a list since they have threads
    // that will act for them when they finish.


    pthread_mutex_t mutex; // protects POThreadPool data structures
    pthread_cond_t cond; // Used with mutex above.

    // flag for when main (master) thread is blocking
    // i.e. when calling pthread_cond_wait()
    bool cleanup; // blocking in poThreadPool_tryDestroy()

    // flag masks that poThreadPool_runTask() is calling
    // pthread_cond_wait() because there is no task room
    // in the General or tracts queues.
    bool taskWaitingToBeRun,
        // poThreadPool_runTask() waits if queues are full, otherwise not.
        waitIfFull;
    uint32_t maxQueueLength, maxNumThreads, maxIdleTime;
};
