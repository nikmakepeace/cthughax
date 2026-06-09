/** @file
 * Runtime shutdown request port and close-state implementation.
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

    /**
     * Reports whether shutdown has been requested.
     *
     * @return True once requestClose() has been called.
     */
    virtual bool closeRequested() const = 0;
};

/** Application-owned close request state. */
class RuntimeCloseState : public RuntimeShutdown {
    bool closeRequestedValue;

public:
    /** Creates a close state with no pending close request. */
    RuntimeCloseState();

    /** Records an application shutdown request. */
    virtual void requestClose();

    /**
     * Reports whether shutdown has been requested.
     *
     * @return True once requestClose() has been called.
     */
    virtual bool closeRequested() const;
};

#endif
