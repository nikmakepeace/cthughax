// Platform lifecycle hooks.
//
// The application owns frame boundaries and resource state.  This layer only
// translates platform-specific lifecycle requests, such as POSIX job-control
// suspend, into safe application callbacks.

#ifndef __PLATFORM_LIFECYCLE_H
#define __PLATFORM_LIFECYCLE_H

#include <memory>

class LogSink;
class PlatformSuspendSignalState;

struct PlatformLifecycleCallbacks {
    /** Called at a safe frame boundary immediately before process suspension. */
    void (*willSuspend)(void*);

    /** Called after the process resumes and before frame processing continues. */
    void (*didResume)(void*);

    /** Opaque pointer passed to callbacks, usually Application*. */
    void* context;

    PlatformLifecycleCallbacks()
        : willSuspend(0)
        , didResume(0)
        , context(0) { }

    /**
     * Creates lifecycle callbacks.
     *
     * @param willSuspend_ Callback run before process suspension.
     * @param didResume_ Callback run after process resume.
     * @param context_ Opaque context passed to both callbacks.
     */
    PlatformLifecycleCallbacks(void (*willSuspend_)(void*),
        void (*didResume_)(void*), void* context_)
        : willSuspend(willSuspend_)
        , didResume(didResume_)
        , context(context_) { }
};

class PlatformLifecycle {
    PlatformLifecycleCallbacks callbacks;
    LogSink& log;
    std::unique_ptr<PlatformSuspendSignalState> signalState;

public:
    /**
     * Creates platform lifecycle integration.
     *
     * @param log_ Sink for lifecycle diagnostics. The referenced object must
     *        outlive this lifecycle object.
     * @param callbacks_ Application callbacks invoked at safe lifecycle
     *        boundaries.
     */
    explicit PlatformLifecycle(LogSink& log_,
        const PlatformLifecycleCallbacks& callbacks_ =
            PlatformLifecycleCallbacks());

    /** Restores installed platform hooks. */
    ~PlatformLifecycle();

    /** Installs supported platform hooks, such as POSIX SIGTSTP handling. */
    void install();

    /** Removes installed platform hooks. Safe to call more than once. */
    void shutdown();

    /** Requests a suspend to be serviced at the next frame boundary. */
    void requestSuspend();

    /**
     * Services pending lifecycle work between frame stages.
     *
     * Application calls this after runFrame() has finished mutating audio,
     * video, and display state so suspend/resume callbacks cannot interrupt
     * partially updated runtime objects.
     */
    void serviceFrameBoundary();
};

#endif
