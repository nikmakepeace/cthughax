/*
 *  sound-display (wave-functions)
 */
#ifndef __WAVES_H
#define __WAVES_H

#include "cthugha.h"
#include "CoreOption.h"

class CthughaBuffer;
class VisualFrameContext;

class WaveEntry : public CoreOptionEntry {
    void (*wave)(CthughaBuffer& buffer);

public:
    WaveEntry(void (*f)(CthughaBuffer& buffer), const char* name, const char* desc, int inUse = 1);

    int operator()();
    int operator()(CthughaBuffer& buffer);
    void execute(CthughaBuffer& buffer, const VisualFrameContext& context);
};

int init_tables();
int init_wave();

extern CoreOptionEntry* _waves[];
extern int _nWaves;

typedef unsigned char pal_table[256]; /* Table for display_wave */
extern pal_table tables[]; /* Palette-Tables */
extern int nr_tables; /* number of tables */

typedef int WObject[2][3]; // a 3D object is a list of lines, each line
                           // is two 3-space points.
                           // the list is terminated by the final line having
                           // coordinates that are all -1

extern OptionOnOff use_objects; /* use 3-D objects */

#endif
