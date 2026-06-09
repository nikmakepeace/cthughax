/** @file
 * SDL3 display driver factory.
 */

#ifndef CTHUGHA_DISPLAY_DEVICE_SDL3_H
#define CTHUGHA_DISPLAY_DEVICE_SDL3_H

#include <memory>

class DisplayDriverFactory;
struct SDL3Config;

/**
 * Creates the SDL3 display driver factory when SDL3 is compiled in.
 *
 * @param config SDL3-specific startup configuration.
 */
std::unique_ptr<DisplayDriverFactory> newSDL3DisplayDriverFactory(
    const SDL3Config& config);

#endif
