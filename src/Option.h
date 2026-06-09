// -*- C++ -*-

#ifndef __OPTION_H
#define __OPTION_H

#include "defaults.h"

#include <stdio.h>

//
// class Option
//
class Option {
protected:
    const char* _name;

    void doSave();
    void doRestore();

public:
    int value; // this is only public for the initialization

    Option(const char* n)
        : _name(n)
        , value(DEFAULT_OPTION_VALUE) { }

    virtual void change(int by) = 0;
    virtual void change(const char* to) = 0;

    virtual void setValue(int value_) { // set value without any checks
        value = value_;
    }

    virtual const char* name() const { return _name; }
    virtual const char* text() const = 0;
    virtual operator int() const { return value; }

    virtual ~Option() = 0;
};

//
// Integer and Time Options
//
// -> OptionInt.cc
//

class OptionInt : public Option {
protected:
    // minValue <= value                   for maxValue == 0
    // minValue <= value < maxValue        for maxValue != 0
    int maxValue;
    int minValue;

public:
    OptionInt(const char* name, int iV, int maxV = DEFAULT_OPTION_INT_NO_MAX,
        int minV = DEFAULT_OPTION_INT_MIN)
        : Option(name)
        , maxValue(maxV)
        , minValue(minV) {
        setValue(iV);
    }

    virtual void setValue(int value_);
    void setMaxValue(int value_) { maxValue = value_; }

    virtual void change(int by);
    virtual void change(const char* to);

    virtual const char* text() const {
        static char s[64];
        snprintf(s, sizeof(s), "%7d", value);
        return s;
    }
};

class OptionTime : public OptionInt {
protected:
public:
    OptionTime(const char* name, int v)
        : OptionInt(name, v) { }
    virtual const char* text() const {
        static char s[64];
        snprintf(s, sizeof(s), "%5.2f sec", double(value) / 1000.0);
        return s;
    }
    virtual void change(const char* to); // also accept "sec"
};

//
// On/Off value
//
class OptionOnOff : public OptionInt {
protected:
public:
    OptionOnOff(const char* name, int v)
        : OptionInt(name, v, OPTION_ON_OFF_MAX_EXCLUSIVE) { }
    virtual const char* text() const { return value ? (char*)" on" : (char*)"off"; }
    virtual void change(int by) { OptionInt::change(by); }
    virtual void change(const char* to);
};

class OptionDummy : public Option {
public:
    OptionDummy()
        : Option("dummy") { }
    virtual const char* text() const { return ""; }
    virtual void change(const char* /* to */) { }
    virtual void change(int /* by */) { }
};
extern OptionDummy optionDummy;

#endif
