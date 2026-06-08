// Runtime coordinate remap executor used by TranslateFilter.

#include "cthugha.h"
#include "Translate.h"
#include "FrameRenderTarget.h"

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

void Translate::execute(FrameRenderTarget& buffer, const FrameRenderContext& context) const {
    (void)context;

    if (!ready())
        return;

    if (buffer.width() != tableValue.width() || buffer.height() != tableValue.height()
        || buffer.size() != tableValue.size())
        return;

    const int* trans = tableValue.data();

    buffer.swapBuffers();
    unsigned char* src = buffer.passivePixels();

    src[0] = 0;

    for (int y = 0; y < buffer.height(); y++) {
        unsigned char* dst = buffer.activeRow(y);
        for (int x = 0; x < buffer.width(); x++) {
            int sourceIndex = *trans++;
            int sourceX = sourceIndex % buffer.width();
            int sourceY = sourceIndex / buffer.width();
            dst[x] = src[buffer.visibleOffset(sourceX, sourceY)];
        }
    }
}
