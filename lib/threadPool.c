#include "_threadPool.h"

// Note: Reading and writing errno is thread safe.  errno is a magic CPP
// macro that looks like a global variable.

static inline void *
alloc(size_t s)
{
    void *ret;
    ret = malloc(s);
    if(ASSERT(ret)) return NULL;
    // By initializing all the data to zero, we will have the correct
    // initialization for many struct POThreadPool variables.
    memset(ret, 0, s);
    return ret;
}


struct POThreadPool *poThreadPool_create(
        bool waitIfFull /* otherwise run() returns if full */,
        uint32_t maxQueueLength, uint32_t maxNumThreads,
        uint32_t maxIdleTime/*micro-seconds*/)
{
    struct POThreadPool *p;

    p = alloc(sizeof(*p));
    if(maxQueueLength)
        p->task = alloc(sizeof(*p->task)*maxQueueLength);
    if(maxNumThreads)
        p->worker = alloc(sizeof(*p->worker)*maxNumThreads);

    p->maxQueueLength = maxQueueLength;
    p->maxNumThreads = maxNumThreads;
    p->maxIdleTime = maxIdleTime;
    p->waitIfFull = waitIfFull;

#ifdef PO_DEBUG
    p->master = pthread_self();
#endif

    int i;

    if(maxQueueLength)
    {
        // Setup the tasks queue with all tasks in unused.
        struct POThreadPool_task *task;
        task = p->task;
        p->tasks.unused = task;
        // fill the unused stack
        for(i=0; i < maxQueueLength - 1; ++i)
            task[i].next = &task[i+1];
        task[i].next = NULL; // bottom of the stack
#ifdef PO_DEBUG
        p->tasks.unusedLength = maxQueueLength;
#endif
    }

    if(maxNumThreads)
    {
        // Setup the workers
        struct POThreadPool_worker *worker;
        worker = p->worker;
        // Setup the worker list as unused
        p->workers.unused = worker;
        for(i=0; i< maxNumThreads-1; ++i)
        {
            worker[i].pool = p;
            worker[i].next = &worker[i+1];
            condInit(&worker[i].cond);
        }
        worker[i].pool = p;
        worker[i].next = NULL;
        condInit(&worker[i].cond);
#ifdef PO_DEBUG
        p->workers.unusedLength = maxNumThreads;
#endif
    }

    mutexInit(&p->mutex);
    condInit(&p->cond);

    INFO("Created threadPool with queue length %d, "
        "%d available threads\n",
        maxQueueLength,
        maxNumThreads);

    return p;
}


// Removes a worker from the idle worker list and signals the waiting
// worker thread so it will wake out.  What happens to the worker after it
// waits up depends on chance given that we can't control when it wakes
// up.  It's only guaranteed to wake up, but we can't say when, and there
// could be new tasks by the time it wakes up.  We signal the older worker
// thread before the younger, so that we tend to remove threads with
// longer idle times.
static inline
void workerOldIdlePopSignal(struct POThreadPool *p, double t)
{
    DASSERT(p);
    struct POThreadPool_worker *worker;
    // pop off the front
    worker = p->workers.idleFront;
    DASSERT(worker);
    DASSERT(!p->workers.idleBack->prev);
    DASSERT(worker >= p->worker);
    DASSERT(worker <= &p->worker[p->maxNumThreads-1]);
    DASSERT(!worker->next);
    p->workers.idleFront = worker->prev;
    if(worker->prev)
    {
        worker->prev->next = NULL;
        worker->prev = NULL;
    }
    else
    {
        DASSERT(p->workers.idleBack == worker);
        p->workers.idleBack = NULL;
        // The idle list is empty now.
    }
    worker->next = NULL;

#ifdef PO_DEBUG
    --p->workers.idleLength;
    DASSERT(p->maxNumThreads >= p->numThreads);
#endif

    // Wake up this worker.
    ASSERT((errno = pthread_cond_signal(&worker->cond)) == 0);

    INFO("signaled thread %ld idle %.2lf seconds\n",
        (unsigned long) worker->thread, t - worker->lastWorkTime);
}


// We need a threadPool mutex lock to call this.
bool _poThreadPool_destroy(struct POThreadPool *p)
{
    DASSERT(p);
    DASSERT(p->numThreads <= p->maxNumThreads);
    DASSERT(!p->cleanup);

    double t;
    t = poTime_getDouble();

    while(p->workers.idleFront)
    {
        // The General queue should be empty.
        DASSERT(!p->tasks.front);
        DASSERT(!p->tasks.back);

        // We have idle workers waiting.
        workerOldIdlePopSignal(p, t);
    }

    if(p->numThreads > 0)
    {
        p->cleanup = true;
        // Wait for the last thread to finish.  The last thread to finish
        // will signal us on the way out.
        condWait(&p->cond, &p->mutex);
        DASSERT(!p->cleanup); // debug sanity check
    }

    // The General Queue should be empty now.
    DASSERT(!p->tasks.front);
    DASSERT(!p->tasks.back);
    DASSERT(p->numThreads == 0);

#ifdef PO_DEBUG
    memset(p->task, 0, sizeof(*p->task)*p->maxQueueLength);
    memset(p->worker, 0, sizeof(*p->worker)*p->maxNumThreads);
#endif

    free(p->task);
    free(p->worker);

#ifdef PO_DEBUG
        memset(p, 0, sizeof(*p));
#endif

    free(p);

    return false; // success
}


// Waits for current jobs/requests to finish and then cleans up. 
bool poThreadPool_destroy(struct POThreadPool *p)
{
    DASSERT(p);

    DSPEW("\n");
    DASSERT(pthread_equal(pthread_self(), p->master));

    mutexLock(&p->mutex);

    bool ret;
    ret = _poThreadPool_destroy(p);

    mutexUnlock(&p->mutex);

    return ret;
}


// We must have the threadPool mutex lock to call this.
static inline
bool _poThreadPool_checkIdleThreadTimeout(struct POThreadPool *p)
{
    DASSERT(p);
    DASSERT(p->numThreads <= p->maxNumThreads);

    if(p->numThreads <= 1) return false; // keep at least one thread

    // Remove a timed-out thread. We just remove one worker thread.
    // Removing many threads at once may cause problems in many apps.  We
    // are assuming that this function is called regularly.

    double t;
    t = poTime_getDouble();

    if(p->workers.idleFront &&
            (t - p->workers.idleFront->lastWorkTime) >
            (p->maxIdleTime/1000.0))
        workerOldIdlePopSignal(p, t);

    // Returns true if there is more than one idle thread and
    // the oldest idle thread is old enough to retire.  
    return (p->workers.idleFront &&
            // if front == back then there is just on in the idle list.
            (p->workers.idleFront != p->workers.idleBack) &&
        (t - p->workers.idleFront->lastWorkTime) >
        (p->maxIdleTime/1000.0));
}


// We must have the threadPool mutex lock to call this.
// This function cannot change threadPool state, we just
// need to know the answer to the question:
static inline
bool tractHasRunningWorker(struct POThreadPool *p,
        struct POThreadPool_tract *tract)
{
    if(tract && tract->worker)
    {
        DASSERT(tract->worker >= p->worker);
        DASSERT(tract->worker <= &p->worker[
                p->maxNumThreads-1]);
        DVASSERT(tract == tract->worker->tract,
                "tract(%p)", tract);

        return true;
    }
    return false; // No running worker.
}




// This pops a young worker off the back of the idle stack (list) of
// workers.  The idle list is doubly linked so that we may remove the
// older workers from the front as they time-out due to have no work for a
// long time.  This also makes the worker ready to work (run) its' thread.
// Finally the idle worker threads are signaled here to go back to work.
// We assume (code is correct) that the worker is blocking on a call to
// pthread_cond_wait(), condWait() being a wrapper of pthread_cond_wait().
static inline
bool workerIdleYoungPop(struct POThreadPool *p,
        struct POThreadPool_tract *tract,
        void *(*callback)(void *), void *callbackData)
{
    struct POThreadPool_worker *worker;
    // If we have idle worker threads we should not have any queued
    // tasks:
    DASSERT(!p->tasks.back);
    DASSERT(!p->tasks.front);

    DASSERT(p->workers.idleFront);
    DASSERT(p->workers.idleBack);
    DASSERT(!p->workers.idleFront->next);
    DASSERT(!p->workers.idleBack->prev);

    worker = p->workers.idleBack;

    DASSERT(!worker->prev);
    DASSERT(worker >= p->worker);
    DASSERT(worker <= &p->worker[
                p->maxNumThreads-1]);

    p->workers.idleBack = worker->next;
 
    if(worker->next)
    {
        worker->next->prev = NULL;
        worker->next = NULL;
    }
    else
    {
        DASSERT(p->workers.idleFront == worker);
        p->workers.idleFront = NULL;
        // The idle worker list is empty.
    }

#ifdef PO_DEBUG
    --p->workers.idleLength;
    DASSERT(!p->workers.idleFront || p->workers.idleLength); 
#endif

    // Crack the whip.  Work on this slave!
    worker->userCallback = callback;
    worker->userData = callbackData;
    if(tract)
    {
        // This worker is working on a particular tract.  We are
        // starting or restarting work on a tract, i.e. it must be that
        // there is no current worker working on this tract.  By
        // definition a tract may only have one worker working on it
        // in a given in instant.
        DASSERT(!tract->worker);
        DASSERT(!worker->tract);
        tract->worker = worker;
        worker->tract = tract;
    }

    // This thread exists and it waiting on a condition variable; so
    // lets put it back to work.  ASSERT() is not removed if not
    // PO_DEBUG.
    ASSERT((errno = pthread_cond_signal(&worker->cond)) == 0);

    // Check and remove extra old unemployed threads.
    _poThreadPool_checkIdleThreadTimeout(p);

    return false; // Success.
}


// We must have a threadPool mutex lock to call this.
// The task passed to this is not in any list.
static inline
void tractQueueTask(struct POThreadPool *p,
        struct POThreadPool_task *task)
{
    DASSERT(p);

    // We have a worker thread running with this tract and nothing in
    // the General task queue, or this task just popped off the General
    // task queue, so we make a "blocked" (waiting) task that is kept by
    // the worker that is currently busy on another task.

    struct POThreadPool_tract *tract;
    DASSERT(task);
    tract = task->tract;
    DASSERT(tract);
    DASSERT(tract->worker);
    DASSERT(tract->worker->tract == tract);

    // put task in worker/tract queue
    task->next = NULL;

    if(tract->lastTask)
    {
        DASSERT(tract->firstTask);
        DASSERT(!tract->lastTask->next);
        tract->lastTask->next = task;
    }
    else
    {
        DASSERT(!tract->firstTask);
        tract->firstTask = task;
    }
    tract->lastTask = task;
}


typedef void *(*_poThreadPool_callback_t)(void *);

// Find a task for a running thread
// We much have the threadPool mutex lock to call this.
static inline
_poThreadPool_callback_t lookForWork(struct POThreadPool *p,
        struct POThreadPool_worker *worker,
        void **userData)
{
    // We can find tasks from 3 kinds of sources.
    DSPEW("\n");

    DASSERT(worker >= p->worker);
    DASSERT(worker <= &p->worker[
                p->maxNumThreads-1]);

    // 1. Worker has work in the worker struct
    // The worker was assigned work already.
    // This work came from a call to poThreadPool_runTask().
    //
    // This worker must have been idle and this work is considered
    // owned by this worker and is not in a list any more, and
    // there is no associated taskCount.
    if(worker->userCallback)
    {
        DASSERT(!worker->tract || worker->tract->worker == worker);
        *userData = worker->userData;
        return worker->userCallback;
    }

    // 2. Tract Queue
    // This has priority over the General Queue (3) below, for this was
    // added before when there was nothing in the General Queue.
    if(worker->tract && worker->tract->firstTask)
    {
        struct POThreadPool_task *task;
        struct POThreadPool_tract *tract;
        tract = worker->tract;
        DASSERT(tract->lastTask);
        DASSERT(tract->worker == worker);
        DASSERT(worker->tract == tract);


        // Pop from the tract task queue
        task = tract->firstTask;

        DASSERT(task >= p->task);
        DASSERT(task <= &p->task[p->maxQueueLength-1]);

        tract->firstTask = task->next;
        if(!tract->firstTask)
            tract->lastTask = NULL;

        // Put task on the unused tasks queue
        task->next = p->tasks.unused;
        p->tasks.unused = task;

        DASSERT(task->userCallback);

        DASSERT(tract->taskCount > 0);
        --tract->taskCount;
        
        *userData = task->userData;
        return task->userCallback;
    }

    // 3. General Queue
    if(p->tasks.front)
    {
        struct POThreadPool_task *task;
        DASSERT(p->tasks.back);
        // Pop a task off of tasks General queue front
        task = p->tasks.front;
        DASSERT(task >= p->task);
        DASSERT(task <= &p->task[p->maxQueueLength-1]);

        p->tasks.front = task->next;

        if(!p->tasks.front)
        {
            // We got the last task in the queue.
            DASSERT(task == p->tasks.back);
            p->tasks.back = NULL;
        }
#ifdef PO_DEBUG
        --p->tasks.queueLength;
#endif

        DASSERT(task->userCallback);

        if(task->tract)
        {
            if(task->tract->worker)
            {
                // We already have a worker working on this tract.
                // Add this task to the Tract task queue.
                // We'll run it as soon as there is no worker
                // running a task in that same tract.
                tractQueueTask(p, task);

                // Try again, there may be other tasks
                // we can do in the General queue.
                return lookForWork(p, worker, userData);
            }
            else
            {
                // We have no worker on this tract yet, so
                // this will become a tract bound worker.
                if(worker->tract)
                {
                    // This worker will lose this tract and
                    // switch to new task->tract, this tract
                    // has an empty tract queue now.
                    DASSERT(worker->tract != task->tract);
                    DASSERT(worker->tract->worker == worker);
                    DASSERT(!worker->tract->firstTask);
                    DASSERT(!worker->tract->lastTask);
                    worker->tract->worker = NULL;
                    //worker->tract = NULL;// set below
                }

                DASSERT(!task->tract->worker);
                worker->tract = task->tract;
                worker->tract->worker = worker;

                DASSERT(worker->tract->taskCount > 0);
                --worker->tract->taskCount;
            }
        }

        // Task is moved to the unused stack
        task->next = p->tasks.unused;
        p->tasks.unused = task;
#ifdef PO_DEBUG
        ++p->tasks.unusedLength;
#endif

 
        *userData = task->userData;
        return task->userCallback;
    }


    // At this point there is no work, except work that is bound to
    // another worker, but not this worker.

    if(worker->tract)
    {
        DSPEW("no work for tract(%p)\n", worker->tract);
        // This worker was working on a tract, but now there is no work
        // available now.  The tract still exists for the user, but
        // there is no need to bind this worker to this particular tract
        // anymore.

        // An Idle worker should not be bound to a tract so that we can
        // have other tracts use the idle worker.
        DASSERT(worker->tract->worker == worker);
        DASSERT(!worker->tract->firstTask);
        DASSERT(!worker->tract->lastTask);

        // Unbind this worker from the tract.
        worker->tract->worker = NULL;
        worker->tract = NULL;
    }

    // Sorry worker, you are unemployed with no tract.
    return NULL;
}


static inline
bool lastWorkerSignalCleanup(struct POThreadPool *p)
{
    if(p->cleanup)
    {
        // cleanup is set by the master when it is waiting.
        if(p->numThreads == 1)
        {
            // This is the last thread.  Signal the master
            // on the way out.
            ASSERT((errno = pthread_cond_signal(&p->cond)) == 0);
#ifdef PO_DEBUG
            p->cleanup = false; // sanity check
#endif
        }
        return true;
    }
    return false;
}


static void
*workerPthreadCallback(struct POThreadPool_worker *worker)
{
    DASSERT(worker);
    DASSERT(worker->pool);
    struct POThreadPool *p;
    pthread_mutex_t *pmutex /*pool mutex*/;
    p = worker->pool;
    pmutex = &p->mutex;

    // This worker here now is not in any threadPool worker list.


         /////////////////////////////////////////////////|
        ////////////// ACCESSING POOL DATA ////////////////|
       /////////////////////////////////////////////////////|
      /////                                              \///|
     /*-*/              mutexLock(pmutex);                ////|
    /////                                                  \///|
    

    void *(*userCallback)(void *);
    void *userData;

    // Copy data to the stack in case things change while we did not
    // have the pmutex lock while we were calling the user callback.

    userCallback = worker->userCallback;
    userData = worker->userData;

    ++p->numThreads;
    DASSERT(p->numThreads <= p->maxNumThreads);

    // extra spaces to line up with other print below
    INFO("adding thread, there are now %"PRIu32" threads\n",
            p->numThreads);

    // Now tell the manager thread to proceed:
    //
    // This is signaling the thread at function void launchWorkerThread()
    // from a user call to poThreadPool_runTask().
    ASSERT((errno = pthread_cond_signal(&p->cond)) == 0);

    while(true)
    {

        worker->userCallback = NULL;

        DASSERT(!worker->tract ||
            (worker->tract->worker == worker));
        DSPEW("tract(%p)\n", worker->tract);


        ////|                                                  /////
         /*-*/             mutexUnlock(pmutex);               /////
          /////                                              /////
           //////////////////////////////////////////////////////
            /////////// FINISHED ACCESSING POOL DATA ///////////
             //////////////////////////////////////////////////

        DSPEW("starting task\n");

        //////////////// go to work on the task ///////////////////
        userCallback(userData); // working callback
        ////////////////// finished work on task //////////////////

        DSPEW("finished task\n");


             /////////////////////////////////////////////////|
            ////////////// ACCESSING POOL DATA ////////////////|
           /////////////////////////////////////////////////////|
          /////                                              \///|
         /*-*/              mutexLock(pmutex);                ////|
        /////                                                  \///|

        DASSERT(!worker->tract ||
            (worker->tract->worker == worker));

        if(p->taskWaitingToBeRun)
        {
            p->taskWaitingToBeRun = false;
            ASSERT((errno = pthread_cond_signal(&p->cond)) == 0);
            // We'll get this waiting task after the managing
            // thread starts running again, but not while
            // we hold this mutex lock.
        }

        if((userCallback = lookForWork(p, worker, &userData)))
            continue;

        if(lastWorkerSignalCleanup(p))
            break;

        // This worker may not be assigned to a tract, otherwise
        // lookForWork() would have found work.
        DASSERT(!worker->tract);


        // So we can tell if we get a new task after this.
        worker->userCallback = NULL;

        worker->lastWorkTime = poTime_getDouble();

        // Put this worker in the idle worker list so that the managing
        // thread can signal it to put it back to work or make it return.
        DASSERT(!worker->prev);
        DASSERT(!worker->next);
        worker->next = p->workers.idleBack;
        if(p->workers.idleBack)
        {
            DASSERT(p->workers.idleFront);
            p->workers.idleBack->prev = worker;
        }
        else
        {
            DASSERT(!p->workers.idleFront);
            p->workers.idleFront = worker;
        }
        p->workers.idleBack = worker;
#ifdef PO_DEBUG
        ++p->workers.idleLength;
#endif

        ///////////////////////////////////////////////////////////////
        ////////////////////// IDLE WORKER SLEEP //////////////////////
        ///////////////////////////////////////////////////////////////
        // We wait until the manager pops this worker off for new task.
        //
        // pthread_cond_wait() does unlock pmutex, wait for signal,
        // and then lock pmutex
        condWait(&worker->cond, pmutex);
        // Note: a thread can sit here an arbitrary amount of time,
        // even if it is signaled.hr
        ///////////////////////////////////////////////////////////////

        if((userCallback = lookForWork(p, worker, &userData)))
            continue;

        lastWorkerSignalCleanup(p);

        break; // this thread will exit

    } ///////////////////////////// while(true) loop


    --p->numThreads;

    INFO("removing thread, there are now %"PRIu32" threads\n",
                p->numThreads);

    // put this worker in the unused worker stack
    worker->next = p->workers.unused;
    p->workers.unused = worker;

#ifdef PO_DEBUG
    ++p->workers.unusedLength;
    DASSERT(p->maxNumThreads > p->numThreads);
#endif

    ////|                                                  /////
     /*-*/             mutexUnlock(pmutex);               /////
      /////                                              /////
       //////////////////////////////////////////////////////
        /////////// FINISHED ACCESSING POOL DATA ///////////
         //////////////////////////////////////////////////

#ifdef PO_THREADPOOL_DETACH
    ASSERT((errno = pthread_detach(pthread_self())) == 0);
#endif

    return NULL;
}


// We must have the threadPool mutex lock to call this.
static inline
void launchWorkerThread(struct POThreadPool *p,
        struct POThreadPool_worker *worker)
{
    pthread_attr_t attr;

    DASSERT(p->numThreads < p->maxNumThreads);
    ASSERT((errno = pthread_attr_init(&attr)) == 0);

#if 0 // For Debugging stack size.
    size_t stackSize;
    ASSERT((errno = pthread_attr_getstacksize(&attr, &stackSize)) == 0);
    ERROR("stacksize= %zu MBytes\n", stackSize/(1024*1024));
#endif

#ifdef PO_THREADPOOL_STACKSIZE
    ASSERT((errno = pthread_attr_setstacksize(&attr,
            PO_THREADPOOL_STACKSIZE)) == 0);
#endif

    // We will add one more thread now
    DASSERT(p->numThreads < p->maxNumThreads);

    ASSERT((errno = pthread_create(&worker->thread, &attr,
            (void *(*)(void *)) workerPthreadCallback, worker)) == 0);

    // Wait for the thread to get going so that the POThreadPool structure
    // is consistent, and so on.  We assume that the user does not call
    // poThreadPool_runTask() from more than one thread in the process, so
    // this will only continue from here if the new worker is about to go
    // to work calling worker->userCallback().
    condWait(&p->cond, &p->mutex); // release mutex lock, wait
    // got mutex lock now

    DASSERT(p->numThreads <= p->maxNumThreads);
}


// Launch a thread with an unused worker
// We must have the threadPool mutex lock to call this.
static inline
bool workerUnusedPop(struct POThreadPool *p,
        struct POThreadPool_tract *tract,
        void *(*callback)(void *), void *callbackData)
{
    struct POThreadPool_worker *worker;
    DASSERT(p->workers.unused);
    // Grab an unused worker by popping the unused worker stack
    worker = p->workers.unused;
    DASSERT(worker >= p->worker);
    DASSERT(worker <= &p->worker[
                p->maxNumThreads-1]);
    DASSERT(!worker->prev);
    p->workers.unused = worker->next;
    worker->next = NULL;

#ifdef PO_DEBUG
    --p->workers.unusedLength;
#endif

    // There should be no General queued tasks
    DASSERT(!p->tasks.back);
    DASSERT(!p->tasks.front);
    DVASSERT(!p->tasks.queueLength,
            "p->tasks.queueLength=%d\n", p->tasks.queueLength);

    worker->userCallback = callback;
    worker->userData = callbackData;
    if(tract)
    {
        DASSERT(!tract->worker);
        DASSERT(!worker->tract);
        // We are starting or restarting work on a tract and
        // there is no current worker working on this tract.
        tract->worker = worker;
        worker->tract = tract;
    }
    launchWorkerThread(p, worker);
    return false; // success
}


// We must have the threadPool mutex lock to call this.
static
bool _poThreadPool_runTask(struct POThreadPool *p,
        struct POThreadPool_tract *tract,
        void *(*callback)(void *), void *callbackData)
{
    DASSERT(p);
    DASSERT(callback);
    DASSERT(p->numThreads <= p->maxNumThreads);

    bool hasTractWorker;
    hasTractWorker = tractHasRunningWorker(p, tract);
    // hasTractWorker is set if we have a tract with a worker that has a
    // running thread, so it will block running a thread for this request.
    // A tract is defined by having only one worker thread working on it
    // at a time.  The tract is like a baton that is must be held by
    // the worker that is currently working on the tract.


    // Can we try to put a thread to work on this new task now.
    
    if(p->workers.idleFront && !hasTractWorker)
    {
        //
        // ### CASE 1:  we have idle worker threads ready to work
        //
        // There should be no tasks in the General Queue
        DASSERT(!p->tasks.front);
        DASSERT(!p->tasks.back);

        // We use an idle worker thread in this case.
        return workerIdleYoungPop(p, tract, callback, callbackData);
        // returns false == success
    }

    if(p->workers.unused && !hasTractWorker)
    {
        //
        // ### CASE 2:  we have unused workers that can be working threads
        //
        // Launch a new thread with an unused worker
        return workerUnusedPop(p, tract, callback, callbackData);
        // returns false == success
    }



    // The rest are queuing or blocking cases.


    if(!p->tasks.unused)
    {
        //
        // ### CASE 3:  we need to queue this, but the queues are full
        //
        // We have no tasks to queue with, i.e. the queues are full.

        if(!p->waitIfFull) return true; // fail

        DASSERT(!p->taskWaitingToBeRun);
        p->taskWaitingToBeRun = true;
        // Next try we should have a free task struct to queue with
        // or a free worker thread to run with.
        NOTICE(
#ifdef PO_DEBUG
                "Using all %d tasks "
                "and general queue with %d tasks, waiting now\n",
                p->maxQueueLength,
                p->tasks.queueLength
#else
                "We ran out of tasks using all %d queued tasks, "
                "waiting now\n",
                p->maxQueueLength
#endif
                );
        condWait(&p->cond, &p->mutex);
        DASSERT(!p->taskWaitingToBeRun);
        // Try again.  There should be no state change up to now!
        return _poThreadPool_runTask(p, tract, callback, callbackData);
    }

    if(tract)
        // Add to the tract task Count because this will be queued
        // somewhere.
        ++tract->taskCount;


    if(p->tasks.front || !hasTractWorker)
    {
        //
        // ### CASE 4:  we put this task in the General queue
        //
        // General Queue
        // We can put this "task" in the regular General queue.
        // The General queue should be used so tasks stay 
        struct POThreadPool_task *task;

        // pop a task off the unused task stack.
        task = p->tasks.unused;
        p->tasks.unused = task->next;

        // Put this task in back of the General task queue.
        if(p->tasks.back)
        {
            DASSERT(p->tasks.front);
            p->tasks.back->next = task;
        }
        else
        {
            DASSERT(!p->tasks.front);
            p->tasks.front = task;
        }
        p->tasks.back = task;
        task->next = NULL;

#ifdef PO_DEBUG
        ++p->tasks.queueLength;
        --p->tasks.unusedLength;
#endif
        task->userCallback = callback;
        task->userData = callbackData;
        task->tract = tract;

        return false; // success, it's queued in the General queue.
    }


    // ### CASE 5:  we have a tract worker thread running and there
    //              are no tasks in the General queue, so we may add
    //              this task to tract task queue.

    DASSERT(!p->tasks.front);
    DASSERT(!p->tasks.back);
    DASSERT(!p->tasks.queueLength);
    DASSERT(hasTractWorker);
    DASSERT(tract);

    struct POThreadPool_task *task;

    // pop the task from unused
    task = p->tasks.unused;
    p->tasks.unused = task->next;

    task->userCallback = callback;
    task->userData = callbackData;
    task->tract = tract;
#ifdef PO_DEBUG
    --p->tasks.unusedLength;
#endif

    tractQueueTask(p, task);

    // Now it should be run by the working thread after all
    // the other tasks in the worker tract task list.  It's
    // in a "Blocked" tract task queue which is kept in the
    // particular worker/tract tract data structure.
    return false;
}


/* Add a task to the thread pool.
 *
 * If tract is NULL no tract is used.
 *
 */
bool poThreadPool_runTask(struct POThreadPool *p,
        struct POThreadPool_tract *tract,
        void *(*callback)(void *), void *callbackData)
{
    DASSERT(p);

    DASSERT(pthread_equal(pthread_self(), p->master));

    mutexLock(&p->mutex);

    DSPEW("\n");

    bool ret;
    ret = _poThreadPool_runTask(p, tract,
            callback, callbackData);

    mutexUnlock(&p->mutex);

    return ret;
}


// this is also done in poThreadPool_runTask()
bool poThreadPool_checkIdleThreadTimeout(struct POThreadPool *p)
{
    DASSERT(p);
    DASSERT(p->numThreads <= p->maxNumThreads);

    DSPEW("\n");

    mutexLock(&p->mutex);

    bool ret;
    ret = _poThreadPool_checkIdleThreadTimeout(p);

    mutexUnlock(&p->mutex);

    return ret;
}


bool
poThreadPool_checkTractFinish(struct POThreadPool *p,
        struct POThreadPool_tract *tract)
{
    DASSERT(p);
    DASSERT(tract);
    DASSERT(p->numThreads <= p->maxNumThreads);
    bool ret = false;

    mutexLock(&p->mutex);

    if(tract->taskCount == 0 && !tract->worker)
    {
        memset(tract, 0, sizeof(*tract));
        ret = true;
    }
#ifdef PO_DEBUG
    else
    {
        if(tract->lastTaskCount != 0)
            // We make sure that the stupid user is not adding
            // tasks to this tract.
            DASSERT(tract->lastTaskCount >= tract->taskCount);
        tract->lastTaskCount = tract->taskCount;
    }
#endif

    mutexUnlock(&p->mutex);

    return ret; // true = tract has no queues or running tasks
}
