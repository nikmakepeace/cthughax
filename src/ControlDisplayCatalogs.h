/** @file
 * Display catalog view used by the generic control IPC layer.
 */

#ifndef CTHUGHA_CONTROL_DISPLAY_CATALOGS_H
#define CTHUGHA_CONTROL_DISPLAY_CATALOGS_H

class ControlDisplayCatalogs {
public:
    virtual ~ControlDisplayCatalogs() { }

    virtual int screenChoiceCount() const = 0;
    virtual const char* screenChoiceNameAt(int index) const = 0;
    virtual const char* screenChoiceLabelAt(int index) const = 0;
    virtual int screenChoiceInUseAt(int index) const = 0;
};

class BuiltInControlDisplayCatalogs : public ControlDisplayCatalogs {
public:
    virtual int screenChoiceCount() const;
    virtual const char* screenChoiceNameAt(int index) const;
    virtual const char* screenChoiceLabelAt(int index) const;
    virtual int screenChoiceInUseAt(int index) const;
};

#endif
