#include "cthugha.h"
#include "Interface.h"
#include "display.h"
#include "cth_buffer.h"
#include "imath.h"
#include "CthughaBuffer.h"
#include "SoundAnalyze.h"
#include "CthughaDisplay.h"

#include <math.h>

OptionInt sound_minnoise("minnoise", 5, 256); /* quiet is below this */

SoundAnalyze soundAnalyze;

SoundAnalyze::SoundAnalyze() { intensity = 0.0; }

void SoundAnalyze::operator()() {

    static double lastFires[16];
    static int lastFiresP = 0;

    static int lastamp = 0;
    int al = 0, ar = 0;

    /* get the amplitude of this sound frame (root mean squared) */
    char* d = (char*)soundDevice->data;
    for (int i = 1024; i != 0; i--) {
        al += *d * *d;
        d++;
        ar += *d * *d;
        d++;
    }
    /* sqare root the mean */
    al = int(sqrt(double(al) / 1024));
    ar = int(sqrt(double(ar) / 1024));

    amplitude = (al + ar) / 2;
    amplitudeLeft = al;
    amplitudeRight = ar;

    if (amplitude < lastamp - 9) /* ignore such a small decrease */
        amplitude = lastamp - 9;

    if (amplitude > lastamp)
        attackLevel += amplitude - lastamp;

    if ((now - lastFires[lastFiresP]) > 0.001)
        speed = 16.0 / (now - lastFires[lastFiresP]);

    /* if the attack is finally over, then fire at the intensity of the attack */
    if (amplitude < lastamp) {
        fire = attackLevel;
        attackLevel = 0;

        if (fire > 0)
            CTH_DEBUG("sound fire: fire=%d fireLevel=%d amplitude=%d lastamp=%d speed=%.2f\n",
                fire, fireLevel + fire, amplitude, lastamp, speed);

        lastFires[lastFiresP] = now;
        lastFiresP = (lastFiresP + 1) % 16;
    } else
        fire = 0;
    fireLevel += fire;

    lastamp = amplitude;

    /* check for silence */
    noisy = ((amplitudeLeft >= sound_minnoise) || (amplitudeRight >= sound_minnoise));

    intensity = intensity * 0.95 + (amplitude / 128.0) * 0.05;

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
