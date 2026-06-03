#ifndef __OVERLAY_SOURCE_H
#define __OVERLAY_SOURCE_H

#include <stddef.h>
#include <string>
#include <vector>

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
