/** @file
 * Concrete runtime command mediator.
 */

#ifndef __RUNTIME_CHANGE_MEDIATOR_H
#define __RUNTIME_CHANGE_MEDIATOR_H

#include "RuntimeCommandSink.h"

class RuntimePersistence;
class RuntimeShutdown;
class SceneCommands;

class RuntimeChangeMediator : public RuntimeCommandSink {
    SceneCommands& sceneCommands;
    RuntimePersistence& runtimePersistence;
    RuntimeShutdown& runtimeShutdown;

    RuntimeChangeSet applySceneBy(RuntimeSceneTarget target, int by);
    RuntimeChangeSet applySceneTo(RuntimeSceneTarget target, const char* to);

public:
    /**
     * Creates a mediator for live runtime commands.
     *
     * @param sceneCommands_ Scene command facade used for scene changes.
     * @param runtimePersistence_ Persistence port used for runtime saves.
     * @param runtimeShutdown_ Shutdown port used for close requests.
     */
    RuntimeChangeMediator(SceneCommands& sceneCommands_,
        RuntimePersistence& runtimePersistence_,
        RuntimeShutdown& runtimeShutdown_);

    /**
     * Applies a runtime command and reports the areas changed by it.
     *
     * @param command Runtime command to execute.
     * @return Change flags describing the command's visible effects.
     */
    virtual RuntimeChangeSet apply(const RuntimeCommand& command);
};

#endif
