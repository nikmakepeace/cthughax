// -*- c++ -*-

#ifndef __AUDIO_ANALYZER_H
#define __AUDIO_ANALYZER_H

#include "cthugha.h"
#include "AudioTypes.h"

#include "CoreOption.h"

extern OptionInt sound_minnoise; /* quiet is below this */

struct AudioAnalysis {
    int amplitude;
    int amplitudeLeft;
    int amplitudeRight;
    int noisy;

    AudioAnalysis();
};

class AcousticContext {
    double intensityValue;
    int lastAmplitudeValue;
    int attackLevelValue;
    int fireValue;
    int fireLevelValue;

public:
    AcousticContext();

    void update(const AudioAnalysis& analysis);
    double intensity() const;
    int fire() const;
    int fireLevel() const;
    void setFire(int fire);
    void resetFire();
    void resetFireLevel();
};

class AudioAnalyzer {
public:
    AudioAnalyzer();

    AudioAnalysis analyze(const char2* frame);
    void operator()();
};

extern AudioAnalyzer audioAnalyzer;
extern AudioAnalysis audioAnalysis;
extern AcousticContext acousticContext;

#endif
