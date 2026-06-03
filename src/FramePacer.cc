#include "FramePacer.h"

#include <unistd.h>

void SystemFrameSleeper::sleepSeconds(double seconds) {
    if (seconds > 0)
        usleep(int(seconds * 1e6));
}

FramePacingResult::FramePacingResult()
    : frameStartSeconds(0.0)
    , frameEndSeconds(0.0)
    , targetFrameEndSeconds(0.0)
    , requestedSleepSeconds(0.0)
    , maxFramesPerSecond(0) {
}

FramePacer::FramePacer(FrameSleeper& sleeper_)
    : sleeper(sleeper_)
    , nextFrameStartTargetSeconds(0.0)
    , hasFrameStartTarget(0) {
}

FramePacingResult FramePacer::paceFrameEnd(double frameStartSeconds,
    double frameEndSeconds, int maxFramesPerSecond) {
    FramePacingResult result;
    result.frameStartSeconds = frameStartSeconds;
    result.frameEndSeconds = frameEndSeconds;
    result.maxFramesPerSecond = maxFramesPerSecond;

    if (maxFramesPerSecond <= 0) {
        hasFrameStartTarget = 0;
        result.targetFrameEndSeconds = frameEndSeconds;
        return result;
    }

    double framePeriodSeconds = 1.0 / double(maxFramesPerSecond);
    if (!hasFrameStartTarget
        || frameStartSeconds > nextFrameStartTargetSeconds + framePeriodSeconds) {
        nextFrameStartTargetSeconds = frameStartSeconds + framePeriodSeconds;
        hasFrameStartTarget = 1;
    }

    result.targetFrameEndSeconds = nextFrameStartTargetSeconds;
    nextFrameStartTargetSeconds += framePeriodSeconds;
    result.requestedSleepSeconds = result.targetFrameEndSeconds - frameEndSeconds;
    if (result.requestedSleepSeconds <= 0) {
        result.requestedSleepSeconds = 0.0;
        return result;
    }

    sleeper.sleepSeconds(result.requestedSleepSeconds);
    return result;
}
