/** @file
 * Shared overlay collection for display coordinators.
 */

#ifndef CTHUGHA_DISPLAY_OVERLAY_H
#define CTHUGHA_DISPLAY_OVERLAY_H

#include "OverlaySource.h"

class DisplayPresentationSettings;
class ErrorMessages;
class InterfaceRuntime;

/**
 * Collects interface, error, and FPS overlay commands for a display frame.
 *
 * @param framesPerSecond Instantaneous frame rate used by the optional FPS row.
 * @param runtime Interface runtime that owns the active panel.
 * @param errorMessages Application error-message source.
 * @param layout Text-cell layout exposed by the current display output.
 * @param status Display status text and frame timing for interface rows.
 * @param settings Presentation settings controlling overlay visibility.
 * @return Ordered text commands ready for backend rendering.
 */
OverlayCommands collectDisplayOverlayCommands(double framesPerSecond,
    InterfaceRuntime& runtime, ErrorMessages& errorMessages,
    const OverlayLayout& layout, const DisplayStatusSnapshot& status,
    DisplayPresentationSettings& settings);

#endif
