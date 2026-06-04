// Runtime command sink contract and change reporting.

#ifndef __RUNTIME_COMMAND_SINK_H
#define __RUNTIME_COMMAND_SINK_H

#include "RuntimeCommand.h"

struct RuntimeChangeSet {
    unsigned int sceneChanges;
    int displayChanged;
    int audioProcessingChanged;
    int autoChangeChanged;
    int audioResetRequested;
    int persistenceRequested;
    int fpsChanged;
    int closeRequested;
    int uiChanged;

    RuntimeChangeSet();
    int any() const;
    void merge(const RuntimeChangeSet& other);
};

class RuntimeCommandSink {
public:
    virtual ~RuntimeCommandSink() { }
    virtual RuntimeChangeSet apply(const RuntimeCommand& command) = 0;
};

#endif
