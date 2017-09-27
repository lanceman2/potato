
/// \cond SKIP

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

/// \endcond


/** create a potato thread pool
 *
 * This is the only function in the threadPool class that allocates
 * memory.  The memory is allocated only in this call, and the size
 * depends on the parameters maxQueueLength and maxNumThreads.
 *
 *  \param waitIfFull  if true, will cause poThreadPool_runTask()
 *  to block waiting for a free worker thread to become available.
 *  \param maxQueueLength  the maximum number of tasks that can
 *  be queue, waiting to be run via a worker thread.
 *  \param maxNumThreads  the maximum number of worker threads
 *  that can exist.  This does not include the master thread that
 *  calls poThreadPool_runTask().
 *  \param maxIdleTime  is the time that an idle worker thread
 *  will wait idly until it is destroyed.  The idle worker threads
 *  are only destroyed when poThreadPool_runTask() is called.
 *
 *  \return a pointer to an opaque struct POThreadPool
 */
extern
struct POThreadPool *poThreadPool_create(
        bool waitIfFull /* otherwise run() returns if full */,
        uint32_t maxQueueLength, uint32_t maxNumThreads,
        uint32_t maxIdleTime /*micro-seconds*/);

/** Waits for current jobs/requests to finish and then cleans up
 * all memory and thread resources.
 *
 * \return false on success, and true on the unlikely error case.
 */
extern
bool poThreadPool_destroy(struct POThreadPool *p);


/** add a task to the thread pool.
 *
 * Request to run a task with a thread in the pool of threads.  If the
 * there are maxNumThreads tasks running, or there is a task running
 * that is part of the same tract, the task may be queued if there
 * is room in the task queue and run later after running task finishes,
 * or else if there is no room in the queue and waitIfFull is true
 * this call with block until the queue or thread pool can handle the
 * task, or else if waitIfFull is false this call will return true
 * (for the error return case).
 *
 *
 * There are three cases for the value of tract to consider
 *
 *  1. with tract NULL (0)
 *      - if the maximum number of working threads is not reached
 *      the task is given to a idle or new worker thread, and false
 *      is returned
 *      - else if the maximum number of worker threads are working on
 *      tasks and the task queue is not full the task is queued, and false
 *      is returned
 *      - else if the maximum number of working threads and the queue is
 *      full and waitIfFull is set the call will block and wait until
 *      there is room in the queue
 *      - else if the maximum number of working threads and the queue is
 *      full and waitIfFull is not set the call return true (a failed
 *      case).
 *
 *  2. with the memory that tract points to set to zero, this acts the the
 *  same as case 1, but with a new tract being created, setting the memory
 *  pointed to by tract is used to create a new tract.
 *
 *  3. with the memory that tract points to values set from a previous
 *  call to poThreadPool_runTask(), this will act as in case 1, but with
 *  the added condition that the corresponding task will not run until no
 *  tasks from the same tract are running.
 *
 * This call must be called by the same thread for a given POThreadPool.
 *
 * The user needs to track their tracts.
 *
 * \param p returned from a call to poThreadPool_create()
 * \param tract may be NULL (0) to have the task unrelated to any other
 * tasks, pointing to memory that has been zeroed to have this task
 * associated with a new task, or use a value of task set from a previous
 * call to poThreadPool_runTask() to have the new task request associated
 * with an old tract.
 * \param callback a function to call in the task thread.
 * \param callbackData a pointer to pass to the \a callback function.
 *
 * \return false on success and the task will be running or queued to
 * run, or true in the case of an error.
 */
extern
bool poThreadPool_runTask(struct POThreadPool *p,
        struct POThreadPool_tract *tract,
        void *(*callback)(void *), void *callbackData);


/** Check for and remove a timed out thread from the pool.
 *
 * \return true if there are more idle threads to remove after this
 * call, else return false.
 */
extern
bool poThreadPool_checkIdleThreadTimeout(struct POThreadPool *p);


/** Checks if the tract is running a thread or has any pending thread
 * tasks.
 *
 * \return true when and if there are no pending or running tasks
 * for this tract, and resets the tract data structure for reuse, else
 * this returns false and nothing changes.
 */
extern bool
poThreadPool_checkTractFinish(struct POThreadPool *p,
        struct POThreadPool_tract *tract);
