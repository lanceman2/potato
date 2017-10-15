
static inline
bool mutexInit(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t mutexattr;
    return
        ASSERT((errno = pthread_mutexattr_init(&mutexattr)) == 0) ||
        ASSERT((errno = pthread_mutexattr_settype(&mutexattr,
                PTHREAD_MUTEX_FAST_NP)) == 0) ||
        ASSERT((errno = pthread_mutex_init(mutex, &mutexattr)) == 0) ||
        ASSERT((errno = pthread_mutexattr_destroy(&mutexattr)) == 0);
}

static inline
bool _mutexTryLock(pthread_mutex_t *mutex)
{
    errno = pthread_mutex_trylock(mutex);

    switch(errno)
    {
        case 0:
            return true; // we have the lock
        case EBUSY:
            return false; // we do NOT have the lock
        default:
            VASSERT(0, "pthread_mutex_trylock() failed");
    }
    return false; // oh shit
}

static inline
bool _mutexLock(pthread_mutex_t *mutex)
{
    while((errno = pthread_mutex_lock(mutex)) != 0)
    {
        if(errno != EINTR)
            return VASSERT(0, "pthread_mutex_lock() failed");
        WARN("pthread_mutex_lock() was interrupted by a signal");
    }
    return false;
}

#ifdef PO_PTHREADMUTEX_DEBUG

static inline bool
_MutexTryLock(pthread_mutex_t *mutex, const char *file, int line)
{
    bool ret = _mutexTryLock(mutex);
    if(!ret)
        ERROR("%s:%d:%s() failed", file, line, __func__);
    return ret;
}

static inline bool
_MutexLock(pthread_mutex_t *mutex, const char *file, int line)
{
    if(!_mutexTryLock(mutex))
    {
        ERROR("%s:%d:%s() blocked", file, line, __func__);
        return _mutexLock(mutex);
    }
    return false;
}

#  define mutexTryLock(x) _MutexTryLock((x), __BASE_FILE__, __LINE__)
#  define mutexLock(x) _MutexLock((x), __BASE_FILE__, __LINE__)

#else

#  define mutexTryLock(x) _mutexTryLock(x)
#  define mutexLock(x) _mutexLock(x)

#endif




static inline
bool mutexUnlock(pthread_mutex_t *mutex)
{
    return ASSERT((errno = pthread_mutex_unlock(mutex)) == 0);
}

static inline
bool condInit(pthread_cond_t *cond)
{
    return ASSERT((errno = pthread_cond_init(cond, NULL)) == 0);
}

static inline
bool mutexDestroy(pthread_mutex_t *mutex)
{
    return ASSERT((errno = pthread_mutex_destroy(mutex)) == 0);
}

static inline
bool condDestroy(pthread_cond_t *cond)
{
    return ASSERT((errno = pthread_cond_destroy(cond)) == 0);
}

static inline
bool condWait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    errno = 0;
    if((errno = pthread_cond_wait(cond, mutex)) != 0)
    {
        if(errno != EINTR)
            return VASSERT(0, "pthread_cond_wait() failed");
        WARN("pthread_cond_wait() was interrupted by a signal\n");
    }
    return false;
}

static inline
int condTimedWait(pthread_cond_t *cond, pthread_mutex_t *mutex,
        uint32_t timeOut /*milli-seconds = 1/1000 sec*/)
{
    struct timeval now;
    struct timespec timeout;
    ASSERT(gettimeofday(&now, 0) == 0);
    timeout.tv_sec = now.tv_sec + timeOut/1000;
    // tv_nsec is in nano seconds = 10^(-9) seconds
    // tv_usec is in microseconds = 10^(-6) seconds
    // timeOut in in milliseconds = 10^(-3) seconds
    timeout.tv_nsec = now.tv_usec*1000 + (timeOut%1000)*1000000;
    
    if(timeout.tv_nsec > 1000000000)
    {
        uint64_t extraSecs;
        extraSecs = timeout.tv_nsec/1000000000;
        timeout.tv_sec += extraSecs;
        timeout.tv_nsec -= extraSecs * 1000000000;
    }

    errno = 0;
    if((errno = pthread_cond_timedwait(cond, mutex, &timeout)) != 0)
    {
        if(errno != EINTR && errno != ETIMEDOUT)
        {
            VASSERT(0, "pthread_cond_timedwait() failed");
            return errno;
        }
        else if(errno == EINTR)
            WARN("pthread_cond_timedwait() was interrupted by a signal\n");
        // errno == ETIMEDOUT is the time expired case that will be
        // common.
        //else if(errno == ETIMEDOUT)
          //  WARN("pthread_cond_timedwait() time expired");
    }
    return errno;
}
