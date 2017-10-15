/** \file threadPool.h
 *
 * The potato thread pool.
 *
 * The potato thread pool has a few more features than some thread pools.
 * 
 * \section thread_pool_data_structure thread pool memory
 * 
 * This figure just shows the different possible lists in the \p
 * POThreadPool data structure.  This figure is not a valid state of the
 * data structure; there should not be queued tasks in the General queue
 * if there are idle or unused workers.  This figure is just to illustrate
 * all possible list and how the memory is managed.  The memory for these
 * lists is fixed, as is the number of CPU cores on your computer.  We see
 * on point in using unlimited threads on computers with limited cores.
 * Some of the lists are doubly linked and some are not.  Clearly the \e
 * used lists are not required to be doubly linked.  Writing this code is
 * not easy without this figure.
 *
 * \htmlonly
 *   <a href="threadPool_drawing.png" title="larger image">
 * \endhtmlonly

 *
 * \image html threadPool_drawing_sm.png
 *
 * \htmlonly
 *    </a>
 * \endhtmlonly
 *
 * \section thread_pool_size thread pool size
 *
 * The potato thread pool has a maximum number of running threads that is
 * set by the user.  The number of running threads is increases up to this
 * maximum size by successive calls to poThreadPool_runTask() and when the
 * maximum number of working threads is reached poThreadPool_runTask()
 * will either block until a thread finishes a task or return an error,
 * depending on user preferences.
 *
 * \section task_queue task queue
 *
 * The potato thread pool has a waiting queued with a user set maximum
 * size.  All incoming tasks are prioritised by first in being run first
 * to run with the exception that no more than one task in a "tract" can
 * be run at a time.
 *
 * \section tracts tracts
 *
 * The user may make a group tasks that that we call a tract.  Tasks that
 * are in a given tract never run currently with tasks that are in the
 * same tract.  Tracts of tasks can get queued with non-tract tasks, but
 * once a task of a given tract gets to the front of the line it will be
 * stuck in the queue until there are no tasks in that tract running.
 * Tasks in a tract alway run in the order in which they are asked to run
 * via poThreadPool_runTask().
 *
 * If you have tasks that share resources that you want to not run
 * concurrently, use a tract for each such a group.  Recycle the tract
 * when you know the last task in the tract has run.  The user must manage
 * the opaque struct POThreadPool_tract memory, but the memory cannot be
 * recycled until poThreadPool_checkTractFinish() true.
 *
 * \section thread_wind_down thread wind down
 *
 * The user may set a maxIdleTime, in microseconds, in which idle workers
 * threads are stopped.
 */


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
};

/// \endcond


/** create a potato thread pool
 *
 * Cutting down on thread contention by increasing design complexity in
 * the thread pool.  The user will have decreased design complexity and
 * increased performance in the limit of large number of service user
 * channels.  We introduce the thread "tract".  See poThreadPool_runTask().
 *
 * Create a potato thread pool with a set maximum number of working
 * threads, \p maxNumThreads, and a waiting task queue of length
 * \p maxQueueLength.
 *
 * This is the only function in the threadPool class that allocates memory
 * and the size depends on the parameters \p maxQueueLength and \p
 * maxNumThreads.
 *
 * \param maxNumThreads  the maximum number of worker threads that can
 * exist.  This does not include the master thread that calls
 * poThreadPool_runTask().
 *
 * \param maxQueueLength  the maximum number of tasks that can be queued,
 * waiting to be run via a worker thread.  If \p maxQueueLength is 0 the
 * created thread pool will not queue any tasks in a wait queue.  If you
 * use tracts of tasks, it's a good idea to have \p maxQueueLength large
 * enough to hold all the tasks that will be blocked by running tasks that
 * are in the same tract.
 *
 * \param maxIdleTime  is the time that an idle worker thread will wait
 * idly until it is destroyed in the next user invoked action, like
 * poThreadPool_checkIdleThreadTimeout().  The idle worker threads are
 * only destroyed when poThreadPool_runTask() or
 * poThreadPool_checkIdleThreadTimeout() are called.  if \p maxIdleTime is
 * zero than there will be no idle threads waiting for tasks.
 *
 *  \return a pointer to an opaque struct POThreadPool
 */
extern
struct POThreadPool *poThreadPool_create(
        uint32_t maxNumThreads, uint32_t maxQueueLength,
        uint32_t maxIdleTime /*micro-seconds*/);


/** Waits for all the current task requests to finish and then cleans up
 * all memory and thread resources, but only if all tasks are finished.
 *
 * This will not interrupt any threads that are working on tasks.
 *
 * \param timeOut the time to wait in milli-seconds for all tasks to
 * finish.  If \p timeOut is PO_LONGTIME this will wait
 * indefinitely.
 *
 * \param timeOut in milli-seconds.  poThreadPool_tryDestroy() will return
 * in \p timeOut milli-seconds.  If \p timeOut is PO_LONGTIME
 * poThreadPool_tryDestroy() will not return until all tasks are finished.
 * With \p timeOut in milliseconds and being a \a uint32_t, the longest
 * timeout is a little longer than 49.7 days (= 2^32 /(3600 * 1000 * 24)).
 *
 * \return 0 if all queued and running tasks completed, returns the
 * number of possible uncompleted tasks from the worker threads and the
 * number in the queue that have not completed, and returns
 * PO_THREADPOOL_ERROR in the unlikely error case.
 */
extern
uint32_t poThreadPool_tryDestroy(struct POThreadPool *p,
        uint32_t timeOut /*in milli-seconds. 1 milli-sec = 1/1000 of sec*/);


/** add a task to the thread pool.
 *
 * Request to run a task with a thread in the pool of threads.  If the
 * there are \p maxNumThreads tasks running, or there is a task running
 * that is part of the same tract, the task may be queued if there
 * is room in the task queue and run later after running task finishes,
 * or else if there is no room in the queue and \p waitIfFull is true
 * this call with block until the queue or thread pool can handle the
 * task, or else if \p waitIfFull is false this call will return true
 * (for the error return case).
 *
 * There are three cases for the value of tract to consider
 *
 *  1. with tract NULL (0)
 *      - if the maximum number of working threads is not reached
 *      the task is given to an idle or new worker thread, and false
 *      is returned
 *      - else if the maximum number of worker threads are working on
 *      tasks and the task queue is not full the task is queued in the
 *      wait queue, and false is returned
 *      - else if the maximum number of working threads and the queue is
 *      full and \p waitIfFull is set the call will block and wait until
 *      there is room in the wait queue
 *      - else if the maximum number of working threads and the queue is
 *      full and \p waitIfFull is not set the call return true (a failed
 *      case).
 *
 *  2. with the memory that \p tract points to set to zero, this acts as in
 *  case 1, but with a new tract being created, setting the memory pointed
 *  to by tract is used to create a new tract.  The user may use the tract
 *  as in case 3 below.
 *
 *  3. with the memory that \p tract points to values set from a previous
 *  call to poThreadPool_runTask(), this will act as in case 1, but with
 *  the added condition that the corresponding task will not run until no
 *  tasks from the same tract are running; if the task can't run it will
 *  be put in the wait queue, if it can, and so on ...
 *
 * This call must be called by the same thread that called the
 * poThreadPool_create() which returned \p p.
 *
 * All a tract does is keep tasks that are in the same tract from running
 * concurrently, but otherwise it's first come first serve in a line to
 * the pool.  The swimming pool with a queue line of people waiting to
 * enter, with tracts that keep not more than one person of a given
 * tract in the pool at once.  People in the same tract just can't swim at
 * the same time.  They just don't get along if they are in the same
 * tract.  If all the people are in the same tract than we have just one
 * person in the pool at a given time, but that is not the use case we
 * will use this for.
 *
 * Tracts provide a way to keep related threads from running concurrently
 * with much less overall contention than without the tracts.  You don't
 * need to add additional mutexes to keep "closely related" threads from
 * stepping on each other.  All the threads in a tract will never run
 * concurrently.  Without this "tract" hock you may have to have all
 * threads whither they are related or not share mutexes (or like thing).
 * Without tracts, there would be lots more contention in the case where
 * most threads are not sharing much data, or are shall we say threads
 * are "unrelated".  Tracts make potato faster than other web servers that
 * do not have "tracts", or sometime that provides tract-like behavior.
 * It's pretty simple.
 *
 * The user needs to track their tracts and manage the memory of their
 * tracts in struct POThreadPool_tract memory, on the stack or by
 * dynamically allocating them.  This simplifies the interfaces and code
 * of the potato thread pool considerably.
 *
 * \param p returned from a call to poThreadPool_create()
 * \param timeOut will wait of \p timeOut muilliseconds
 * for an available worker thread there are \p maxNumThreads
 * working already, or \p timeOut is PO_LONGTIME
 * wait forever.  Use \p timeOut equal to \p 0 if you wish to
 * return immediately.
 * \param tract may be NULL (0) to have the task unrelated to any other
 * tasks, pointing to memory that has been zeroed to have this task
 * associated with a new task, or use a value of task set from a previous
 * call to poThreadPool_runTask() to have the new task request associated
 * with an old tract.
 * \param callback a function to call in the task thread.
 * \param callbackData a pointer to pass to the \p callback function.
 *
 * \return 0 on success and the task will be running or queued to
 * run, or \p PO_ERROR_TIMEOUT in the case there the time out has expired.
 */
extern
int poThreadPool_runTask(struct POThreadPool *p,
        uint32_t timeOut, /*in milliseconds = 10^(-3) seconds*/
        struct POThreadPool_tract *tract,
        void *(*callback)(void *), void *callbackData);


/** Check for and remove a timed out idle thread from the pool.
 *
 * Idle threads may be removed in a call to poThreadPool_runTask(),
 * but if you just need to wind down your worker threads and not
 * add tasks you call call this.
 *
 * Note the potato thread pool does not bother to count the number
 * of running threads, that would require more memory and computer
 * resources, except in a DEBUG build.
 *
 * \param p a struct POThreadPool pointer returned from
 * poThreadPool_create().
 *
 * \return true if there are one or more idle threads to remove after this
 * call, else returns false.
 */
extern
bool poThreadPool_checkIdleThreadTimeout(struct POThreadPool *p);


/** Checks if the tract is running a thread or has any pending thread
 * tasks.
 *
 * \param p a pointer to a struct POThreadPool that was returned from
 * poThreadPool_create().
 * \param tract a pointer to a tract that was used in
 * poThreadPool_runTask().
 *
 * \return true when and if there are no pending or running tasks
 * for this tract, and resets the tract data structure for reuse, else
 * this returns false and nothing changes.
 */
extern bool
poThreadPool_checkTractFinish(struct POThreadPool *p,
        struct POThreadPool_tract *tract);
