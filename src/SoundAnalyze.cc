#include "cthugha.h"
#include "Interface.h"
#include "display.h"
#include "cth_buffer.h"
#include "imath.h"
#include "CthughaBuffer.h"
#include "SoundAnalyze.h"
#include "CthughaDisplay.h"
#include "AudioFrame.h"

#include <math.h>

OptionInt sound_minnoise("minnoise", 5, 256); /* quiet is below this */

SoundAnalyze soundAnalyze;

AudioAnalysis::AudioAnalysis()
    : amplitude(0)
    , amplitudeLeft(0)
    , amplitudeRight(0)
    , noisy(0)
    , attackLevel(0)
    , fire(0)
    , intensity(0.0)
    , speed(0.0) { }

AudioAnalyzer::AudioAnalyzer()
    : lastFiresP(0)
    , lastamp(0)
    , attackLevel(0)
    , intensity(0.0)
    , speed(0.0) {
    memset(lastFires, 0, sizeof(lastFires));
}

AudioAnalysis AudioAnalyzer::analyze(const char2* frame) {
    AudioAnalysis analysis;
    int al = 0, ar = 0;

    /* get the amplitude of this sound frame (root mean squared) */
    char* d = (char*)frame;
    for (int i = 1024; i != 0; i--) {
        al += *d * *d;
        d++;
        ar += *d * *d;
        d++;
    }
    /* sqare root the mean */
    al = int(sqrt(double(al) / 1024));
    ar = int(sqrt(double(ar) / 1024));

    analysis.amplitude = (al + ar) / 2;
    analysis.amplitudeLeft = al;
    analysis.amplitudeRight = ar;

    if (analysis.amplitude < lastamp - 9) /* ignore such a small decrease */
        analysis.amplitude = lastamp - 9;

    if (analysis.amplitude > lastamp)
        attackLevel += analysis.amplitude - lastamp;

    if ((now - lastFires[lastFiresP]) > 0.001)
        speed = 16.0 / (now - lastFires[lastFiresP]);

    /* if the attack is finally over, then fire at the intensity of the attack */
    if (analysis.amplitude < lastamp) {
        analysis.fire = attackLevel;
        attackLevel = 0;

        if (analysis.fire > 0)
            CTH_DEBUG("sound fire: fire=%d amplitude=%d lastamp=%d speed=%.2f\n",
                analysis.fire, analysis.amplitude, lastamp, speed);

        lastFires[lastFiresP] = now;
        lastFiresP = (lastFiresP + 1) % 16;
    } else
        analysis.fire = 0;

    lastamp = analysis.amplitude;
    analysis.attackLevel = attackLevel;

    /* check for silence */
    analysis.noisy = ((analysis.amplitudeLeft >= sound_minnoise)
        || (analysis.amplitudeRight >= sound_minnoise));

    intensity = intensity * 0.95 + (analysis.amplitude / 128.0) * 0.05;
    analysis.intensity = intensity;
    analysis.speed = speed;

    return analysis;
}

SoundAnalyze::SoundAnalyze()
    : analyzer() {
    amplitude = 0;
    amplitudeLeft = 0;
    amplitudeRight = 0;
    noisy = 0;
    attackLevel = 0;
    fire = 0;
    fireLevel = 0;
    intensity = 0.0;
    speed = 0.0;
}

void SoundAnalyze::operator()() {
    AudioAnalysis analysis = analyzer.analyze(audioFrameData());

    amplitude = analysis.amplitude;
    amplitudeLeft = analysis.amplitudeLeft;
    amplitudeRight = analysis.amplitudeRight;
    noisy = analysis.noisy;
    attackLevel = analysis.attackLevel;
    fire = analysis.fire;
    fireLevel += fire;
    intensity = analysis.intensity;
    speed = analysis.speed;

#if 0
    /* compute beats/minute, not working as it should */
    if(soundAnalyze->fire > 20) {
	static int bt[16];
	static int bn=0;
	int sound_bpm;

	bt[bn] = gettime();

	sound_bpm = bt[bn] - bt[ (bn+1)%16 ];		/* time for 16 beats */
	if(sound_bpm > 0) {
	    sound_bpm = 16*60000/sound_bpm;
	}
	CTH_DEBUG("bpm: %d\n", sound_bpm);

	bn = (bn+1)%16;
    }
#endif
}
