#ifndef __FLAME_H
#define __FLAME_H

class CthughaBuffer;
class VisualFrameContext;

class FlameLookupTables {
public:
    unsigned char divsub[4 * 256];
    unsigned char divsub2[4 * 256];
    unsigned int divsub4[256];
    unsigned int divsub_s0[4 * 256];
    unsigned int divsub_s1[4 * 256];
    unsigned int divsub_s2[4 * 256];
    unsigned int divsub_s3[4 * 256];

    FlameLookupTables();
};

class FlameRuntime {
public:
    int generalFlame;
    const FlameLookupTables& lookupTables;

    FlameRuntime(int generalFlame_, const FlameLookupTables& lookupTables_);
};

class Flame {
public:
    typedef void (*Function)(CthughaBuffer& buffer,
        const VisualFrameContext& context, FlameRuntime& runtime);

private:
    Function functionValue;
    const char* nameValue;
    const char* descriptionValue;

public:
    Flame(Function function, const char* name, const char* description);

    const char* name() const;
    const char* description() const;
    void execute(CthughaBuffer& buffer, const VisualFrameContext& context,
        int generalFlame, const FlameLookupTables& lookupTables) const;
};

extern const Flame flameCatalog[];
extern const int nFlameCatalogEntries;

const Flame* flameByIndex(int index);
int init_flames();

#endif
