#ifndef __SCREEN_H
#define __SCREEN_H

#include "cthugha.h"
#include "EffectControl.h"

class ScreenRenderContext;

/**
 * Catalog entry and executor for one classic screen/presentation effect.
 *
 * Screen effects consume the current source IndexedFrame and destination
 * IndexedDisplayFrame through an explicit ScreenRenderContext.
 */
class ScreenEntry : public EffectChoice {
public:
    /** Screen renderer function. Returns nonzero when it wants another choice. */
    typedef int (*Function)(ScreenRenderContext&);

private:
    Function functionValue;
    xy filledScaleValue;
    xy outputScaleValue;

public:
    /**
     * Creates a screen-effect catalog entry.
     *
     * @param function Renderer function, or NULL for a no-op entry.
     * @param name Short option/UI name.
     * @param description Human-readable description.
     * @param filledScale Integer multiplier from source frame size to the
     *        region the renderer fills before mirror completion.
     * @param inUse Nonzero when the option can be selected.
     */
    ScreenEntry(Function function, const char* name, const char* description,
        xy filledScale, int inUse = 1);

    /**
     * Creates a screen-effect catalog entry with explicit output scaling.
     *
     * @param outputScale Integer multiplier from source frame size to indexed
     *        display-frame size. {1,1} means source-sized output.
     */
    ScreenEntry(Function function, const char* name, const char* description,
        xy filledScale, xy outputScale, int inUse = 1);

    /** Executes the renderer with explicit source, destination, and timing. */
    int render(ScreenRenderContext& context);

    /** @return Integer multiplier from source frame size to filled region. */
    xy filledScale() const;

    /** @return Pixel size of the region this renderer fills. */
    xy filledOutputSize(int sourceWidth, int sourceHeight) const;

    /**
     * @return Historical output geometry used by the classic screen catalog.
     *
     * Today every built-in screen effect uses a 2x indexed display frame.
     */
    static xy compatibilityOutputSize(int sourceWidth, int sourceHeight) {
        return xy(2 * sourceWidth, 2 * sourceHeight);
    }

    /**
     * @param outputScale Integer multiplier from source frame size.
     * @return Output geometry for the requested scale.
     */
    static xy scaledOutputSize(int sourceWidth, int sourceHeight, xy outputScale) {
        return xy(sourceWidth * outputScale.x, sourceHeight * outputScale.y);
    }

    /**
     * @return Indexed output geometry this screen effect wants to produce.
     *
     * Future entries can override this to request 1x, 4x, cropped, or otherwise
     * pragmatic display-frame sizes without changing display backends.
     */
    virtual xy outputSize(int sourceWidth, int sourceHeight) const;
};

/** Built-in screen/presentation effect catalog used by the screen option. */
extern ScreenEntry screenCatalog[];

/** Number of entries in screenCatalog. */
extern const int nScreenCatalogEntries;

/**
 * @param index Catalog index.
 * @return Screen entry at index, or NULL when out of range.
 */
ScreenEntry* screenByIndex(int index);

/** EffectChoice view of screenCatalog, used by the option system. */
extern EffectChoiceList screenEntries;

/** Selected screen/presentation effect. */
extern EffectControl screen;

#endif
