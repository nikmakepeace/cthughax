#include "OverlaySource.h"

#include <assert.h>
#include <string.h>

class FakeOverlayProducer : public OverlayProducer {
    const char* textValue;
    double yValue;
    int justificationValue;
    int colorValue;
    int noDarkenValue;

public:
    int calls;

    FakeOverlayProducer(const char* text_, double y_, int justification_,
        int color_, int noDarken_)
        : textValue(text_)
        , yValue(y_)
        , justificationValue(justification_)
        , colorValue(color_)
        , noDarkenValue(noDarken_)
        , calls(0) {
    }

    virtual void produceOverlay(OverlaySink& sink) {
        calls++;
        sink.printText(textValue, yValue, justificationValue, colorValue,
            noDarkenValue);
    }
};

class ChainedOverlayProducer : public OverlayProducer {
public:
    virtual void produceOverlay(OverlaySink& sink) {
        double nextLine = sink.printText("first\nsecond", 1.5, 'l', 0, 0);
        sink.printText("after", nextLine, 'r', 2, 1);
    }
};

static void testCollectsInterfaceAndErrorCommandsInOrder() {
    FakeOverlayProducer interfaceProducer("interface", 0.0, 'l', 2, 0);
    FakeOverlayProducer errorProducer("error", -1.0, 'r', 1, 0);
    OverlaySource source(&interfaceProducer, &errorProducer);

    OverlayCommands commands = source.collect();

    assert(interfaceProducer.calls == 1);
    assert(errorProducer.calls == 1);
    assert(commands.count() == 2);
    assert(strcmp(commands.at(0).text.c_str(), "interface") == 0);
    assert(commands.at(0).y == 0.0);
    assert(commands.at(0).justification == 'l');
    assert(commands.at(0).color == 2);
    assert(commands.at(0).noDarken == 0);
    assert(strcmp(commands.at(1).text.c_str(), "error") == 0);
    assert(commands.at(1).y == -1.0);
    assert(commands.at(1).justification == 'r');
    assert(commands.at(1).color == 1);
}

static void testEmptySourcesProduceNoCommands() {
    OverlaySource source;

    OverlayCommands commands = source.collect();

    assert(commands.empty());
}

static void testSinkReturnsNextLineForMultilineText() {
    ChainedOverlayProducer producer;
    OverlaySource source(&producer, 0);

    OverlayCommands commands = source.collect();

    assert(commands.count() == 2);
    assert(strcmp(commands.at(0).text.c_str(), "first\nsecond") == 0);
    assert(commands.at(0).y == 1.5);
    assert(strcmp(commands.at(1).text.c_str(), "after") == 0);
    assert(commands.at(1).y == 3.5);
}

int main() {
    testCollectsInterfaceAndErrorCommandsInOrder();
    testEmptySourcesProduceNoCommands();
    testSinkReturnsNextLineForMultilineText();
    return 0;
}
