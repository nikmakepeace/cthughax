// -*- c++ -*-

#ifndef __PCX_H
#define __PCX_H

#include "cthugha.h"
#include "CoreOption.h"

class CthughaBuffer;

class PCXEntry : public CoreOptionEntry {

public:
    unsigned char* data;
    int pal; // number of the palette
    int width, height;

    PCXEntry(const char* name, const char* desc)
        : CoreOptionEntry(name, desc)
        , data(NULL)
        , pal(0)
        , width(0)
        , height(0) { }
    PCXEntry(FILE* file, const char* name);
    ~PCXEntry() {
        delete[] data;
        data = NULL;
    }

    int overlay(CthughaBuffer& buffer) const;

    friend CoreOptionEntry* read_pcx(FILE*, const char*, const char*);
};

extern CoreOptionEntryList pcxEntries;
class OptionPCX : public CoreOption {
public:
    OptionPCX(int b, const char* n)
        : CoreOption(b, n, pcxEntries) { }
};

#endif
