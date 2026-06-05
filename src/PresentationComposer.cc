#include "PresentationComposer.h"

#include "FrameCompletion.h"
#include "IndexedDisplayFrame.h"
#include "IndexedFrame.h"
#include "Screen.h"
#include "VideoFilterchain.h"

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

static int screenAlreadyAttempted(ScreenEntry* const* attempted, int count,
    ScreenEntry* screen) {
    for (int i = 0; i < count; ++i)
        if (attempted[i] == screen)
            return 1;

    return 0;
}

static int renderWithScreen(ScreenEntry& screen, const IndexedFrame& source,
    IndexedDisplayFrame& destination, double frameTimeSeconds,
    double deltaTimeSeconds, double framesPerSecond,
    PresentationFrameObserver* observer, const VideoFrameContext* frameContext) {
    xy outputSize = screen.outputSize(source.width, source.height);
    prepareDestination(destination, outputSize.x, outputSize.y,
        source.framePalette, observer);

    ScreenRenderContext context = frameContext != 0
        ? ScreenRenderContext(source, destination, frameTimeSeconds,
              deltaTimeSeconds, framesPerSecond, *frameContext)
        : ScreenRenderContext(source, destination, frameTimeSeconds,
              deltaTimeSeconds, framesPerSecond);
    return screen.render(context);
}

PresentationComposer::PresentationComposer()
    : renderedScreenValue(0)
    , lastSuccessfulScreenValue(0) {
}

const IndexedDisplayFrame& PresentationComposer::compose(const IndexedFrame& source,
    IndexedDisplayFrame& destination, PresentationScreenSelection& selection,
    double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond,
    PresentationFrameObserver* observer, const VideoFrameContext* frameContext) {
    renderedScreenValue = 0;
    ScreenEntry* requestedScreen = selection.current();
    if (!source.valid() || requestedScreen == 0) {
        destination.reset();
        return destination;
    }

    ScreenEntry* safeFallbackScreen = screenByIndex(0);
    ScreenEntry* candidates[] = {
        requestedScreen,
        lastSuccessfulScreenValue,
        safeFallbackScreen
    };
    ScreenEntry* attempted[3] = { 0, 0, 0 };
    int attemptedCount = 0;

    for (int i = 0; i < int(sizeof(candidates) / sizeof(candidates[0])); ++i) {
        ScreenEntry* candidate = candidates[i];
        if (candidate == 0)
            continue;
        if (screenAlreadyAttempted(attempted, attemptedCount, candidate))
            continue;

        attempted[attemptedCount++] = candidate;
        if (renderWithScreen(*candidate, source, destination, frameTimeSeconds,
                deltaTimeSeconds, framesPerSecond, observer, frameContext)
            == 0) {
            renderedScreenValue = candidate;
            lastSuccessfulScreenValue = candidate;
            break;
        }
    }

    if (renderedScreenValue != 0 && destination.valid()) {
        xy filledSize = renderedScreenValue->filledOutputSize(source.width,
            source.height);
        FrameCompletion::completeMirrored(destination, filledSize.x,
            filledSize.y);
    } else {
        destination.reset();
    }

    return destination;
}

ScreenEntry* PresentationComposer::renderedScreen() const {
    return renderedScreenValue;
}
