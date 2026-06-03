#ifndef __FRAME_PACER_H
#define __FRAME_PACER_H

class FrameSleeper {
public:
    virtual ~FrameSleeper() { }
    virtual void sleepSeconds(double seconds) = 0;
};

class SystemFrameSleeper : public FrameSleeper {
public:
    virtual void sleepSeconds(double seconds);
};

class FramePacingResult {
public:
    double frameStartSeconds;
    double frameEndSeconds;
    double targetFrameEndSeconds;
    double requestedSleepSeconds;
    int maxFramesPerSecond;

    FramePacingResult();
};

class FramePacer {
    FrameSleeper& sleeper;
    double nextFrameStartTargetSeconds;
    int hasFrameStartTarget;

public:
    explicit FramePacer(FrameSleeper& sleeper_);

    FramePacingResult paceFrameEnd(double frameStartSeconds,
        double frameEndSeconds, int maxFramesPerSecond);
};

#endif
