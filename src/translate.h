// -*-c++-*-
//
//    CTHUGHA-L							translate.h
//

#ifndef __TRANSLATE_H__
#define __TRANSLATE_H__

#include "cthugha.h"
#include "CoreOption.h"

class CthughaBuffer;
class VisualFrameContext;

class TranslateEntry : public CoreOptionEntry {

    int loadLine(FILE* in, int n);

public:
    int* trans;

    static char cmdRead[PATH_MAX];

    char command[PATH_MAX];

    TranslateEntry(const char* name, const char* desc)
        : CoreOptionEntry(name, desc)
        , trans(NULL) {
        command[0] = '\0';
    }
    virtual ~TranslateEntry() {
        delete[] trans;
        trans = NULL;
    }

    int operator()(CthughaBuffer& buffer);
    void execute(CthughaBuffer& buffer, const VisualFrameContext& context);

    static CoreOptionEntry* loaderCmd(FILE*, const char*, const char*, const char*);
    static CoreOptionEntry* loaderTab(FILE*, const char*, const char*, const char*);

    friend class TranslateOption;
};

class TranslateOption : public CoreOption {

    //
    // variables for load on demand
    //
    int* lodCommonTrans;
    FILE* lodPipe;
    int lodPID;
    int lodLine;
    int lodCurrent;

    int openPipe(const char* command);

public:
    TranslateOption(int buffer, const char* name);

    int prepareCurrentEntry(TranslateEntry*& entry);

    virtual const char* status();
};

int init_translate();

extern OptionOnOff use_translates; /* allow translations */
extern OptionOnOff trans_stretch; /* allow stretching */
extern OptionOnOff transLoadOnDemand;
extern OptionOnOff transLoadLate;

#endif
