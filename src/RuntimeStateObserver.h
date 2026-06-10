/** @file
 * Observer for externally visible runtime state changes.
 */

#ifndef CTHUGHA_RUNTIME_STATE_OBSERVER_H
#define CTHUGHA_RUNTIME_STATE_OBSERVER_H

class RuntimeStateObserver {
public:
    virtual ~RuntimeStateObserver() { }

    /** Reports that canonical runtime state should be republished. */
    virtual void runtimeStateChanged() = 0;
};

#endif
