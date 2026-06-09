#ifndef __FLAME_H
#define __FLAME_H

class FrameRenderTarget;
class FrameGeneratorContext;

/**
 * Precomputed byte math used by flame feedback kernels.
 *
 * The flame functions repeatedly average small groups of 8-bit palette indexes
 * and apply decay. These tables keep that hot path cheap.
 */
class FlameLookupTables {
public:
    /** Four-sample average with decay, indexed by sums in the range 0-1020. */
    unsigned char divsub[4 * 256];

    /** Two-sample average with decay, indexed by sums in the range 0-1020. */
    unsigned char divsub2[4 * 256];

    /** Per-byte fade table used when processing four packed pixels. */
    unsigned int divsub4[256];

    /** Pre-shifted divsub values for packed four-pixel writes. */
    unsigned int divsub_s0[4 * 256];

    /** Pre-shifted divsub values for packed four-pixel writes. */
    unsigned int divsub_s1[4 * 256];

    /** Pre-shifted divsub values for packed four-pixel writes. */
    unsigned int divsub_s2[4 * 256];

    /** Pre-shifted divsub values for packed four-pixel writes. */
    unsigned int divsub_s3[4 * 256];

    FlameLookupTables();
};

/**
 * Per-frame scratch context passed to a flame kernel.
 */
class FlameRuntime {
public:
    /**
     * Encoded general-flame setting.
     *
     * Interpreted by general flame kernels as base-9 neighbor choices and shift.
     */
    int generalFlame;

    /** Borrowed lookup tables owned by FlameFilter. */
    const FlameLookupTables& lookupTables;

    FlameRuntime(int generalFlame_, const FlameLookupTables& lookupTables_);
};

/**
 * Catalog entry and executor for one flame feedback kernel.
 */
class Flame {
public:
    /**
     * Flame kernel function.
     *
     * @param buffer Active/passive indexed pixel buffer.
     * @param context Current visual frame context.
     * @param runtime Flame lookup tables and general-flame setting.
     */
    typedef void (*Function)(FrameRenderTarget& buffer,
        const FrameGeneratorContext& context, FlameRuntime& runtime);

private:
    Function functionValue;
    const char* nameValue;
    const char* descriptionValue;

public:
    /**
     * Creates a flame catalog entry.
     *
     * @param function Kernel function to run, or NULL for a no-op entry.
     * @param name Short option/UI name.
     * @param description Human-readable description.
     */
    Flame(Function function, const char* name, const char* description);

    /** @return Short option/UI name. */
    const char* name() const;

    /** @return Human-readable description. */
    const char* description() const;

    /**
     * Executes the flame kernel for one visual frame.
     *
     * @param buffer Active/passive indexed pixel buffer to read/write.
     * @param context Current visual frame context.
     * @param generalFlame Encoded general-flame option value.
     * @param lookupTables Precomputed flame math tables.
     */
    void execute(FrameRenderTarget& buffer, const FrameGeneratorContext& context,
        int generalFlame, const FlameLookupTables& lookupTables) const;
};

/** Built-in flame catalog used by FlameOption. */
extern const Flame flameCatalog[];

/** Number of entries in flameCatalog. */
extern const int nFlameCatalogEntries;

/**
 * @param index Catalog index.
 * @return Flame at index, or NULL when out of range.
 */
const Flame* flameByIndex(int index);

/**
 * Legacy startup hook for flame initialization.
 *
 * Flame lookup tables are now owned by FlameFilter, so this currently only
 * preserves the old initialization sequence.
 */
int init_flames();

#endif
