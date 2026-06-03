#include "ScreenRenderContext.h"

static ScreenRenderContext* currentContext = 0;

ScreenRenderContext::ScreenRenderContext(const IndexedFrame& source,
    IndexedDisplayFrame& destination, double frameTimeSeconds, double deltaTimeSeconds,
    double framesPerSecond, ScreenSelectionController* selectionController)
    : sourceValue(&source)
    , destinationValue(&destination)
    , destinationPixelsValue(destination.pixels())
    , destinationWidthValue(destination.width())
    , destinationHeightValue(destination.height())
    , destinationPitchValue(destination.pitch())
    , frameTimeSecondsValue(frameTimeSeconds)
    , deltaTimeSecondsValue(deltaTimeSeconds)
    , framesPerSecondValue(framesPerSecond)
    , selectionControllerValue(selectionController) {
}

ScreenRenderContext::ScreenRenderContext(const IndexedFrame& source,
    IndexedDisplayFrame& destination, unsigned char* destinationPixels,
    int destinationWidth, int destinationHeight, int destinationPitch,
    double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond,
    ScreenSelectionController* selectionController)
    : sourceValue(&source)
    , destinationValue(&destination)
    , destinationPixelsValue(destinationPixels)
    , destinationWidthValue(destinationWidth)
    , destinationHeightValue(destinationHeight)
    , destinationPitchValue(destinationPitch)
    , frameTimeSecondsValue(frameTimeSeconds)
    , deltaTimeSecondsValue(deltaTimeSeconds)
    , framesPerSecondValue(framesPerSecond)
    , selectionControllerValue(selectionController) {
}

const IndexedFrame& ScreenRenderContext::source() const {
    return *sourceValue;
}

IndexedDisplayFrame& ScreenRenderContext::destination() const {
    return *destinationValue;
}

const unsigned char* ScreenRenderContext::sourcePixels() const {
    return sourceValue->pixels;
}

const unsigned char* ScreenRenderContext::sourceLine(int y) const {
    return sourceValue->pixels + y * sourceValue->pitch;
}

int ScreenRenderContext::sourceWidth() const {
    return sourceValue->width;
}

int ScreenRenderContext::sourceHeight() const {
    return sourceValue->height;
}

int ScreenRenderContext::sourcePitch() const {
    return sourceValue->pitch;
}

unsigned char* ScreenRenderContext::destinationPixels() {
    return destinationPixelsValue;
}

unsigned char* ScreenRenderContext::destinationLine(int y) {
    return destinationPixelsValue + y * destinationPitchValue;
}

int ScreenRenderContext::destinationWidth() const {
    return destinationWidthValue;
}

int ScreenRenderContext::destinationHeight() const {
    return destinationHeightValue;
}

int ScreenRenderContext::destinationPitch() const {
    return destinationPitchValue;
}

double ScreenRenderContext::frameTimeSeconds() const {
    return frameTimeSecondsValue;
}

double ScreenRenderContext::deltaTimeSeconds() const {
    return deltaTimeSecondsValue;
}

double ScreenRenderContext::framesPerSecond() const {
    return framesPerSecondValue;
}

int ScreenRenderContext::requestScreenChange(int by, int doSave) {
    if (selectionControllerValue == 0)
        return 0;

    selectionControllerValue->change(by, doSave);
    return 1;
}

ScreenRenderContext* currentScreenRenderContext() {
    return currentContext;
}

void setCurrentScreenRenderContext(ScreenRenderContext* context) {
    currentContext = context;
}

ScopedScreenRenderContext::ScopedScreenRenderContext(ScreenRenderContext& context)
    : previousContext(currentScreenRenderContext()) {
    setCurrentScreenRenderContext(&context);
}

ScopedScreenRenderContext::~ScopedScreenRenderContext() {
    setCurrentScreenRenderContext(previousContext);
}
