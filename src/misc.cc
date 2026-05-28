#include "cthugha.h"
#include "information.h" /* title, credits, ... */
#include "display.h"
#include "translate.h"
#include "options.h"
#include "keys.h"
#include "imath.h"
#include "waves.h"
#include "Option.h"
#include "AudioProcessor.h"
#include "AutoChanger.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "CDPlayer.h"

#include <errno.h>

#include <stdarg.h>

//
// usefull to find memory errors
//
#if 0
void * operator new(size_t s) {
    void * r = malloc(s);
    printf("new %d: %x\n", s, r);
    return r;
}
void operator delete(void *d) {
    printf("delete  %x\n", d);
    if(d != NULL)
	free(d);
}
#endif

OptionInt cthugha_verbose("verbose", 1, 0, -1); // verbosity level (no max, min=-1)

static void copy_console_format(char* out, const char* in) {
    int i;
    for (i = 0; *in != '\0'; in++) {
        out[i++] = *in;
        if (*in == '\n')
            out[i++] = '\r';
    }
    out[i] = '\0';
}

static int vprintfv(int lvl, const char* fmt, va_list ap) {
    if (lvl <= int(cthugha_verbose)) {
        // I had problems with missing carrige returns on Linux console
        // so i translate it to \n\r
        char fmt_r[2 * strlen(fmt) + 1];
        copy_console_format(fmt_r, fmt);

        //	printf("%5d:", getpid());

#ifdef HAVE_VPRINTF
        vprintf(fmt_r, ap);
#else
        printf(fmt_r);
#endif
        fflush(stdout);
    }
    return 0;
}

static int vprintfe(const char* fmt, va_list ap) {
    printf("\n\r");
    fflush(stdout);

    // I had problems with missing carrige returns on Linux console
    // so i translate it to \n\r
    char fmt_r[2 * strlen(fmt) + 1];
    copy_console_format(fmt_r, fmt);

#ifdef HAVE_VPRINTF
    vfprintf(stderr, fmt_r, ap);
#else
    fprintf(stderr, fmt);
#endif

    fflush(stderr);

    return 0;
}

static int vprintfee(int errnum, const char* fmt, va_list ap) {
    printf("\n\r");
    fflush(stdout);

    // I had problems with missing carrige returns on Linux console
    // so i translate it to \n\r
    char fmt_r[2 * strlen(fmt) + 1];
    copy_console_format(fmt_r, fmt);

#ifdef HAVE_VPRINTF
    vfprintf(stderr, fmt_r, ap);
#else
    fprintf(stderr, fmt_r);
#endif

#ifdef HAVE_STRERROR
    fprintf(stderr, " (%d - %s)\n\r", errnum, strerror(errnum));
#else
    fprintf(stderr, " (%d)\n\r", errnum);
#endif

    fflush(stderr);

    return 0;
}

//
// print a verbose message
//
int printfv(int lvl, const char* fmt, ...) {
#ifdef HAVE_VPRINTF
    va_list ap;
    va_start(ap, fmt);
    vprintfv(lvl, fmt, ap);
    va_end(ap);
#else
    if (lvl <= int(cthugha_verbose))
        printf(fmt);
#endif
    return 0;
}

//
// print a named-level log message
//
int cth_log_enabled(int lvl) {
    return (lvl <= CTH_LOG_ERROR) || (lvl <= int(cthugha_verbose));
}

int cth_log(int lvl, const char* fmt, ...) {
#ifdef HAVE_VPRINTF
    va_list ap;
    va_start(ap, fmt);
    if (lvl <= CTH_LOG_ERROR)
        vprintfe(fmt, ap);
    else
        vprintfv(lvl, fmt, ap);
    va_end(ap);
#else
    if (lvl <= CTH_LOG_ERROR)
        fprintf(stderr, fmt);
    else if (lvl <= int(cthugha_verbose))
        printf(fmt);
#endif
    return 0;
}

int cth_log_context(int lvl, const char* context, const char* fmt, ...) {
    if (!cth_log_enabled(lvl))
        return 0;

#ifdef HAVE_VPRINTF
    va_list ap;
    va_start(ap, fmt);
    if (context && context[0]) {
        if (lvl <= CTH_LOG_ERROR)
            fprintf(stderr, "%s: ", context);
        else
            printfv(lvl, "%s: ", context);
    }
    if (lvl <= CTH_LOG_ERROR)
        vprintfe(fmt, ap);
    else
        vprintfv(lvl, fmt, ap);
    va_end(ap);
#else
    if (context && context[0]) {
        if (lvl <= CTH_LOG_ERROR)
            fprintf(stderr, "%s: ", context);
        else if (lvl <= int(cthugha_verbose))
            printf("%s: ", context);
    }
    if (lvl <= CTH_LOG_ERROR)
        fprintf(stderr, fmt);
    else if (lvl <= int(cthugha_verbose))
        printf(fmt);
#endif
    return 0;
}

//
// print a named-level error message and a given error number
//
int cth_log_errno(int errnum, const char* fmt, ...) {

#ifdef HAVE_VPRINTF
    va_list ap;
    va_start(ap, fmt);
    vprintfee(errnum, fmt, ap);
    va_end(ap);
#else
    fprintf(stderr, fmt);
    fprintf(stderr, " (%d)\n\r", errnum);
#endif

    return 0;
}

//
// print a named-level error message
//
int cth_log_error(const char* fmt, ...) {

#ifdef HAVE_VPRINTF
    va_list ap;
    va_start(ap, fmt);
    vprintfe(fmt, ap);
    va_end(ap);
#else
    fprintf(stderr, fmt);
#endif

    return 0;
}

//
// combine sprintf and system
//
int systemf(const char* fmt, ...) {

#ifdef HAVE_VPRINTF
    char cmd[6 * PATH_MAX];

    va_list ap;
    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    va_end(ap);

    return system(cmd);
#else
#error 'vprintf' is not available. Some parts of Cthugha will not work!
    return -1;
#endif
}

int cthugha_close = 0; // closing right now
int cthugha_pause = 0; // going to pause (^Z)

/*
 * get the 1/100 sec since program start
 */
int gettime() {
    struct timeval tv;
    static int starttime = 0;

    gettimeofday(&tv, NULL);

    if (starttime == 0) {
        starttime = tv.tv_sec;
    }
    tv.tv_sec -= starttime;

    return tv.tv_sec * 100L + tv.tv_usec / 10000L;
}
double getTime() {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return double(tv.tv_sec) + 1e-6 * double(tv.tv_usec);
}
