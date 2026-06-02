#include "CthughaDisplay.h"
#include "IndexedDisplayFrame.h"
#include "IndexedFrame.h"

#include <assert.h>

static void testIndexedDisplayFrameGeometry() {
    IndexedDisplayFrame frame;
    assert(!frame.valid());
    assert(frame.byteCount() == 0);

    frame.resize(320, 240);
    assert(frame.valid());
    assert(frame.width() == 320);
    assert(frame.height() == 240);
    assert(frame.pitch() == 320);
    assert(frame.byteCount() == 320 * 240);
    assert(frame.line(10) == frame.pixels() + 10 * 320);

    unsigned char* originalPixels = frame.pixels();
    int originalCapacity = frame.capacityByteCount();

    frame.resize(160, 100, 200);
    assert(frame.valid());
    assert(frame.width() == 160);
    assert(frame.height() == 100);
    assert(frame.pitch() == 200);
    assert(frame.byteCount() == 200 * 100);
    assert(frame.line(7) == frame.pixels() + 7 * 200);
    assert(frame.pixels() == originalPixels);
    assert(frame.capacityByteCount() == originalCapacity);

    frame.resize(100, 10, 99);
    assert(!frame.valid());
    assert(frame.pixels() == 0);
    assert(frame.byteCount() == 0);
}

static void testIndexedFramePitchValidity() {
    unsigned char pixels[64];

    IndexedFrame padded(pixels, 8, 4, 12, 0);
    assert(padded.valid());

    IndexedFrame tooTight(pixels, 8, 4, 7, 0);
    assert(!tooTight.valid());
}

static void testScreenEntryCompatibilityOutputSize() {
    xy output = ScreenEntry::compatibilityOutputSize(320, 240);

    assert(output.x == 640);
    assert(output.y == 480);
}

int main() {
    testIndexedDisplayFrameGeometry();
    testIndexedFramePitchValidity();
    testScreenEntryCompatibilityOutputSize();
    return 0;
}
