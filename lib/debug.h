
///////////////////////////////////////////////////////////////////////////
//
// ##__VA_ARGS__ comma not eaten with -std=c++0x
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44317
//
// There is a GCC bug where GCC wrongly warns about ##__VA_ARGS__, so using
// #pragma GCC system_header suppresses these warnings.  Should this GCC
// bug get fixed, it'd be good to remove this next code line.
// See also: https://gcc.gnu.org/onlinedocs/cpp/System-Headers.html
#pragma GCC system_header

// We would like to be able to just call SPEW() with no arguments
// which can make a zero length printf format.
#pragma GCC diagnostic ignored "-Wformat-zero-length"

//
///////////////////////////////////////////////////////////////////////////
//
// The macros SPEW() ERROR() ASSERT() FAIL() are always on 
//


enum PO_SPEW_LEVEL {
    PO_SPEW_ERROR  = 0,
    PO_SPEW_WARN   = 1,
    PO_SPEW_NOTICE = 2,
    PO_SPEW_INFO   = 3,
    PO_SPEW_DEBUG  = 4
};


extern void _po_spew(const char *pre, enum PO_SPEW_LEVEL level,
        const char *file, const char *func,
        int line, const char *fmt, ...)
         // check printf format errors at compile time:
        __attribute__ ((format (printf, 6, 7)));

extern void _po_vspew(const char *pre, const char *file,
        const char *func, int line,
        const char *fmt, va_list ap);

extern void _po_assertAction(void);

static inline bool _assert(bool val, const char *pre,
        const char *file, const char *func,
        int line, const char *fmt, ...)
{
    if((bool) (val)) return false; // success
    va_list ap;
    va_start(ap, fmt);
    _po_vspew(pre, file, func, line, fmt, ap);
    va_end(ap);
    _po_assertAction();
    return true; // fail
}

/** /brief set up a SIGFAULT signal catcher
 */

extern void poDebugInit(void);


#define _SPEW(pre, level, fmt, ... )\
     _po_spew(pre, level,  __BASE_FILE__,\
        __func__, __LINE__, fmt "\n", ##__VA_ARGS__)


#define ERROR(fmt, ...)\
     _SPEW("ERROR: ", PO_SPEW_ERROR, fmt, ##__VA_ARGS__)

// SPEW is like ERROR but just called SPEW
#define SPEW(fmt, ...)\
     _SPEW("SPEW: ", PO_SPEW_ERROR, fmt, ##__VA_ARGS__)

#define _VASSERT(val, fmt, ...) \
    _assert((val), "ASSERT("#val") ", __BASE_FILE__,\
        __func__, __LINE__, "failed: " fmt, ##__VA_ARGS__)

#define ASSERT(val)                _VASSERT(val, "")
#define VASSERT(val, fmt, ...)     _VASSERT(val, fmt, ##__VA_ARGS__)


#define FAIL(fmt, ...) \
    _assert(false, "FAIL: ", __BASE_FILE__,\
        __func__, __LINE__, "failed: " fmt, ##__VA_ARGS__)

///////////////////////////////////////////////////////////////////////////


#ifdef DEBUG

#  if defined(SPEW_LEVEL_ERROR)\
    || defined(SPEW_LEVEL_WARN)\
    || defined(SPEW_LEVEL_NOTICE)\
    || defined(SPEW_LEVEL_INFO)\
    || defined(SPEW_LEVEL_DEBUG)
// We have a level set
#  else
// We do not have a spew level set so we
// set it to the default that we define here:
#    define SPEW_LEVEL_INFO
#  endif


#  ifdef SPEW_LEVEL_DEBUG // The highest verbosity
#    define SPEW_LEVEL_DFT PO_SPEW_DEBUG
#  endif
#  ifdef SPEW_LEVEL_INFO
#    define SPEW_LEVEL_DFT PO_SPEW_INFO
#  endif
#  ifdef SPEW_LEVEL_NOTICE
#    define SPEW_LEVEL_DFT PO_SPEW_NOTICE
#  endif
#  ifdef SPEW_LEVEL_WARN
#    define SPEW_LEVEL_DFT PO_SPEW_WARN
#  endif
#  ifdef SPEW_LEVEL_ERROR
// SPEW_LEVEL_ERROR must be defined
#    define SPEW_LEVEL_DFT PO_SPEW_ERROR
#  endif


#  ifdef SPEW_LEVEL_DEBUG
#  undef SPEW_LEVEL_DEBUG
#  endif
#  ifdef SPEW_LEVEL_INFO
#  undef SPEW_LEVEL_INFO
#  endif
#  ifdef SPEW_LEVEL_WARN
#  undef SPEW_LEVEL_WARN
#  endif
#  ifdef SPEW_LEVEL_NOTICE
#  undef SPEW_LEVEL_NOTICE
#  endif
#  ifdef SPEW_LEVEL_ERROR
#  undef SPEW_LEVEL_ERROR
#  endif


#  define DASSERT(val)            _VASSERT(val, "")
#  define DVASSERT(val, fmt, ...) _VASSERT(val, fmt, ##__VA_ARGS__)
#  define WARN(fmt, ...)   _SPEW("WARN: ", PO_SPEW_WARN, fmt, ##__VA_ARGS__)
#  define NOTICE(fmt, ...) _SPEW("NOTICE: ", PO_SPEW_NOTICE, fmt, ##__VA_ARGS__)
#  define INFO(fmt, ...)   _SPEW("INFO: ", PO_SPEW_INFO, fmt, ##__VA_ARGS__)
#  define DSPEW(fmt, ...)  _SPEW("DEBUG: ", PO_SPEW_DEBUG, fmt, ##__VA_ARGS__)

#else

#  define DASSERT(val)            /*empty marco*/
#  define DVASSERT(val, fmt, ...) /*empty marco*/
#  define WARN(fmt, ...)          /*empty macro*/
#  define NOTICE(fmt, ...)        /*empty macro*/
#  define INFO(fmt, ...)          /*empty macro*/
#  define DSPEW(fmt, ...)         /*empty macro*/

#endif

