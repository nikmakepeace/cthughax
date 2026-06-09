#ifndef __DISPLAY_BACKEND_H
#define __DISPLAY_BACKEND_H

#include "DisplayDevice.h"
#include "DisplayPresentation.h"

class InputEventSink;

class DisplayBackend {
public:
    virtual ~DisplayBackend() { }

    virtual DisplayEventStats processEvents(InputEventSink& input) = 0;
    virtual PixelSize outputSize() const = 0;
    virtual void present(const DisplayPresentation&) = 0;
};

#endif
