#ifndef __OVERLAY_SOURCE_H
#define __OVERLAY_SOURCE_H

#include <stddef.h>
#include <string>
#include <vector>

enum OverlayTextColor {
    TEXT_COLOR_NORMAL = 0,
    TEXT_COLOR_ERROR = 1,
    TEXT_COLOR_HIGHLIGHT = 2
};

/** Text-cell geometry available to overlay producers for the current frame. */
class OverlayLayout {
public:
    int columns;
    int rows;
    int fontWidth;
    int fontHeight;

    /** Creates an empty overlay layout. */
    OverlayLayout();

    /**
     * Creates a concrete overlay text layout.
     *
     * @param columns_ Number of text columns available.
     * @param rows_ Number of text rows available.
     * @param fontWidth_ Width of one text cell in pixels.
     * @param fontHeight_ Height of one text cell in pixels.
     */
    OverlayLayout(int columns_, int rows_, int fontWidth_, int fontHeight_);
};

/** Presentation status Display makes available to overlay producers. */
class DisplayStatusSnapshot {
    std::string frameStatusValue;
    double frameDeltaSecondsValue;

public:
    /** Creates an empty status snapshot. */
    DisplayStatusSnapshot();

    /**
     * Creates a status snapshot for one visual frame.
     *
     * @param frameStatus Display-owned status text such as the FPS readout.
     * @param frameDeltaSeconds Seconds elapsed since the previous visual frame.
     */
    DisplayStatusSnapshot(const char* frameStatus,
        double frameDeltaSeconds);

    /** @return Display-owned frame status text. */
    const char* frameStatus() const;

    /** @return Seconds elapsed since the previous visual frame. */
    double frameDeltaSeconds() const;
};

class OverlayTextCommand {
public:
    std::string text;
    double y;
    int justification;
    int color;
    int noDarken;

    OverlayTextCommand();
    OverlayTextCommand(const char* text_, double y_, int justification_,
        int color_, int noDarken_);
};

class OverlayCommands {
    std::vector<OverlayTextCommand> textCommands;

public:
    size_t count() const;
    bool empty() const;
    const OverlayTextCommand& at(size_t index) const;
    double addText(const char* text, double y, int justification, int color,
        int noDarken);
};

class OverlaySink {
public:
    virtual ~OverlaySink() { }
    virtual double printText(const char* text, double y, int justification,
        int color, int noDarken) = 0;
};

/** Explicit print target and frame-local metrics for interface overlays. */
class OverlayRenderContext {
    OverlaySink& sinkValue;
    OverlayLayout layoutValue;
    DisplayStatusSnapshot statusValue;

public:
    /**
     * Creates an overlay rendering context.
     *
     * @param sink Sink that records or draws text commands.
     * @param layout Text-cell layout for the current display output.
     * @param status Display status snapshot for the current visual frame.
     */
    OverlayRenderContext(OverlaySink& sink, const OverlayLayout& layout,
        const DisplayStatusSnapshot& status);

    /** Records one text overlay command and returns the next line position. */
    double printText(const char* text, double y, int justification, int color,
        int noDarken = 0);

    /** @return Number of text columns available to overlay producers. */
    int textColumns() const;

    /** @return Number of text rows available to overlay producers. */
    int textRows() const;

    /** @return Display status snapshot for the current visual frame. */
    const DisplayStatusSnapshot& status() const;
};

class OverlayProducer {
public:
    virtual ~OverlayProducer() { }
    virtual void produceOverlay(OverlaySink& sink) = 0;
};

class OverlaySource {
    OverlayProducer* interfaceProducer;
    OverlayProducer* errorProducer;

public:
    OverlaySource(OverlayProducer* interfaceProducer_ = 0,
        OverlayProducer* errorProducer_ = 0);

    OverlayCommands collect() const;
};

#endif
