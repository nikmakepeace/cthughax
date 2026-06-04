// Audio lifecycle owner.

#ifndef __AUDIO_RUNTIME_H
#define __AUDIO_RUNTIME_H

#include "RuntimeFactory.h"

struct AudioConfig;
class RuntimeCommandSink;

/**
 * Builds and starts the audio runtime from the current option snapshot.
 *
 * @param initializeInputControls Nonzero to initialize hardware/input controls
 *        such as mixer state while creating the input source.
 * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
 *        before display zoom. Used to size audio frame and DSP sample windows.
 * @return 0 on success, nonzero if input, output, or processing setup fails.
 */
int audioRuntimeInit(const AudioConfig& config, int initializeInputControls,
    int visualMaxDimension, RuntimeCommandSink* runtimeCommands = 0);

/**
 * Advances the audio runtime by one visual frame boundary.
 *
 * Pumps input/output, rebuilds the current AudioFrame, and services completion
 * state for file playback.
 */
void audioRuntimeTick();

/**
 * Stops audio processing, releases runtime-owned input/output objects, and
 * clears the published current frame.
 */
void audioRuntimeShutdown();

/**
 * @return Nonzero when audioRuntimeInit() has completed and runtime objects are live.
 */
int audioRuntimeIsInitialized();

/**
 * @return The active input processor, or NULL when the audio runtime is shut down.
 */
AudioInputProcessor* audioRuntimeProcessor();

/**
 * @return The current audio frame facade, or NULL before the runtime publishes one.
 */
AudioFrame* audioRuntimeCurrentFrame();

/**
 * @return Nonzero when the selected input has finished playback and output drain is complete.
 */
int audioRuntimeIsComplete();

#endif
