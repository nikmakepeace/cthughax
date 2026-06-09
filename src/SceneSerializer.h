// Runtime configuration contributor for current Scene selections.

#ifndef CTHUGHA_SCENE_SERIALIZER_H
#define CTHUGHA_SCENE_SERIALIZER_H

#include "RuntimeConfigRegistry.h"

class Scene;

/**
 * Writes the current Scene selection state into a runtime configuration snapshot.
 */
class SceneSerializer : public RuntimeConfigContributor {
    const Scene& scene;

public:
    explicit SceneSerializer(const Scene& scene_);

    virtual void contribute(Config& config) const;
};

#endif
