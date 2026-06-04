// Concrete runtime command mediator.

#ifndef __RUNTIME_CHANGE_MEDIATOR_H
#define __RUNTIME_CHANGE_MEDIATOR_H

#include "RuntimeCommandSink.h"

class SceneCommands;

class RuntimeChangeMediator : public RuntimeCommandSink {
    SceneCommands& sceneCommands;

    RuntimeChangeSet applySceneBy(RuntimeSceneTarget target, int by);
    RuntimeChangeSet applySceneTo(RuntimeSceneTarget target, const char* to);

public:
    RuntimeChangeMediator(SceneCommands& sceneCommands_);

    virtual RuntimeChangeSet apply(const RuntimeCommand& command);
};

#endif
