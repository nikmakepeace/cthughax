/** @file
 * Runtime frame-generator control port and default implementation.
 */

#ifndef CTHUGHA_RUNTIME_FRAME_GENERATOR_CONTROLS_H
#define CTHUGHA_RUNTIME_FRAME_GENERATOR_CONTROLS_H

#include <string>
#include <vector>

class FrameGeneratorRuntime;

/** Controls runtime frame-generation policy options. */
class RuntimeFrameGeneratorControls {
public:
    /** Destroys the frame-generator controls interface. */
    virtual ~RuntimeFrameGeneratorControls() { }

    /**
     * Sets the probability that palette changes smooth.
     *
     * @param chance Probability in the range 0..1.
     */
    virtual void changePaletteSmoothingChanceTo(double chance) = 0;

    /**
     * Sets the runtime frame filterchain stage order.
     *
     * @param stages Stable names for user-reorderable stages.
     * @param enabled Per-stage enabled state; omitted entries are enabled.
     */
    virtual void changeFilterchainSequenceTo(
        const std::vector<std::string>& stages,
        const std::vector<int>& enabled) = 0;

    /**
     * Sets runtime frame filterchain stage enable gates without changing order.
     *
     * @param stages Stable names for user-controlled stages.
     * @param enabled Per-stage enabled state; omitted entries are enabled.
     */
    virtual void changeFilterchainEnabledTo(
        const std::vector<std::string>& stages,
        const std::vector<int>& enabled) = 0;
};

/** RuntimeFrameGeneratorControls backed by the owned frame generator. */
class DefaultRuntimeFrameGeneratorControls
    : public RuntimeFrameGeneratorControls {
    FrameGeneratorRuntime& frameGenerator;

public:
    /**
     * Creates controls over an application-owned frame generator.
     *
     * @param frameGenerator_ Frame generator to mutate.
     */
    explicit DefaultRuntimeFrameGeneratorControls(
        FrameGeneratorRuntime& frameGenerator_);

    /** Sets the probability that palette changes smooth. */
    virtual void changePaletteSmoothingChanceTo(double chance);

    /** Sets the runtime frame filterchain stage order. */
    virtual void changeFilterchainSequenceTo(
        const std::vector<std::string>& stages,
        const std::vector<int>& enabled);

    /** Sets runtime frame filterchain stage enable gates. */
    virtual void changeFilterchainEnabledTo(
        const std::vector<std::string>& stages,
        const std::vector<int>& enabled);
};

#endif
