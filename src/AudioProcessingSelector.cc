/** @file
 * Audio-processing selector frame application.
 */

#include "cthugha.h"
#include "AudioProcessing.h"

#include "Audio.h"
#include "AudioFrame.h"

int AudioProcessingSelector::process(AudioFrame& frame) {
    CTH_TRACE("processing mode=`%s'\n", "audio processing", text());

    switch (mode()) {
    case AudioProcessingModeFft:
        processor.fft(frame);
        break;
    case AudioProcessingModeFilter1:
        processor.filter1(frame);
        break;
    case AudioProcessingModeFilter2:
        processor.filter2(frame);
        break;
    case AudioProcessingModeNone:
    default:
        processor.none(frame);
        break;
    }

    return 1;
}
