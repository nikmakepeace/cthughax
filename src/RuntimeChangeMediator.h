/** @file
 * Concrete runtime command mediator.
 */

#ifndef __RUNTIME_CHANGE_MEDIATOR_H
#define __RUNTIME_CHANGE_MEDIATOR_H

#include "RuntimeCommandSink.h"

class RuntimePersistence;
class RuntimeShutdown;
class RuntimeDisplayControls;
class RuntimeAudioControls;
class RuntimeAutoChangeControls;
class RuntimeEffectControls;
class SceneCommandTarget;

class RuntimeChangeMediator : public RuntimeCommandSink {
    SceneCommandTarget& sceneCommands;
    RuntimePersistence& runtimePersistence;
    RuntimeShutdown& runtimeShutdown;
    RuntimeDisplayControls& displayControls;
    RuntimeAudioControls& audioControls;
    RuntimeAutoChangeControls& autoChangeControls;
    RuntimeEffectControls& effectControls;

    RuntimeChangeSet applySceneBy(RuntimeSceneTarget target, int by);
    RuntimeChangeSet applySceneTo(RuntimeSceneTarget target, const char* to);

public:
    /**
     * Creates a mediator for live runtime commands.
     *
     * @param sceneCommands_ Scene command target used for scene changes.
     * @param runtimePersistence_ Persistence port used for runtime saves.
     * @param runtimeShutdown_ Shutdown port used for close requests.
     * @param displayControls_ Display/presentation control port.
     * @param audioControls_ Audio control port.
     * @param autoChangeControls_ Automatic scene-change control port.
     * @param effectControls_ Legacy EffectControl state port.
     */
    RuntimeChangeMediator(SceneCommandTarget& sceneCommands_,
        RuntimePersistence& runtimePersistence_,
        RuntimeShutdown& runtimeShutdown_,
        RuntimeDisplayControls& displayControls_,
        RuntimeAudioControls& audioControls_,
        RuntimeAutoChangeControls& autoChangeControls_,
        RuntimeEffectControls& effectControls_);

    /**
     * Applies a runtime command and reports the areas changed by it.
     *
     * @param command Runtime command to execute.
     * @return Change flags describing the command's visible effects.
     */
    virtual RuntimeChangeSet apply(const RuntimeCommand& command);
};

#endif
