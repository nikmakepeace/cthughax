#include "cthugha.h"
#include "Audio.h"
#include "AudioFrame.h"
#include "AudioProcessor.h"
#include "Interface.h"
#include "display.h"
#include "cth_buffer.h"
#include "imath.h"

#include <math.h>

///////////////////////////////////////////////////////////////////////////////////

//
// a simple comlex class,
//
// this is here because I got reports that <g++/Complex.h> is
// not available everywhere
//

class complex {
    float r, i;

public:
    complex() { }
    complex(float r_, float i_)
        : r(r_)
        , i(i_) { }

    float real() const { return r; }
    float imag() const { return i; }

    friend complex operator+(complex& c1, complex& c2);
    friend complex operator-(complex& c1, complex& c2);
    friend complex operator*(complex& c1, complex& c2);
};
complex operator+(complex& c1, complex& c2) { return complex(c1.r + c2.r, c1.i + c2.i); }
complex operator-(complex& c1, complex& c2) { return complex(c1.r - c2.r, c1.i - c2.i); }
complex operator*(complex& c1, complex& c2) {
    return complex(c1.r * c2.r - c1.i * c2.i, c1.r * c2.i + c1.i * c2.r);
}

static int r(int k, int n) {
    int r = 0;
    for (; n > 1; n >>= 1) {
        r = (r << 1) | (k & 1);
        k >>= 1;
    }
    return r;
}

static complex audioProcessorWp[1024];
static int audioProcessorR[1024];
static int audioProcessorFftInit = 0;

static AudioProcessor audioFrameProcessor;

static void initAudioProcessorFft() {
    if (audioProcessorFftInit)
        return;

    for (int i = 0; i < 1024; i++) {
        audioProcessorWp[i] = complex(cos(2.0 * M_PI / double(1024) * double(i)),
            sin(2.0 * M_PI / double(1024) * double(i)));
        audioProcessorR[i] = r(i, 1024);
    }
    audioProcessorFftInit = 1;
}

static AudioFrame* currentAudioFrame() {
    return audioFrameCurrent();
}

void AudioProcessor::none(AudioFrame& frame) {
    none(frame.data, frame.processed);
}

void AudioProcessor::filter1(AudioFrame& frame) {
    filter1(frame.data, frame.processed);
}

void AudioProcessor::filter2(AudioFrame& frame) {
    filter2(frame.data, frame.processed);
}

void AudioProcessor::fft(AudioFrame& frame) {
    fft(frame.data, frame.processed);
}

void AudioProcessor::none(char2* data, char2* processed) {
    memcpy(processed, data, 1024 * sizeof(char2));
}

void AudioProcessor::filter1(char2* data, char2* processed) {
    memcpy(processed, data, 1024 * sizeof(char2));

    int temp = processed[0][1];
    int temp2 = processed[0][0];
    for (int x = 1; x < 1024; x++) {
        if ((processed[x][1] - temp) > 10)
            processed[x][1] = temp + 10;
        else if ((processed[x][1] - temp) < -10)
            processed[x][1] = temp - 10;

        if ((processed[x][0] - temp2) > 10)
            processed[x][0] = temp2 + 10;
        else if ((processed[x][0] - temp2) < -10)
            processed[x][0] = temp2 - 10;

        temp = processed[x][1];
        temp2 = processed[x][0];
    }
}

void AudioProcessor::filter2(char2* data, char2* processed) {
    int temp = data[0][1];
    int temp2 = data[0][0];
    for (int x = 1; x < 1024; x++) {
        processed[x][1] = processed[x - 1][1] + (data[x][1] - temp) / 16;
        processed[x][0] = processed[x - 1][0] + (data[x][0] - temp2) / 16;
        temp2 = processed[x][0];
        temp = processed[x][1];
    }
}

void AudioProcessor::fft(char2* data, char2* processed) {
    int h, k;
    int p;
    int z, zp;
    complex c[1024];

    initAudioProcessorFft();

    for (k = 0; k < 1024; k++)
        c[k] = complex(data[k][0], data[k][1]);

    /* the algorithm I use here is from:
     *
     * The Design and Analysis of Parallel Algorithms
     * Selim G. Akl
     * Prentice Hall, Englewood Clifs New Jersey 076322; 1989
     * ISBN 0-12-700056-3
     * page 244
     */
    p = 1024 / 2;
    z = 1;
    for (h = 1024; h != 0; h >>= 1) {
        zp = 0;
        for (k = 0; k < 1024; k++) {
            if ((k & p) == 0) {
                complex t = c[k] + c[k + p];
                complex t1 = (c[k] - c[k + p]);
                c[k + p] = t1 * audioProcessorWp[zp];
                c[k] = t;
            }
            zp = (zp + z) % 1024;
        }
        p >>= 1;
        z *= 2;
    }

    float a = 2.0 / sqrt(1024);
    for (k = 0; k < 1024; k++) {
        processed[audioProcessorR[k]][0] = int(c[k].real() * a);
        processed[audioProcessorR[k]][1] = int(c[k].imag() * a);
    }
}

class FFT : public CoreOptionEntry {
public:
    FFT()
        : CoreOptionEntry("FFT", "", 1) { }

    int operator()() {
        if (currentAudioFrame())
            audioFrameProcessor.fft(*currentAudioFrame());
        else
            audioFrameProcessor.fft(audioFrameData(), audioFrameProcessedData());
        return 0;
    }
};

class Massage1 : public CoreOptionEntry {
public:
    Massage1()
        : CoreOptionEntry("Filter1", "", 1) { }

    int operator()() {
        if (currentAudioFrame())
            audioFrameProcessor.filter1(*currentAudioFrame());
        else
            audioFrameProcessor.filter1(audioFrameData(), audioFrameProcessedData());
        return 0;
    }
};

class Massage2 : public CoreOptionEntry {
public:
    Massage2()
        : CoreOptionEntry("Filter2", "low pass filter", 1) { }

    int operator()() {
        if (currentAudioFrame())
            audioFrameProcessor.filter2(*currentAudioFrame());
        else
            audioFrameProcessor.filter2(audioFrameData(), audioFrameProcessedData());
        return 0;
    }
};

class NoAudioProcess : public CoreOptionEntry {
public:
    NoAudioProcess()
        : CoreOptionEntry("none", "", 1) { }

    int operator()() {
        if (currentAudioFrame())
            audioFrameProcessor.none(*currentAudioFrame());
        else
            audioFrameProcessor.none(audioFrameData(), audioFrameProcessedData());
        return 0;
    }
};

static CoreOptionEntry* _audioProcessorOptionEntries[]
    = { new NoAudioProcess(), new Massage1(), new Massage2(), new FFT() };
CoreOptionEntryList audioProcessorEntries(_audioProcessorOptionEntries, 4);

AudioProcessingOption audioProcessing("sound-processing", audioProcessorEntries);

AudioProcessingOption::AudioProcessingOption(const char* name, CoreOptionEntryList& entries_)
    : Option(name)
    , entries(entries_) {
    initialEntry[0] = '\0';
}

int AudioProcessingOption::entryCount() const {
    return entries.n();
}

int AudioProcessingOption::optNr(const char* name) const {
    int n = entryCount();

    if (n == 0)
        return 0;

    if ((name == NULL) || (name[0] == '\0'))
        return Random(n);

    for (int i = 0; i < n; i++) {
        CoreOptionEntry* entry = entries[i];
        if ((entry != NULL) && entry->sameName(name))
            return i;
    }

    char* pos;
    int parsed = strtol(name, &pos, 0);
    if (pos == name)
        return Random(n);

    return mod(parsed, n);
}

void AudioProcessingOption::setInitialEntry(const char* entry) {
    if (entry == NULL)
        initialEntry[0] = '\0';
    else {
        strncpy(initialEntry, entry, sizeof(initialEntry));
        initialEntry[sizeof(initialEntry) - 1] = '\0';
    }
}

void AudioProcessingOption::changeToInitial() {
    change(initialEntry);
}

void AudioProcessingOption::change(int by) {
    int n = entryCount();
    if (n == 0)
        return;

    value = mod(value + by, n);
    CTH_DEBUG("changed audio processing to `%s'\n", text());
}

void AudioProcessingOption::change(const char* to) {
    int n = entryCount();
    if (n == 0)
        return;

    value = optNr(to);
    if ((value < 0) || (value >= n))
        value = 0;

    CTH_DEBUG("changed audio processing to `%s'\n", text());
}

const char* AudioProcessingOption::text() const {
    if ((value < 0) || (value >= entryCount()) || (entries[value] == NULL))
        return "unknown";

    return entries[value]->Name();
}

int AudioProcessingOption::process() {
    if ((value < 0) || (value >= entryCount()) || (entries[value] == NULL))
        return 0;

    CTH_TRACE("processing mode=`%s'\n", "audio processing", text());
    return entries[value]->operator()();
}
