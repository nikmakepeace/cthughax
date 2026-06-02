/*
    CTHUGHA-L 							cthugha.h
*/

#ifndef __CTHUGHA_H__
#define __CTHUGHA_H__

#include "config.h"

// #define bzero(b,len) (memset((b), '\0', (len)), (void) 0)

/* check if soundcard header file is available, if not disable DSP and Mixer */
#if !defined(HAVE_LINUX_SOUNDCARD_H) && !defined(HAVE_SYS_SOUNDCARD_H)
#undef WITH_DSP
#undef WITH_MIXER
#define WITH_DSP 0
#define WITH_MIXER 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Keep old portability fallbacks while CMake supplies the feature defines. */
#if STDC_HEADERS
#include <string.h>
#else
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif
char *strchr(), *strrchr();
#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#define memmove(d, s, n) bcopy((s), (d), (n))
#endif
#endif

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#if HAVE_ENDIAN_H
#include <endian.h>
#else
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __PDP_ENDIAN 3412
#ifdef WORDS_BIGENDIAN
#define __BYTE_ORDER __BIG_ENDIAN
#else
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#define BYTE_ORDER __BYTE_ORDER
#endif

/*
 * variables
 */
extern int cthugha_close; /* cthugha is closing now */

char* cthugha_mode_text();

// Initialize the selected graphical frontend when display startup is reached.
int cth_init(int* argc, char* argv[]);
int cth_main();

enum CthughaLogLevel {
    CTH_LOG_ERROR = 0,
    CTH_LOG_WARN = 1,
    CTH_LOG_INFO = 2,
    CTH_LOG_DEBUG = 5,
    CTH_LOG_TRACE = 10
};

int printfv(int lvl, const char* fmt, ...); // print verbose message
int cth_log_enabled(int lvl); // true if named-level log message would be printed
int cth_log(int lvl, const char* fmt, ...); // print named-level log message
int cth_log_context(int lvl, const char* context, const char* fmt, ...); // print contextual log message
int cth_log_error(const char* fmt, ...); // print error log message
int cth_log_errno(int errnum, const char* fmt, ...); // print error log message with given errno

#define CTH_LOG_ENABLED(lvl) (cth_log_enabled(lvl))
#define CTH_LOG(lvl, args...) \
    do { \
        if (CTH_LOG_ENABLED(lvl)) \
            cth_log(lvl, args); \
    } while (0)
#define CTH_ERROR(args...) cth_log_error(args)
#define CTH_ERRNO(errnum, args...) cth_log_errno(errnum, args)
#define CTH_WARN(args...) CTH_LOG(CTH_LOG_WARN, args)
#define CTH_INFO(args...) CTH_LOG(CTH_LOG_INFO, args)
#define CTH_DEBUG(args...) CTH_LOG(CTH_LOG_DEBUG, args)
#define CTH_TRACE(fmt, context, args...) \
    do { \
        if (CTH_LOG_ENABLED(CTH_LOG_TRACE)) \
            cth_log_context(CTH_LOG_TRACE, context, fmt, ##args); \
    } while (0)

int systemf(const char* fmt, ...); // combined sprintf and system

#ifdef __cplusplus

inline int fclose0(FILE*& stream) {
    int ret = fclose(stream);
    stream = NULL;
    return ret;
}

#endif

int gettime();
double getTime(); // return time in seconds

#ifdef __cplusplus

class xy {
public:
    int x, y;
    xy() { }
    xy(int X, int Y)
        : x(X)
        , y(Y) { }
};

#endif

#endif
