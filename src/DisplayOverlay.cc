/** @file
 * Display-neutral overlay producer composition.
 */

#include "DisplayOverlay.h"

#include "DisplayPresentationOptions.h"
#include "FpsOverlay.h"
#include "Interface.h"
#include "InterfaceRuntime.h"

class CurrentInterfaceOverlayProducer : public OverlayProducer {
    InterfaceRuntime& runtime;
    OverlayLayout layout;
    DisplayStatusSnapshot status;

public:
    CurrentInterfaceOverlayProducer(InterfaceRuntime& runtime_,
        const OverlayLayout& layout_, const DisplayStatusSnapshot& status_)
        : runtime(runtime_)
        , layout(layout_)
        , status(status_) {
    }

    virtual void produceOverlay(OverlaySink& sink) {
        Interface* currentInterface = runtime.current();
        if (currentInterface == 0)
            return;

        OverlayRenderContext overlay(sink, layout, status);
        currentInterface->display(runtime, overlay);
    }
};

class ErrorMessagesOverlayProducer : public OverlayProducer {
    ErrorMessages& errorMessages;
    InterfaceRuntime& runtime;
    OverlayLayout layout;
    DisplayStatusSnapshot status;

public:
    ErrorMessagesOverlayProducer(ErrorMessages& errorMessages_,
        InterfaceRuntime& runtime_, const OverlayLayout& layout_,
        const DisplayStatusSnapshot& status_)
        : errorMessages(errorMessages_)
        , runtime(runtime_)
        , layout(layout_)
        , status(status_) {
    }

    virtual void produceOverlay(OverlaySink& sink) {
        OverlayRenderContext overlay(sink, layout, status);
        errorMessages.display(runtime, overlay);
    }
};

OverlayCommands collectDisplayOverlayCommands(double framesPerSecond,
    InterfaceRuntime& runtime, ErrorMessages& errorMessages,
    const OverlayLayout& layout, const DisplayStatusSnapshot& status,
    DisplayPresentationSettings& settings) {
    CurrentInterfaceOverlayProducer interfaceProducer(runtime, layout, status);
    ErrorMessagesOverlayProducer errorProducer(errorMessages, runtime, layout,
        status);
    OverlaySource source(&interfaceProducer, &errorProducer);
    OverlayCommands overlays = source.collect();
    FpsOverlay::append(overlays, framesPerSecond, int(settings.showFPS));
    return overlays;
}
