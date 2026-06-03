#include "PresentationComposer.h"

#include "FrameCompletion.h"
#include "IndexedDisplayFrame.h"
#include "IndexedFrame.h"
#include "Screen.h"

static void prepareDestination(IndexedDisplayFrame& destination, int width,
    int height, FramePalette* framePalette, PresentationFrameObserver* observer) {
    if (width <= 0 || height <= 0) {
        destination.reset();
        return;
    }

    unsigned char* oldPixels = destination.pixels();
    int oldWidth = destination.width();
    int oldHeight = destination.height();
    int oldPitch = destination.pitch();
    int requiredByteCount = width * height;

    if (observer != 0 && requiredByteCount > destination.capacityByteCount())
        observer->indexedPixelsWillMove(oldPixels);

    destination.resize(width, height);
    destination.setFramePalette(framePalette);

    if (observer != 0
        && (destination.pixels() != oldPixels || destination.width() != oldWidth
            || destination.height() != oldHeight
            || destination.pitch() != oldPitch))
        observer->indexedFrameGeometryChanged();
}

PresentationComposer::PresentationComposer()
    : renderedScreenValue(0) {
}

const IndexedDisplayFrame& PresentationComposer::compose(const IndexedFrame& source,
    IndexedDisplayFrame& destination, PresentationScreenSelection& selection,
    double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond,
    PresentationFrameObserver* observer) {
    renderedScreenValue = selection.current();
    if (!source.valid() || renderedScreenValue == 0) {
        destination.reset();
        return destination;
    }

    int keepRendering = 1;
    while (keepRendering && renderedScreenValue != 0) {
        xy outputSize = renderedScreenValue->outputSize(source.width, source.height);
        prepareDestination(destination, outputSize.x, outputSize.y,
            source.framePalette, observer);

        ScreenRenderContext context(source, destination, frameTimeSeconds,
            deltaTimeSeconds, framesPerSecond, &selection);
        keepRendering = renderedScreenValue->render(context);
        if (keepRendering) {
            ScreenEntry* nextScreen = selection.current();
            if (nextScreen == renderedScreenValue)
                keepRendering = 0;
            renderedScreenValue = nextScreen;
        }
    }

    if (renderedScreenValue != 0 && destination.valid()) {
        xy filledSize = renderedScreenValue->filledOutputSize(source.width,
            source.height);
        FrameCompletion::completeMirrored(destination, filledSize.x,
            filledSize.y);
    }

    return destination;
}

ScreenEntry* PresentationComposer::renderedScreen() const {
    return renderedScreenValue;
}
