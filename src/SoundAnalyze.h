// -*- c++ -*-

#ifndef __SOUND_ANALYZE_H
#define __SOUND_ANALYZE_H

#include "cthugha.h"
#include "Sound.h"

#include "CoreOption.h"

extern OptionInt sound_minnoise; /* quiet is below this */

struct AudioAnalysis {
    int amplitude;
    int amplitudeLeft;
    int amplitudeRight;
    int noisy;
    int attackLevel;
    int fire;
    double intensity;
    double speed;

    AudioAnalysis();
};

class AudioAnalyzer {
    double lastFires[16];
    int lastFiresP;
    int lastamp;
    int attackLevel;
    double intensity;
    double speed;

public:
    AudioAnalyzer();

    AudioAnalysis analyze(const char2* frame);
};

class SoundAnalyze {
    AudioAnalyzer analyzer;

public:
    SoundAnalyze();

    void operator()(); // does noiselvel checking, massage, fft, ...

    int amplitude; // sound amplitude (variance)
    int amplitudeLeft;
    int amplitudeRight;
    int noisy; // there is some sound
    int attackLevel; // attack level
    int fire; // fired now
    int fireLevel; // accumulated fire (for change)

    double intensity; // smoothed and normalized amplitude

    double speed;
};

extern SoundAnalyze soundAnalyze;

#endif
