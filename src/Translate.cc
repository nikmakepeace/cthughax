// Runtime coordinate remap executor used by TranslateFilter.

#include "Translate.h"
#include "FrameStageBuffer.h"

Translate::Translate()
    : tableValue() { }

Translate::Translate(const TranslationTable& table)
    : tableValue(table) { }

const char* Translate::name() const {
    return tableValue.name();
}

int Translate::ready() const {
    return tableValue.ready();
}

static unsigned char translatedSourcePixel(const FrameStageBuffer& buffer,
    const unsigned char* src, int sourceIndex) {
    if (sourceIndex == 0)
        return 0;

    return src[buffer.visibleLinearOffset(sourceIndex)];
}

void Translate::execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context) const {
    (void)context;

    if (!ready())
        return;

    const int width = buffer.width();
    const int height = buffer.height();
    const int size = buffer.size();

    if (width != tableValue.width() || height != tableValue.height()
        || size != tableValue.size())
        return;

    const int* trans = tableValue.data();

    const unsigned char* src = buffer.sourcePixels();
    unsigned char* dst = buffer.destinationPixels();

    if (buffer.visibleRowsArePacked()) {
        for (int i = 0; i < size; i++)
            dst[i] = trans[i] == 0 ? 0 : src[trans[i]];
        return;
    }

    for (int y = 0; y < height; y++) {
        unsigned char* row = buffer.destinationRow(y);
        for (int x = 0; x < width; x++)
            row[x] = translatedSourcePixel(buffer, src, *trans++);
    }
}
