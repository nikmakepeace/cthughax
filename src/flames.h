#ifndef __FLAMES_H
#define __FLAMES_H

#include "cthugha.h"
#include "CoreOption.h"

class CthughaBuffer;
class VisualFrameContext;

class FlameEntry : public CoreOptionEntry {
    void (*flame)(CthughaBuffer& buffer);

public:
    FlameEntry(void (*f)(CthughaBuffer& buffer), const char* name, const char* desc, int inUse = 1);

    void execute(CthughaBuffer& buffer, const VisualFrameContext& context);
};

extern CoreOptionEntry* _flames[];
extern int _nFlames;

int init_flames();

extern CoreOptionEntryList generalFlameEntries;

class OptionGeneralFlame : public CoreOption {
public:
    OptionGeneralFlame(int buffer)
        : CoreOption(buffer, "flame-general", generalFlameEntries) { }

    const char* text() const;
};

#endif
