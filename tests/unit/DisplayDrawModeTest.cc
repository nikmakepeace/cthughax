#include "DisplayDevice.h"

#include <assert.h>

static void testMappedDrawModeMatchesBytesPerPixel() {
    draw_mode_t mode = DM_mapped4;

    assert(mappedDrawModeForBytesPerPixel(1, mode));
    assert(mode == DM_mapped1);

    assert(mappedDrawModeForBytesPerPixel(2, mode));
    assert(mode == DM_mapped2);

    assert(mappedDrawModeForBytesPerPixel(3, mode));
    assert(mode == DM_mapped3);

    assert(mappedDrawModeForBytesPerPixel(4, mode));
    assert(mode == DM_mapped4);
}

static void testInvalidByteWidthDoesNotSelectMode() {
    draw_mode_t mode = DM_mapped2;

    assert(!mappedDrawModeForBytesPerPixel(0, mode));
    assert(mode == DM_mapped2);

    assert(!mappedDrawModeForBytesPerPixel(5, mode));
    assert(mode == DM_mapped2);
}

int main() {
    testMappedDrawModeMatchesBytesPerPixel();
    testInvalidByteWidthDoesNotSelectMode();
    return 0;
}
