// Platform lifecycle hooks.

#include "PlatformLifecycle.h"

#include "ProcessServices.h"

#include <signal.h>

class PlatformSuspendSignalState {
#if defined(SIGTSTP) && !defined(_WIN32)
    int handlerInstalledValue;
    int previousActionValid;
    struct sigaction previousAction;
#endif
    volatile sig_atomic_t requestPending;

public:
    PlatformSuspendSignalState();
    ~PlatformSuspendSignalState();

    int install(LogSink& log);
    void shutdown();
    void request();
    int consumeRequest();
    void suspendCurrentProcess(LogSink& log);
};

#if defined(SIGTSTP) && !defined(_WIN32)
#define CTH_HAVE_JOB_CONTROL 1
#include <string.h>

static PlatformSuspendSignalState* activeSuspendSignalState = NULL;

static void suspendSignalHandler(int) {
    if (activeSuspendSignalState != NULL)
        activeSuspendSignalState->request();
}

PlatformSuspendSignalState::PlatformSuspendSignalState()
    : handlerInstalledValue(0)
    , previousActionValid(0)
    , requestPending(0) {
    memset(&previousAction, 0, sizeof(previousAction));
}

PlatformSuspendSignalState::~PlatformSuspendSignalState() {
    shutdown();
}

int PlatformSuspendSignalState::install(LogSink& log) {
    if (handlerInstalledValue)
        return 1;

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    action.sa_handler = suspendSignalHandler;

    if (!previousActionValid) {
        struct sigaction currentAction;
        if (sigaction(SIGTSTP, 0, &currentAction) != 0) {
            log.warn("Could not inspect SIGTSTP handler; terminal suspend disabled.\n");
            return 0;
        }

        if (currentAction.sa_handler == SIG_IGN)
            return 0;

        if (sigaction(SIGTSTP, &action, &previousAction) != 0) {
            log.warn("Could not install SIGTSTP handler; terminal suspend disabled.\n");
            return 0;
        }

        previousActionValid = 1;
    } else {
        if (sigaction(SIGTSTP, &action, 0) != 0) {
            log.warn("Could not reinstall SIGTSTP handler; terminal suspend disabled.\n");
            return 0;
        }
    }

    activeSuspendSignalState = this;
    handlerInstalledValue = 1;
    return 1;
}

void PlatformSuspendSignalState::shutdown() {
    if (!handlerInstalledValue)
        return;

    if (previousActionValid)
        sigaction(SIGTSTP, &previousAction, 0);

    if (activeSuspendSignalState == this)
        activeSuspendSignalState = NULL;
    previousActionValid = 0;
    handlerInstalledValue = 0;
}

void PlatformSuspendSignalState::request() {
    requestPending = 1;
}

int PlatformSuspendSignalState::consumeRequest() {
    if (!requestPending)
        return 0;

    requestPending = 0;
    return 1;
}

static void useDefaultSuspendSignalHandler(PlatformSuspendSignalState* state) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    action.sa_handler = SIG_DFL;
    sigaction(SIGTSTP, &action, 0);
    if (activeSuspendSignalState == state)
        activeSuspendSignalState = NULL;
}

void PlatformSuspendSignalState::suspendCurrentProcess(LogSink& log) {
    handlerInstalledValue = 0;
    useDefaultSuspendSignalHandler(this);
    raise(SIGTSTP);
    install(log);
}
#else
#define CTH_HAVE_JOB_CONTROL 0

PlatformSuspendSignalState::PlatformSuspendSignalState()
    : requestPending(0) { }

PlatformSuspendSignalState::~PlatformSuspendSignalState() { }

int PlatformSuspendSignalState::install(LogSink&) {
    return 0;
}

void PlatformSuspendSignalState::shutdown() { }

void PlatformSuspendSignalState::request() {
    requestPending = 1;
}

int PlatformSuspendSignalState::consumeRequest() {
    if (!requestPending)
        return 0;

    requestPending = 0;
    return 1;
}

void PlatformSuspendSignalState::suspendCurrentProcess(LogSink&) { }
#endif

PlatformLifecycle::PlatformLifecycle(LogSink& log_,
    const PlatformLifecycleCallbacks& callbacks_)
    : callbacks(callbacks_)
    , log(log_)
    , signalState(new PlatformSuspendSignalState) { }

PlatformLifecycle::~PlatformLifecycle() {
    shutdown();
}

void PlatformLifecycle::install() {
    signalState->install(log);
}

void PlatformLifecycle::shutdown() {
    signalState->shutdown();
}

void PlatformLifecycle::requestSuspend() {
    signalState->request();
}

void PlatformLifecycle::serviceFrameBoundary() {
    if (!signalState->consumeRequest())
        return;

    if (!CTH_HAVE_JOB_CONTROL) {
        log.debug("Suspend requested, but this platform has no active job-control suspend hook.\n");
        return;
    }

#if CTH_HAVE_JOB_CONTROL
    log.info("Stopping...\n");
    if (callbacks.willSuspend != 0)
        callbacks.willSuspend(callbacks.context);

    signalState->suspendCurrentProcess(log);

    log.info("Continuing...\n");
    if (callbacks.didResume != 0)
        callbacks.didResume(callbacks.context);
#endif
}
