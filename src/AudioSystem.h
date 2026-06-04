// Audio runtime lifecycle hooks used by the main loop and signal handling.

#ifndef __AUDIO_SYSTEM_H
#define __AUDIO_SYSTEM_H

struct AudioConfig;
class RuntimeCommandSink;

/**
 * Initializes the selected audio runtime if it is not already running.
 *
 * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
 *        before display zoom. This is usually max(buffer.width, buffer.height)
 *        and is used to size audio analysis/DSP windows to the visual scale.
 * @return 0 on success, nonzero if audio runtime initialization fails.
 */
int init_sound(const AudioConfig& config, int visualMaxDimension,
    RuntimeCommandSink* runtimeCommands = 0);

/**
 * Shuts down the current audio runtime.
 *
 * Safe to call when no runtime is initialized. Used during normal shutdown and
 * platform suspend handling.
 *
 * @return 0 after shutdown handling completes.
 */
int exit_sound();

#endif
