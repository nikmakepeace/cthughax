// -*- C++ -*-
#ifndef __EFFECT_CONTROL_H
#define __EFFECT_CONTROL_H

#include "cthugha.h"
#include "Option.h"

#include <string>

class RandomSource;

//
// Remarks:
//
// * Visual catalog dimensions must be configured before initial values that
//   depend on generated or file-backed entries are applied.
//

class EffectChoice {
protected:
    char* name; // name (short)
    char* desc; // description or long name
    OptionOnOff use; // in use or not
public:
    EffectChoice(const char* n, const char* d, int inUse = 1);
    virtual ~EffectChoice() {
        delete[] name;
        name = NULL;
        delete[] desc;
        desc = NULL;
    }
    virtual int operator()() { return 0; } // do nothing by default

    const char* Name() const { return name; }
    const char* Desc() const { return desc; }
    void setUse(int inUse) { use.setValue(inUse); }
    int inUse() const { return int(use); }
    const char* useText() const { return use.text(); }

    virtual int sameName(const char* other);

    friend class EffectControl;
    friend class InterfaceList;
    friend class activateAction;
};

enum EffectControlFlags {
    EFFECT_CONTROL_NO_FLAGS = 0,

    // This option participates in automatic scene random/all changes.  New
    // EffectControls should opt in deliberately so construction alone does not
    // make them runtime mutation targets.
    EFFECT_CONTROL_AUTO_CHANGE = 1 << 0
};

class OffEntry : public EffectChoice {
public:
    OffEntry(const char* name = "off")
        : EffectChoice(name, "", 1) { }
    virtual int sameName(const char* other);
};

class OnEntry : public EffectChoice {
public:
    OnEntry()
        : EffectChoice("on", "", 1) { }
    virtual int sameName(const char* other);
};

//
// A single linked list for selectable effect choices.
// implementation is not very efficient
//
class EffectChoiceList {
    EffectChoice* entry;
    EffectChoiceList* next;

public:
    EffectChoiceList()
        : entry(NULL)
        , next(NULL) { }
    EffectChoiceList(EffectChoice* e)
        : entry(e)
        , next(NULL) { }
    EffectChoiceList(EffectChoice** e, int n) {
        if (n > 0) {
            entry = e[0];
            next = (n > 1) ? new EffectChoiceList(e + 1, n - 1) : (EffectChoiceList*)NULL;
        } else {
            entry = NULL;
            next = NULL;
        }
    }
    int inList(EffectChoice* e) {
        return (entry == e) ? 1 : ((next == NULL) ? 0 : next->inList(e));
    }

    void add(EffectChoice* e) {
        if (inList(e))
            return;

        if (entry == NULL)
            entry = e;
        else if (next == NULL)
            next = new EffectChoiceList(e);
        else
            next->add(e);
    }

    EffectChoice* operator[](int n) {
        return (n == 0) ? entry : ((next != NULL) ? (*next)[n - 1] : (EffectChoice*)NULL);
    }
    EffectChoice* operator[](int n) const {
        return (n == 0) ? entry : ((next != NULL) ? (*next)[n - 1] : (EffectChoice*)NULL);
    }
    int n() const { return (next == NULL) ? ((entry == NULL) ? 0 : 1) : 1 + next->n(); }
};

class EffectControl : public Option {
protected:
    //
    // buffer this option belongs to
    //
    int buffer;

    //
    // List of all effect controls
    //
    static EffectControl* first;
    EffectControl* next;

    //
    // History
    //
    int* oldValues;
    int history;

    //
    // Choices
    //
    EffectChoiceList& entries;
    int flags;

    std::string initialEntry;

    void doSave();
    void doRestore();

protected:
    int isAutoChangeCandidate() const;

public:
    EffectControl(int buffer, const char* name, EffectChoiceList& e,
        int flags = EFFECT_CONTROL_NO_FLAGS);

    EffectControl& operator=(const EffectControl& other);

    void setInitialEntry(const char* i) { initialEntry = (i != NULL) ? i : ""; }

    virtual const char* name() const;

    //
    // Changeing
    //
    OptionOnOff lock; // individual lock

    static void changeToInitial();

    /**
     * Changes this option from text using the legacy process random fallback
     * for empty or invalid random-selection text.
     *
     * @param to Choice name, number, lock-prefixed choice, or empty for random.
     * @param doSave Nonzero to save the previous option value first.
     */
    virtual void change(const char* to, int doSave = 1);

    /**
     * Changes this option from text using an injected random source for empty
     * or invalid random-selection text.
     *
     * @param to Choice name, number, lock-prefixed choice, or empty for random.
     * @param randomSource Random source used for fallback selection.
     * @param doSave Nonzero to save the previous option value first.
     */
    virtual void change(const char* to, RandomSource& randomSource, int doSave = 1);

    /**
     * Moves this option by a relative number of entries.
     *
     * @param by Relative entry delta.
     * @param doSave Nonzero to save the previous option value first.
     */
    virtual void change(int by, int doSave = 1);

    /**
     * Selects a random usable entry using the legacy process random fallback.
     *
     * @param save_ Nonzero to save the previous option value first.
     */
    virtual void changeRandom(int save_ = 1);

    /**
     * Selects a random usable entry using an injected random source.
     *
     * @param randomSource Random source used to select the candidate entry.
     * @param save_ Nonzero to save the previous option value first.
     */
    virtual void changeRandom(RandomSource& randomSource, int save_ = 1);

    /**
     * Resolves an entry name or number using the legacy process random fallback
     * for empty or invalid text.
     *
     * @param n Entry name, number, or empty for random.
     * @return Resolved entry index, or 0 for an empty option.
     */
    int optNr(const char* n);

    /**
     * Resolves an entry name or number using an injected random source for
     * empty or invalid text.
     *
     * @param n Entry name, number, or empty for random.
     * @param randomSource Random source used for fallback selection.
     * @return Resolved entry index, or 0 for an empty option.
     */
    int optNr(const char* n, RandomSource& randomSource);

    void change(int) { CTH_ERROR("internal error. wrong change called for option `%s'.\n", name()); }
    void change(const char*) {
        CTH_ERROR("internal error. wrong change called for option `%s'.\n", name());
    }

    /**
     * Randomly changes one auto-change candidate using the legacy process
     * random fallback.
     *
     * @return Changed option, or NULL when no unlocked candidate can change.
     */
    static EffectControl* changeOne();

    /**
     * Randomly changes one auto-change candidate using an injected random
     * source.
     *
     * @param randomSource Random source used for candidate and entry selection.
     * @return Changed option, or NULL when no unlocked candidate can change.
     */
    static EffectControl* changeOne(RandomSource& randomSource);

    /** Randomly changes all auto-change candidates using the legacy fallback. */
    static void changeAll();

    /**
     * Randomly changes all auto-change candidates using an injected random
     * source.
     *
     * @param randomSource Random source used for entry selection.
     */
    static void changeAll(RandomSource& randomSource);

    virtual const char* text() const;

    //
    // Choices
    //
    virtual const char* text(int i) const { // get name of entry i
        if ((i < 0) || (i >= getNEntries()))
            return "unknown";
        return entries[i]->name;
    }
    void add(EffectChoice*); // add a new choice
    void add(EffectChoice**, int nEntries); // add several new choices
    int defined(const char* name); // check if a choice is already defined

    virtual int operator()() { // do the action of the current entry
        if ((value < 0) || (value >= getNEntries()))
            return 0;
        return (entries[value]->operator()());
    }

    EffectChoice* operator[](int i) { // get the i-th entry
        return entries[i];
    }
    EffectChoice* current() { // get the current entry
        return operator[](value);
    }
    int currentN() const { // get current value
        return value;
    }
    const char* currentName() const { // return current name
        if ((value < 0) || (value >= getNEntries()))
            return "unknown";
        return entries[value]->name;
    }
    const char* currentDesc() const { // return current description
        if ((value < 0) || (value >= getNEntries()))
            return "";
        return entries[value]->desc;
    }
    int getNEntries() const { return entries.n(); }

    /** @return Mutable legacy choice list backing this control. */
    EffectChoiceList& choiceList() { return entries; }

    /** @return Immutable legacy choice list backing this control. */
    const EffectChoiceList& choiceList() const { return entries; }

    int autoChangeEnabled() const { return (flags & EFFECT_CONTROL_AUTO_CHANGE) != 0; }
    int bufferIndex() const { return buffer; }
    EffectControl* nextRegistered() const { return next; }

    //
    // control History
    //

    static void save();
    static void restore();

    static EffectControl* firstRegistered();

    friend class EffectRegistry;
    friend class InterfaceList;
    friend class activateAction;
};

extern EffectControl screen;

#endif
