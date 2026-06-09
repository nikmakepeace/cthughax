/** @file
 * Legacy C logging adapter for CTH_* macros.
 */

#include "ProcessServices.h"
#include "cthugha.h"

#include <stdarg.h>

static LoggingRuntime* cthugha_logging_runtime = NULL;

void cthugha_install_logging_runtime(LoggingRuntime& runtime) {
    cthugha_logging_runtime = &runtime;
}

void cthugha_clear_logging_runtime(LoggingRuntime& runtime) {
    if (cthugha_logging_runtime == &runtime)
        cthugha_logging_runtime = NULL;
}

static int logging_enabled(int lvl) {
    if (cthugha_logging_runtime == NULL)
        return lvl <= CTH_LOG_ERROR;
    return cthugha_logging_runtime->enabled(lvl);
}

static int verbose_enabled(int lvl) {
    if (cthugha_logging_runtime == NULL)
        return lvl <= 0;
    return lvl <= cthugha_logging_runtime->verbosity();
}

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
    if (verbose_enabled(lvl)) {
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
        // Trace/debug logging can be extremely hot and may run from audio
        // callbacks. Flushing each line makes diagnostics perturb frame and
        // audio timing, especially when stdout is captured by a debugger.
        if (lvl <= CTH_LOG_INFO)
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

    fprintf(stderr, " (%d - %s)\n\r", errnum, strerror(errnum));

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
    if (verbose_enabled(lvl))
        printf(fmt);
#endif
    return 0;
}

//
// print a named-level log message
//
int cth_log_enabled(int lvl) {
    return logging_enabled(lvl);
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
    else if (logging_enabled(lvl))
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
        else if (logging_enabled(lvl))
            printf("%s: ", context);
    }
    if (lvl <= CTH_LOG_ERROR)
        fprintf(stderr, fmt);
    else if (logging_enabled(lvl))
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
