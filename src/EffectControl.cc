#include "cthugha.h"
#include "EffectControl.h"
#include "ProcessServices.h"

#include <string>

static const int visualBufferCount = 1;

EffectControl* EffectControl::first = NULL;

const int MAX_HISTORY = 128;

static int modInt(int value, int modulo) {
    int result = value % modulo;
    return result < 0 ? result + modulo : result;
}

class EffectControlFallbackRandomSource : public RandomSource {
    unsigned int stateValue;

public:
    EffectControlFallbackRandomSource()
        : stateValue(0x6d2b79f5u) { }

    virtual int uniformInt(int exclusiveMax) {
        if (exclusiveMax <= 1)
            return 0;

        stateValue = stateValue * 1664525u + 1013904223u;
        return int((stateValue >> 1) % unsigned(exclusiveMax));
    }
};

static EffectControlFallbackRandomSource fallbackRandomSource;

EffectControl::EffectControl(int b, const char* n, EffectChoiceList& e, int flags_)
    : Option(n)
    , buffer(b)
    , entries(e)
    , flags(flags_)
    , lock("", 0) {

    // keep in linked list
    next = first;
    first = this;

    // history
    oldValues = new int[MAX_HISTORY];
    history = 0;

}

EffectControl& EffectControl::operator=(const EffectControl& other) {
    entries = other.entries;

    return *this;
}

const char* EffectControl::name() const {
    static char str[512];

    if (buffer < 0)
        return _name;

    snprintf(str, sizeof(str), "%s.%d", _name, buffer);
    return str;
}

//
// Changeing
//
void EffectControl::changeToInitial() {
    for (EffectControl* o = first; o != NULL; o = o->next) {
        o->change(o->initialEntry.c_str(), 0);
    }
}

void EffectControl::change(int by, int doSave) {

    if (doSave)
        save();

    // check for empty set
    if (getNEntries() == 0)
        return;

    /* check, if there is any feature we can use */
    int nouse = 1;
    for (int i = 0; i < getNEntries(); i++)
        if (int(entries[i]->use)) {
            i = getNEntries();
            nouse = 0;
        }

    if (nouse) { // if none in use, use the first
        value = 0;
    } else {
        value = modInt(value + by, getNEntries());
        if (by < 0) {
            while (!int(entries[value]->use))
                value = modInt(value - 1, getNEntries());
        } else
            while (!int(entries[value]->use))
                value = modInt(value + 1, getNEntries());
    }

    CTH_DEBUG("changed option `%s' to `%s'\n", name(), entries[value]->name);

}

void EffectControl::changeRandom(int doSave) {
    changeRandom(fallbackRandomSource, doSave);
}

void EffectControl::changeRandom(RandomSource& randomSource, int doSave) {
    if (lock)
        return;
    if (getNEntries() == 0)
        return;
    value = randomSource.uniformInt(getNEntries()); // select a desired value
    change(0, doSave); // change to next usable value, do saving
}

void EffectControl::change(const char* to, int doSave) {
    change(to, fallbackRandomSource, doSave);
}

void EffectControl::change(const char* to, RandomSource& randomSource, int doSave) {
    char* pos;

    // check, if there is something we can set this to
    if (getNEntries() == 0) {
        return;
    }

    /* if empty, set to a random value */
    if ((to == NULL) || (to[0] == '\0')) {
        CTH_DEBUG("changing option `%s' to a random value.\n", name());
        value = randomSource.uniformInt(getNEntries());
        change(0, 0);
        return;
    }

    CTH_DEBUG("changing option `%s' to `%s'.\n", name(), to);

    if (doSave)
        save();

    // possible prefixes to tun on locking
    static const char* Lon[] = { "l:", "lock:", "locked:" };
    static int nLon = 3;
    // possible prefixes to turn off locking
    static const char* Loff[] = { "no-l:", "no-lock:", "no-locked:", "nol:", "nolock:", "nolocked:",
        "non-l:", "non-lock:", "non-locked:", "nonl:", "nonlock:", "nonlocked:" };
    static int nLoff = sizeof(Loff) / sizeof(const char*);

    // check if tun on prefix is given
    for (int i = 0; i < nLon; i++) {
        if (strncasecmp(to, Lon[i], strlen(Lon[i])) == 0) {
            lock.change("on");
            to = to + strlen(Lon[i]);
            i = nLon;
        }
    }
    // check if turn off prefix is given
    for (int i = 0; i < nLoff; i++) {
        if (strncasecmp(to, Loff[i], strlen(Loff[i])) == 0) {
            lock.change("off");
            to = to + strlen(Loff[i]);
            i = nLoff;
        }
    }

    // try to find it as a name
    for (value = 0; value < getNEntries(); value++)
        if (entries[value]->sameName(to)) { // found tha name
            entries[value]->use.setValue(1);
            return;
        }

    // not a name, try as number
    value = strtol(to, &pos, 0);

    if (pos == to) { // not a number
        /* found no entry, use a random value */
        CTH_WARN("Unknown entry `%s' for option `%s'\n", to, name());
        value = randomSource.uniformInt(getNEntries());
    } else { // it is a number
        value = modInt(value, getNEntries());
    }

    entries[value]->use.setValue(1);
}

//
// convert an option name to a number
//
int EffectControl::optNr(const char* n) {
    return optNr(n, fallbackRandomSource);
}

int EffectControl::optNr(const char* n, RandomSource& randomSource) {
    char* pos;

    // check, if there is something we can set this to
    if (getNEntries() == 0) {
        return 0;
    }

    /* if empty, set to a random value */
    if ((n == NULL) || (n[0] == '\0')) {
        return randomSource.uniformInt(getNEntries());
    }

    // try to find it as a name
    for (int v = 0; v < getNEntries(); v++)
        if (entries[v]->sameName(n)) { // found tha name
            return v;
        }

    // not a name, try as number
    int v = strtol(n, &pos, 0);

    if (pos == n) { // not a number
        return randomSource.uniformInt(getNEntries());
    } else {
        return modInt(v, getNEntries());
    }
}

//
// change randomly one of the features
//
EffectControl* EffectControl::changeOne() {
    return changeOne(fallbackRandomSource);
}

EffectControl* EffectControl::changeOne(RandomSource& randomSource) {

    int nCandidates = 0;
    for (EffectControl* o = first; o != NULL; o = o->next) {
        if (o->isAutoChangeCandidate())
            nCandidates++;
    }

    if (nCandidates == 0)
        return 0;

    int n = randomSource.uniformInt(nCandidates);
    EffectControl* o = EffectControl::first;
    while (o != NULL) {
        if (o->isAutoChangeCandidate()) {
            if (n == 0)
                break;
            n--;
        }
        o = o->next;
    }

    if (o == NULL) {
        CTH_ERROR("internal error: no EffectControl found among %d autochange candidates\n",
            nCandidates);
        return 0;
    }

    if (o->lock)
        return 0;
    o->changeRandom(randomSource, 1);
    return o;
}

//
// change all the features
//
void EffectControl::changeAll() {
    changeAll(fallbackRandomSource);
}

void EffectControl::changeAll(RandomSource& randomSource) {

    save();

    for (EffectControl* o = first; o != NULL; o = o->next) {
        if (o->isAutoChangeCandidate())
            o->changeRandom(randomSource, 0);
    }
}

int EffectControl::isAutoChangeCandidate() const {
    if (!autoChangeEnabled())
        return 0;

    return (buffer < 0) || (buffer < visualBufferCount);
}

const char* EffectControl::text() const {
    static std::string text;

    text.clear();
    if (lock)
        text = "locked:";

    text += currentName();

    if (currentDesc()[0] != '\0') {
        text += " (";
        text += currentDesc();
        text += ")";
    }

    return text.c_str();
}

//
// save the current state
//
void EffectControl::save() {
    for (EffectControl* o = first; o != NULL; o = o->next)
        o->doSave();
}
void EffectControl::doSave() {
    if (history >= MAX_HISTORY) {
        memcpy(oldValues, oldValues + 1, sizeof(int) * (MAX_HISTORY - 1));
        history--;
    }
    oldValues[history] = value;
    history++;
}

//
// get back the last state
//
void EffectControl::restore() {
    for (EffectControl* o = first; o != NULL; o = o->next)
        o->doRestore();
}
void EffectControl::doRestore() {
    if (history > 0) {
        history--;
        value = int(oldValues[history]);
        this->change(0, 0);
    }
}

EffectControl* EffectControl::firstRegistered() { return first; }

//
// add a new entry to the list
//
void EffectControl::add(EffectChoice* entry) {
    entries.add(entry);
}
void EffectControl::add(EffectChoice** entries, int nEntries) {
    for (int i = 0; i < nEntries; i++)
        add(entries[i]);
}

//
// check if an entry is already defines
//
int EffectControl::defined(const char* name) {

    for (int i = 0; i < getNEntries(); i++)
        if (strcmp(entries[i]->name, name) == 0)
            return 1;
    return 0;
}
