/** @file
 * FFT processor implementations for visualization audio frames.
 */

#include "AudioFftProcessor.h"

#include <math.h>
#include <stdint.h>

namespace {

static int bitReverseIndex(int k, int n) {
    int reversed = 0;
    for (; n > 1; n >>= 1) {
        reversed = (reversed << 1) | (k & 1);
        k >>= 1;
    }
    return reversed;
}

struct Int16Complex {
    int16_t real;
    int16_t imag;
};

static int16_t q15FromUnit(double value) {
    int scaled = int(value * 32767.0 + (value >= 0.0 ? 0.5 : -0.5));
    if (scaled > 32767)
        return 32767;
    if (scaled < -32768)
        return -32768;
    return int16_t(scaled);
}

static int32_t shiftRightTowardZero(int32_t value, int bits) {
    int32_t bias = (value >> 31) & ((1 << bits) - 1);
    return (value + bias) >> bits;
}

}

AudioFftProcessor::~AudioFftProcessor() { }

FixedPointAudioFftProcessor::FixedPointAudioFftProcessor() {
    for (int i = 0; i < 512; i++) {
        double radians = 2.0 * M_PI / double(1024) * double(i);
        cosTable[i] = q15FromUnit(cos(radians));
        sinTable[i] = q15FromUnit(sin(radians));
    }

    for (int i = 0; i < 1024; i++)
        bitReversal[i] = uint16_t(bitReverseIndex(i, 1024));
}

void FixedPointAudioFftProcessor::transform(
    const char2* raw, char2* processedWaveData) const {
    Int16Complex c[1024];

    for (int i = 0; i < 1024; i++) {
        c[i].real = int16_t(int(raw[i][0]) << 7);
        c[i].imag = int16_t(int(raw[i][1]) << 7);
    }

    int twiddleStep = 1;
    for (int halfM = 512; halfM > 0; halfM >>= 1) {
        int m = halfM << 1;

        for (int block = 0; block < 1024; block += m) {
            for (int j = 0; j < halfM; j++) {
                int k = block + j;
                int edge = k + halfM;
                int tableIndex = j * twiddleStep;
                int16_t wr = cosTable[tableIndex];
                int16_t wi = sinTable[tableIndex];
                int32_t baseReal = c[k].real;
                int32_t baseImag = c[k].imag;
                int32_t edgeReal = c[edge].real;
                int32_t edgeImag = c[edge].imag;
                int32_t diffReal = baseReal - edgeReal;
                int32_t diffImag = baseImag - edgeImag;
                int32_t tr = shiftRightTowardZero(
                    diffReal * wr - diffImag * wi, 15);
                int32_t ti = shiftRightTowardZero(
                    diffReal * wi + diffImag * wr, 15);

                c[k].real = int16_t(shiftRightTowardZero(baseReal + edgeReal, 1));
                c[k].imag = int16_t(shiftRightTowardZero(baseImag + edgeImag, 1));
                c[edge].real = int16_t(shiftRightTowardZero(tr, 1));
                c[edge].imag = int16_t(shiftRightTowardZero(ti, 1));
            }
        }

        twiddleStep <<= 1;
    }

    for (int k = 0; k < 1024; k++) {
        int target = bitReversal[k];
        processedWaveData[target][0] = char(c[k].real);
        processedWaveData[target][1] = char(c[k].imag);
    }
}
