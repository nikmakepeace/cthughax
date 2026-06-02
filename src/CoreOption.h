// -*- C++ -*-
#ifndef __COREOPTION_H
#define __COREOPTION_H

#include "cthugha.h"
#include "Option.h"

//
// Remarks:
//
// * CthughaBuffer must be inidializes BEFORE any initial values
//   are set for the CoreOptions
//

class CoreOptionEntry {
protected:
    char* name; // name (short)
    char* desc; // description or long name
    OptionOnOff use; // in use or not
public:
    CoreOptionEntry(const char* n, const char* d, int inUse = 1);
    virtual ~CoreOptionEntry() {
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

    virtual int sameName(const char* other);

    friend class CoreOption;
    friend class InterfaceList;
    friend class activateAction;
};

typedef CoreOptionEntry* (*CoreOptionLoader)(FILE*, const char*, const char*, const char*);
typedef CoreOptionEntry* (*CoreOptionContextLoader)(
    FILE*, const char*, const char*, const char*, void*);

enum CoreOptionFlags {
    CORE_OPTION_NO_FLAGS = 0,

    // This option participates in AutoChanger's random/all changes.  New
    // CoreOptions should opt in deliberately so construction alone does not
    // make them runtime mutation targets.
    CORE_OPTION_AUTO_CHANGE = 1 << 0
};

class OffEntry : public CoreOptionEntry {
public:
    OffEntry(const char* name = "off")
        : CoreOptionEntry(name, "", 1) { }
    virtual int sameName(const char* other);
};

class OnEntry : public CoreOptionEntry {
public:
    OnEntry()
        : CoreOptionEntry("on", "", 1) { }
    virtual int sameName(const char* other);
};

//
// a single linked list for CoreOptionEntries
// implementation is not very efficient
//
class CoreOptionEntryList {
    CoreOptionEntry* entry;
    CoreOptionEntryList* next;

public:
    CoreOptionEntryList()
        : entry(NULL)
        , next(NULL) { }
    CoreOptionEntryList(CoreOptionEntry* e)
        : entry(e)
        , next(NULL) { }
    CoreOptionEntryList(CoreOptionEntry** e, int n) {
        if (n > 0) {
            entry = e[0];
            next = (n > 1) ? new CoreOptionEntryList(e + 1, n - 1) : (CoreOptionEntryList*)NULL;
        } else {
            entry = NULL;
            next = NULL;
        }
    }
    int inList(CoreOptionEntry* e) {
        return (entry == e) ? 1 : ((next == NULL) ? 0 : next->inList(e));
    }

    void add(CoreOptionEntry* e) {
        if (inList(e))
            return;

        if (entry == NULL)
            entry = e;
        else if (next == NULL)
            next = new CoreOptionEntryList(e);
        else
            next->add(e);
    }

    CoreOptionEntry* operator[](int n) {
        return (n == 0) ? entry : ((next != NULL) ? (*next)[n - 1] : (CoreOptionEntry*)NULL);
    }
    CoreOptionEntry* operator[](int n) const {
        return (n == 0) ? entry : ((next != NULL) ? (*next)[n - 1] : (CoreOptionEntry*)NULL);
    }
    int n() const { return (next == NULL) ? ((entry == NULL) ? 0 : 1) : 1 + next->n(); }
};

class CoreOption : public Option {
protected:
    //
    // buffer this option belongs to
    //
    int buffer;

    //
    // List of all core options
    //
    static CoreOption* first;
    CoreOption* next;

    //
    // History and Hot Values
    //
    int* oldValues;
    int history;
    int* hot;

    //
    // Entries
    //
    CoreOptionEntryList& entries;
    int flags;

    char initialEntry[256];

    void doSave();
    void doRestore();

    void getIniUsage();
    void putIniUsage();

    //
    // general loading
    //
protected:
    int isCompressed(const char* name);
    int hasExtension(const char* name, const char* extension);
    int isAutoChangeCandidate() const;

    CoreOptionEntry* load(const char* name, char* total_name, const char* dir,
        CoreOptionLoader loader);
    CoreOptionEntry* load(const char* name, char* total_name, const char* dir,
        CoreOptionContextLoader loader, void* context);
    void loadDir(const char* dir, const char* extension,
        CoreOptionLoader loader);
    void loadDir(const char* dir, const char* extension,
        CoreOptionContextLoader loader, void* context);

public:
    int load(const char* searchPath[], const char* extraPath, const char* extension,
        CoreOptionLoader loader);
    int load(const char* searchPath[], const char* extraPath, const char* extension,
        CoreOptionContextLoader loader, void* context);

public:
    CoreOption(int buffer, const char* name, CoreOptionEntryList& e,
        int flags = CORE_OPTION_NO_FLAGS);

    CoreOption& operator=(const CoreOption& other);

    void setInitialEntry(const char* i) { strncpy(initialEntry, i, 256); }

    virtual const char* name() const;

    //
    // Changeing
    //
    OptionOnOff lock; // individual lock

    static void changeToInitial();
    virtual void change(const char* to, int doSave = 1);
    virtual void change(int by, int doSave = 1);
    virtual void changeRandom(int save_ = 1);

    int optNr(const char* n);

    void change(int) { CTH_ERROR("internal error. wrong change called for option `%s'.\n", name()); }
    void change(const char*) {
        CTH_ERROR("internal error. wrong change called for option `%s'.\n", name());
    }

    static CoreOption* changeOne();
    static void changeAll();

    virtual const char* text() const;

    //
    // Entries
    //
    virtual const char* text(int i) const { // get name of entry i
        if ((i < 0) || (i >= getNEntries()))
            return "unknown";
        return entries[i]->name;
    }
    void add(CoreOptionEntry*); // add a new entry
    void add(CoreOptionEntry**, int nEntries); // add several new entries
    int defined(const char* name); // check if entry is already defines

    virtual int operator()() { // do the action of the current entry
        if ((value < 0) || (value >= getNEntries()))
            return 0;
        return (entries[value]->operator()());
    }

    CoreOptionEntry* operator[](int i) { // get the i-th entry
        return entries[i];
    }
    CoreOptionEntry* current() { // get the current entry
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
    int autoChangeEnabled() const { return (flags & CORE_OPTION_AUTO_CHANGE) != 0; }

    //
    // control History and Hot Values
    //

    static void save();
    static void restore();

    static void save(int to);
    static void restore(int from);

    //
    // access ini file
    //
    static void putIniInitials(); // put current values
    static void getIniInitials(); // get initial values

    static void getHotIni();
    static void putHotIni();

    static void getIniUsages();
    static void putIniUsages();
    static int isIniEntry(const char* entry);

    friend class InterfaceList;
    friend class activateAction;
};

extern CoreOption screen;

#endif
