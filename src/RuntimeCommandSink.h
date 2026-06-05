/** @file
 * Runtime command sink contract and change reporting.
 */

#ifndef __RUNTIME_COMMAND_SINK_H
#define __RUNTIME_COMMAND_SINK_H

#include "RuntimeCommand.h"

struct RuntimeChangeSet {
    unsigned int sceneChanges;
    int displayChanged;
    int audioProcessingChanged;
    int autoChangeChanged;
    int persistenceRequested;
    int fpsChanged;
    int closeRequested;
    int uiChanged;

    /** Creates an empty change set. */
    RuntimeChangeSet();

    /**
     * Reports whether any change flag is set.
     *
     * @return Non-zero when the set contains any change.
     */
    int any() const;

    /**
     * Merges another change set into this set.
     *
     * @param other Change set to merge.
     */
    void merge(const RuntimeChangeSet& other);
};

/** Interface for objects that execute RuntimeCommand values. */
class RuntimeCommandSink {
public:
    /** Destroys the runtime command sink interface. */
    virtual ~RuntimeCommandSink() { }

    /**
     * Applies a runtime command.
     *
     * @param command Command to execute.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet apply(const RuntimeCommand& command) = 0;
};

#endif
