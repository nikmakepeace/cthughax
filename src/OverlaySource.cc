#include "OverlaySource.h"

#include <string.h>

OverlayTextCommand::OverlayTextCommand()
    : text()
    , y(0.0)
    , justification('l')
    , color(0)
    , noDarken(0) {
}

OverlayTextCommand::OverlayTextCommand(const char* text_, double y_,
    int justification_, int color_, int noDarken_)
    : text(text_ ? text_ : "")
    , y(y_)
    , justification(justification_)
    , color(color_)
    , noDarken(noDarken_) {
}

size_t OverlayCommands::count() const {
    return textCommands.size();
}

bool OverlayCommands::empty() const {
    return textCommands.empty();
}

const OverlayTextCommand& OverlayCommands::at(size_t index) const {
    return textCommands.at(index);
}

double OverlayCommands::addText(const char* text, double y, int justification,
    int color, int noDarken) {
    textCommands.push_back(OverlayTextCommand(text, y, justification, color,
        noDarken));

    if (text == 0)
        return y;

    const char* lineStart = text;
    const char* lineEnd;
    do {
        lineEnd = strchr(lineStart, '\n');
        int len = lineEnd ? lineEnd - lineStart : strlen(lineStart);
        if (len == 0)
            return y;

        y += 1.0;
        lineStart = lineEnd + 1;
    } while (lineEnd);

    return y;
}

class OverlayCommandCollector : public OverlaySink {
public:
    OverlayCommands commands;

    virtual double printText(const char* text, double y, int justification,
        int color, int noDarken) {
        return commands.addText(text, y, justification, color, noDarken);
    }
};

OverlaySource::OverlaySource(OverlayProducer* interfaceProducer_,
    OverlayProducer* errorProducer_)
    : interfaceProducer(interfaceProducer_)
    , errorProducer(errorProducer_) {
}

OverlayCommands OverlaySource::collect() const {
    OverlayCommandCollector collector;

    if (interfaceProducer != 0)
        interfaceProducer->produceOverlay(collector);
    if (errorProducer != 0)
        errorProducer->produceOverlay(collector);

    return collector.commands;
}
