#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


#include "debug.h"

#ifdef DEBUG_CONFIG_H
// Users can set OUT VOUT and SPEW_FILE
// For example they can use syslog
// or just a regular file.
#  include "DEBUG_CONFIG_H"
#endif


#ifndef SPEW_FILE
#  define SPEW_FILE stdout
#endif

#ifndef OUT
#  define OUT(fmt, ...)   fprintf(SPEW_FILE, fmt, ##__VA_ARGS__)
#endif

#ifndef VOUT
#  define VOUT(fmt, ap)   vfprintf(SPEW_FILE, fmt, ap)
#endif


static void vspew(const char *pre, const char *file,
        const char *func, int line,
        const char *fmt, va_list ap)
{
    OUT("%s%s:%s():%d: ", pre, file, func, line);
    VOUT(fmt, ap);
}


#ifdef DEBUG

static __thread enum PO_SPEW_LEVEL setLevel = -1;

void _spewLevel(enum PO_SPEW_LEVEL level)
{
    setLevel = level;
    const char *env;
    env = getenv("PO_SPEW_LEVEL");
    if(!env) return;
    if(*env == 'e' || *env == 'E' || *env == '0')
    {
        // error level
        setLevel = 0;
        return;
    }
    if(*env == 'w' || *env == 'W' || *env == '1')
    {
        // warn level
        setLevel = 1;
        return;
    }
    if(*env == 'n' || *env == 'N' || *env == '2')
    {
        // notice level
        setLevel = 2;
        return;
    }
    if(*env == 'i' || *env == 'I' || *env == '3')
    {
        // info level
        setLevel = 3;
        return;
    }
    if(*env == 'd' || *env == 'D' || *env == '4')
    {
        // debug level
        setLevel = 4;
    }
}
#endif


// active SPEW MACRO    level  
// ------------------  ---------
// ERROR                 0
// SPEW                  0
// WARN                  1
// NOTICE                2
// INFO                  3
// DSPEW                 4

void _spew(const char *pre, enum PO_SPEW_LEVEL level,
        const char *file, const char *func,
        int line, const char *fmt, ...)
{
#ifdef DEBUG
    if(setLevel < 0) _spewLevel(SPEW_LEVEL_DFT);
    if(setLevel < level) return;
#endif
    va_list ap;
    va_start(ap, fmt);
    vspew(pre, file, func, line, fmt, ap);
    va_end(ap);
}

// This will sleep or exit
void _assertAction(void)
{
    pid_t pid;
    pid = getpid();
#ifdef ASSERT_ACTION_EXIT
    OUT("Will exit due to error\n");
    exit(1); // atexit() calls are called
    // See `man 3 exit' and `man _exit'
#else // ASSERT_ACTION_SLEEP
    int i = 1; // User debugger controller, unset to effect running code.
    OUT("  Consider running: \n\n  gdb -pid %u\n\n  "
        "pid=%u will now SLEEP ...\n", pid, pid);
    while(i) { sleep(1); }
#endif
}

static void segSaultCatcher(int sig)
{
    ERROR("caught signal %d SIGSEGV\n", sig);
    _assertAction();
}

// Add this to the start of your code so you may see where your code is
// seg faulting.
void poDebugInit(void)
{
    struct sigaction s;
    memset(&s, 0, sizeof(s));
    s.sa_handler = segSaultCatcher;
    s.sa_flags = SA_RESETHAND;
    ASSERT(0 == sigaction(SIGSEGV, &s, 0));
}
