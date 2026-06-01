#include "cthugha.h"
#include "Translate.h"
#include "CthughaBuffer.h"

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

void Translate::execute(CthughaBuffer& buffer, const VideoFrameContext& context) const {
    (void)context;

    int i;
    unsigned int* dst;
    unsigned char* src;

    if (!ready())
        return;

    if (buffer.width() != tableValue.width() || buffer.height() != tableValue.height()
        || buffer.size() != tableValue.size())
        return;

    const int* trans = tableValue.data();

    buffer.swapBuffers();
    dst = (unsigned int*)buffer.activePixels();
    src = buffer.passivePixels();

    src[0] = 0;

    // thanks to Antonio Schifano (schifano@cli.di.unipi.it)
    // for finding the endianess bug here

#if (__BYTE_ORDER == __BIG_ENDIAN)
    /* always write 4 values at once */
    for (i = tableValue.size() / 4; i != 0; i--) { /* do the translation */
        unsigned int D = src[*trans++];
        D <<= 8;
        D += src[*trans++];
        D <<= 8;
        D += src[*trans++];
        D <<= 8;
        *dst++ = D + src[*trans++];
    }
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
    for (i = tableValue.size() / 4; i != 0; i--) { /* do the translation */
        unsigned int D = src[trans[0]];
        D += src[trans[1]] << 8;
        D += src[trans[2]] << 16;
        *dst = D + (src[trans[3]] << 24);
        trans += 4;
        dst++;
    }
// #elif
// #error unknown endianess
#endif
}
