/** @file
 * Composes an indexed display frame with the selected screen renderer.
 */

#ifndef __PRESENTATION_COMPOSER_H
#define __PRESENTATION_COMPOSER_H

#include "ScreenRenderContext.h"
#include "cthugha.h"

class IndexedDisplayFrame;
class IndexedFrame;
class ScreenEntry;
class FrameRenderContext;

/** Observer notified when composed presentation storage or geometry changes. */
class PresentationFrameObserver {
public:
    /** Releases the observer interface. */
    virtual ~PresentationFrameObserver() { }

    /**
     * Called before indexed pixels move to a larger allocation.
     *
     * @param oldPixels Previous pixel allocation that is about to be replaced.
     */
    virtual void indexedPixelsWillMove(unsigned char* oldPixels) = 0;

    /** Called after composed indexed-frame geometry changes. */
    virtual void indexedFrameGeometryChanged() = 0;
};

/** Supplies the currently selected presentation screen entry. */
class PresentationScreenSelection {
public:
    /** Releases the selection interface. */
    virtual ~PresentationScreenSelection() { }

    /** @return Currently selected screen entry, or NULL. */
    virtual ScreenEntry* current() = 0;
};

/** Stateful composer for classic screen/presentation effects. */
class PresentationComposer {
    ScreenEntry* renderedScreenValue;
    ScreenEntry* lastSuccessfulScreenValue;

public:
    /** Creates an empty composer with no previous successful screen. */
    PresentationComposer();

    /**
     * Renders source through the current screen selection into destination.
     *
     * @param source Indexed source frame.
     * @param destination Destination storage reused across calls.
     * @param selection Current screen selector.
     * @param frameTimeSeconds Visual-frame timestamp in seconds.
     * @param deltaTimeSeconds Seconds since the previous visual frame.
     * @param framesPerSecond Current presentation-frame rate estimate.
     * @param observer Optional storage/geometry observer.
     * @param frameContext Optional borrowed audio context for audio-reactive
     *        screen renderers.
     * @return Destination frame after successful composition, or reset frame on
     *         failure/invalid source.
     */
    const IndexedDisplayFrame& compose(const IndexedFrame& source,
        IndexedDisplayFrame& destination, PresentationScreenSelection& selection,
        double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond,
        PresentationFrameObserver* observer = 0,
        const FrameRenderContext* frameContext = 0);

    /** @return Screen entry used by the last successful compose call. */
    ScreenEntry* renderedScreen() const;
};

#endif
