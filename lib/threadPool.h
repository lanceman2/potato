
#define POTHREADPOOL_OUTOF_TRACTS ((void *) -1)
#define POTHREADPOOL_INFINITE_IDLE_TIME ((uint32_t) -1)


struct POThreadPool_tract;
struct POThreadPool;


// The user may make tracts for a series of thread tasks that run in
// sequence, without overlapping execution.
// The user can use this struct as a base class, and add the user
// data to it.
struct POThreadPool_tract
{
    struct POThreadPool_worker *worker; // if there is one working now
    // nextTask and lastTask are the tract task queue that is added to
    // when the General task queue is empty.
    struct POThreadPool_task *firstTask, // front of queued task
                            *lastTask; // the back of the queued task list

    // When taskCount goes to zero we can recycle this tract.
    uint32_t taskCount; // number of queued tasks that are associated.
#ifdef PO_DEBUG
    uint32_t lastTaskCount; // checks that the user is not adding tasks
    // after calling poThreadPool_finishTract().
#endif
};


// This is the only function in the threadPool class that
// allocates memory.
extern
struct POThreadPool *poThreadPool_create(
        bool detach, bool waitIfFull /* otherwise run() returns if full */,
        uint32_t maxQueueLength, uint32_t maxNumThreads,
        uint32_t maxIdleTime /*micro-seconds*/);

// Waits for current jobs/requests to finish and then cleans up.
extern
bool poThreadPool_destroy(struct POThreadPool *p);


/* Add a task to the thread pool.
 *
 * If *tract is NULL no tract is used.
 *
 * The user manages the tract memories, treating them as opaque data.
 * If the tract memory is zeroed these call initializes the tract
 * memory for a new tract.
 *
 * This call must be called by the same thread for a given POThreadPool.
 *
 *
 * The user needs to track tracts and not run out of them.
 *
 * Returns false on success and the task will be running or queued to
 * run.
 *
 * Returns true and sets *tract to POTHREADPOOL_OUTOF_TRACTS (which is
 * not NULL), if we ran out of tracts.  The user needs to call
 * poThreadPool_removeTract() to fix this.
 *
 * If the queue is full and mode == POThreadPool_RETURN
 * Returns true and does not set *tract if the queue is full.  What you
 * can do is have this calling thread do the task it's self.  This maybe a
 * better solution to blocking and waiting for the queue to open up, and
 * if the task is longer than others make the queue size large enough to
 * keep the threads in the pool working.
 *
 * If the queue is full and mode == POThreadPool_RUN this
 * will call the userCallback() in the calling thread and return false.
 *
 * If the queue is full and mode == POThreadPool_WAIT the
 * call will block until there the task is queued or run, and than false
 * is returned.
 */
extern
bool poThreadPool_runTask(struct POThreadPool *p,
        struct POThreadPool_tract *tract,
        void *(*callback)(void *), void *callbackData);


// Check for and remove a timed out thread from the pool.
//
// Returns true if there are more idle threads to remove after this
// call.
extern
bool poThreadPool_checkIdleThreadTimeout(struct POThreadPool *p);


/** Checks if the tract is running a thread or has any pending thread
 * tasks.  Returns true when and if there are no pending or running tasks
 * for this tract, and resets the tract data structure for reuse, else
 * this returns false and nothing changes.
 */
extern bool
poThreadPool_checkTractFinish(struct POThreadPool *p,
        struct POThreadPool_tract *tract);
