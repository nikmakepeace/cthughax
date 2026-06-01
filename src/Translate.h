// Runtime translation executor.

#ifndef __TRANSLATE_RUNTIME_H
#define __TRANSLATE_RUNTIME_H

#include "TranslationTable.h"

class CthughaBuffer;
class VideoFrameContext;

class Translate {
    TranslationTable tableValue;

public:
    Translate();
    explicit Translate(const TranslationTable& table);

    const char* name() const;
    int ready() const;
    void execute(CthughaBuffer& buffer, const VideoFrameContext& context) const;
};

#endif
