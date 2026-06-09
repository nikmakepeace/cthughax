// Scene frame geometry port.

#ifndef CTHUGHA_SCENE_GEOMETRY_H
#define CTHUGHA_SCENE_GEOMETRY_H

/**
 * Provides frame dimensions needed to resolve Scene wave/image selections.
 */
class SceneGeometry {
public:
    virtual ~SceneGeometry();
    virtual int width() const = 0;
    virtual int height() const = 0;
};

#endif
