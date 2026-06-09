/** @file
 * Audio-processing selector frame application.
 */

#include "AudioProcessing.h"

#include "Audio.h"
#include "AudioFrame.h"
#include "ProcessServices.h"

int AudioProcessingSelector::process(AudioFrame& frame) {
    log.trace("audio processing", "processing mode=`%s'\n", text());

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
