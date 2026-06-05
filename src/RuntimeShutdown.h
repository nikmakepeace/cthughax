/** @file
 * Runtime shutdown request port and legacy close-flag implementation.
 */

#ifndef CTHUGHA_RUNTIME_SHUTDOWN_H
#define CTHUGHA_RUNTIME_SHUTDOWN_H

/** Handles runtime requests to close the application. */
class RuntimeShutdown {
public:
    /** Destroys the shutdown request interface. */
    virtual ~RuntimeShutdown() { }

    /** Requests application shutdown. */
    virtual void requestClose() = 0;
};

/** Runtime shutdown implementation backed by the legacy close flag. */
class CthughaRuntimeShutdown : public RuntimeShutdown {
public:
    /** Requests shutdown by incrementing the legacy close flag. */
    virtual void requestClose();
};

#endif
