/** @file
 * Read-only status seam for automatic scene-change state.
 */

#ifndef CTHUGHA_SCENE_CHANGE_STATUS_PROVIDER_H
#define CTHUGHA_SCENE_CHANGE_STATUS_PROVIDER_H

/**
 * Provides short status text for the active automatic scene-change scheduler.
 *
 * The interface lets UI code display auto-change progress without owning or
 * reaching into the policy object that performs automatic runtime commands.
 */
class SceneChangeStatusProvider {
public:
    /** Releases the status provider interface. */
    virtual ~SceneChangeStatusProvider() { }

    /**
     * Returns current automatic scene-change status text.
     *
     * @return Pointer to provider-owned text valid until the next status call.
     */
    virtual const char* sceneChangeStatus() const = 0;
};

#endif
