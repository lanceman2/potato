#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "debug.h"
#include "tIme.h"


// Low res uses less system resources
#  define PO_CLOCK       CLOCK_REALTIME_COARSE

// fraction of seconds to show in poTime_get()
#  define PO_PRECISION  0.01

// High resolution uses more system resources
// DO NOT USE THIS CODE, just for testing
//#  define PO_CLOCK       CLOCK_REALTIME

// fraction of seconds to show in poTime_get()
//#  define PO_PRECISION  0.00000001


char *poTime_get(char *buf, size_t bufLen)
{
    DASSERT(bufLen > 0);
    struct timespec t;
    // As of today, 24 June 2015, the resolution looks to be around 1/100
    // of a second.
    if(ASSERT(clock_gettime(PO_CLOCK, &t) == 0))
    {
        strncpy(buf, "no time", bufLen);
        buf[bufLen-1] = '\0';
        return buf;
    }
    DASSERT(t.tv_nsec < 1.0e+9);

    double nsec;
    // save just some digits // stop funny rounding
    nsec = (t.tv_nsec + 0.5)*(1.0e-9/PO_PRECISION);
    t.tv_nsec = nsec;

    snprintf(buf, bufLen, "%ld.%2.2ld", t.tv_sec, t.tv_nsec);
    return buf;
}

double poTime_getDouble()
{
    struct timespec t;
    if(ASSERT(clock_gettime(PO_CLOCK, &t) == 0))
        return 0.0;
    DASSERT(t.tv_nsec < 1.0e+9);

    return t.tv_sec + (t.tv_nsec + 0.5) * 1.0e-9;
}

#undef PO_CLOCK
#undef PO_PRECISION

// Don't use this in the server.  It will eat system resources.
double poTime_getRealDouble()
{
    struct timespec t;
    if(ASSERT(clock_gettime(CLOCK_REALTIME, &t) == 0))
        return 0.0;
    DASSERT(t.tv_nsec < 1.0e+9);

    return t.tv_sec + (t.tv_nsec + 0.5) * 1.0e-9;
}
