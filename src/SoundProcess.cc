#include "cthugha.h"
#include "AudioFrame.h"
#include "Interface.h"
#include "display.h"
#include "cth_buffer.h"
#include "imath.h"
#include "CthughaBuffer.h"

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

class FFT : public CoreOptionEntry {
    static complex wp[1024];
    static int R[1024];
    static int wpInit;

public:
    FFT()
        : CoreOptionEntry("FFT", "", 1) {
        if (wpInit == 0) {
            for (int i = 0; i < 1024; i++) {
                wp[i] = complex(cos(2.0 * M_PI / double(1024) * double(i)),
                    sin(2.0 * M_PI / double(1024) * double(i)));

                R[i] = r(i, 1024);
            }
            wpInit = 1;
        }
    }

    int operator()() {
        int h, k;
        int p;
        int z, zp;
        complex c[1024];

        for (k = 0; k < 1024; k++) { /* feed input */
            // use sound data as input (left -> real, right -> imag)
            c[k] = complex(audioFrameData()[k][0], audioFrameData()[k][1]);
        }

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
                    complex t1 = (c[k] - c[k + p]); // Why is this necessary?
                    c[k + p] = t1 * wp[zp];
                    c[k] = t;
                }
                zp = (zp + z) % 1024;
            }
            p >>= 1;
            z *= 2;
        }

        float a = 2.0 / sqrt(1024); /* normalize result, store back */
        for (k = 0; k < 1024; k++) {
            // get back FFTed sound data (real -> left, imag -> right)
            audioFrameProcessedData()[R[k]][0] = int(c[k].real() * a);
            audioFrameProcessedData()[R[k]][1] = int(c[k].imag() * a);
        }
        return 0;
    }
};
complex FFT::wp[1024];
int FFT::R[1024];
int FFT::wpInit = 0;

class Massage1 : public CoreOptionEntry {
public:
    Massage1()
        : CoreOptionEntry("Filter1", "", 1) { }

    int operator()() {
        memcpy(audioFrameProcessedData(), audioFrameData(), 1024 * sizeof(char2));

        int temp = audioFrameProcessedData()[0][1];
        int temp2 = audioFrameProcessedData()[0][0];
        for (int x = 1; x < 1024; x++) {
            if ((audioFrameProcessedData()[x][1] - temp) > 10) {
                audioFrameProcessedData()[x][1] = temp + 10;
            } else if ((audioFrameProcessedData()[x][1] - temp) < -10) {
                audioFrameProcessedData()[x][1] = temp - 10;
            }
            if ((audioFrameProcessedData()[x][0] - temp2) > 10) {
                audioFrameProcessedData()[x][0] = temp2 + 10;
            } else if ((audioFrameProcessedData()[x][0] - temp2) < -10) {
                audioFrameProcessedData()[x][0] = temp2 - 10;
            }
            temp = audioFrameProcessedData()[x][1];
            temp2 = audioFrameProcessedData()[x][0];
        }
        return 0;
    }
};

class Massage2 : public CoreOptionEntry {
public:
    Massage2()
        : CoreOptionEntry("Filter2", "low pass filter", 1) { }

    int operator()() {
        int temp = audioFrameData()[0][1];
        int temp2 = audioFrameData()[0][0];
        for (int x = 1; x < 1024; x++) {
            audioFrameProcessedData()[x][1]
                = audioFrameProcessedData()[x - 1][1] + (audioFrameData()[x][1] - temp) / 16;
            audioFrameProcessedData()[x][0]
                = audioFrameProcessedData()[x - 1][0] + (audioFrameData()[x][0] - temp2) / 16;
            temp2 = audioFrameProcessedData()[x][0];
            temp = audioFrameProcessedData()[x][1];
        }
        return 0;
    }
};

class NoSoundProcess : public CoreOptionEntry {
public:
    NoSoundProcess()
        : CoreOptionEntry("none", "", 1) { }

    int operator()() {
        memcpy(audioFrameProcessedData(), audioFrameData(), 1024 * sizeof(char2));
        return 0;
    }
};

static CoreOptionEntry* _soundProcessEntries[]
    = { new NoSoundProcess(), new Massage1(), new Massage2(), new FFT() };
CoreOptionEntryList soundProcessEntries(_soundProcessEntries, 4);
