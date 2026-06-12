/** @file
 * wx control panel frame that mirrors visualiser-owned runtime state.
 */

#ifndef CTHUGHA_WX_PANEL_FRAME_H
#define CTHUGHA_WX_PANEL_FRAME_H

#include "CthughaPanelBase.h"
#include "ControlPanelClient.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <wx/timer.h>

class wxRearrangeCtrl;

class CthughaPanelFrame : public CthughaPanelBase {
    struct LabelFlashState {
        wxWindow* surface;
        wxStaticText* label;
        wxColour baseBackground;
        int remainingTicks;

        LabelFlashState();
        LabelFlashState(wxWindow* surface_, wxStaticText* label_,
            const wxColour& baseBackground_);
    };

    std::unique_ptr<ControlPanelClient> client;
    wxTimer pollTimer;
    std::map<std::string, std::vector<std::string> > catalogNames;
    std::map<std::string, LabelFlashState> labelFlashStates;
    wxRearrangeCtrl* filterchainRearrangeCtrl;
    int receivedState;
    int everConnected;
    int updatingControls;

    void setConnectionStatus(const wxString& text);
    void repairGeneratedLayout();
    void replaceFilterchainListBox();
    void initializeLabelFlashes();
    void registerFlashLabel(const char* key, const char* label);
    wxStaticText* findStaticTextByLabel(const wxString& label) const;
    wxWindow* wrapFlashLabel(wxStaticText* label);
    void flashLabel(const char* key);
    void updateLabelFlashes();
    void applyLabelFlash(LabelFlashState& state);
    void restoreLabelFlash(LabelFlashState& state);
    void flashIfChanged(
        const char* key, const std::string& before, const std::string& after);
    void flashIfChanged(const char* key, int before, int after);
    void bindControlEvents();
    void setControlsEnabled(int enabled);
    void updateEnabledState();
    void bringToForeground();
    void onPollTimer(wxTimerEvent& event);
    void onChoiceChanged(wxCommandEvent& event);
    void onLockChanged(wxCommandEvent& event);
    void onFlashlightChanged(wxCommandEvent& event);
    void onAutoChangeModeChanged(wxCommandEvent& event);
    void onFireThresholdChanged(wxCommandEvent& event);
    void onFireSensitivityChanged(wxCommandEvent& event);
    void onPaletteSmoothingChanged(wxCommandEvent& event);
    void onMaxFpsChanged(wxCommandEvent& event);
    void onFilterchainReordered(wxCommandEvent& event);
    void onFilterchainChecked(wxCommandEvent& event);

    void handleClientEvent(const ControlPanelClientEvent& event);
    void handleProtocolMessage(const ControlJsonValue& message);
    void applyCatalogs(const ControlJsonValue& message);
    void applyState(const ControlJsonValue& message);
    void applyChoiceState(
        const char* target, wxChoice* choice, const std::string& value);
    void applyCheckBoxState(
        const char* key, wxCheckBox* checkBox, int checked);
    void applySliderState(const char* key, wxSlider* slider, int value);
    void applyLocks(const ControlJsonValue* locks);
    void applyAutoChangeMode(const ControlJsonValue* autoChange);
    std::string currentAutoChangeMode() const;
    std::string autoChangeModeOf(const ControlJsonValue* autoChange) const;
    void updateSliderText(wxSlider* slider, wxStaticText* text);
    void updatePercentSliderText(wxSlider* slider, wxStaticText* text);
    void updateSliderTexts();

    void updateCatalogForTarget(const char* target, wxChoice* choice,
        const ControlJsonValue& targets);
    void selectChoiceValue(const char* target, wxChoice* choice,
        const std::string& value);
    void updateFireLevel(int cumulativeFireLevel, int threshold);
    std::string currentChoiceValue(const char* target, wxChoice* choice) const;
    std::string targetForChoice(wxChoice* choice) const;
    std::string targetForLock(wxCheckBox* checkBox) const;
    void sendChoiceValue(const char* target, wxChoice* choice);
    void sendAutoChangeMode();
    int collectFilterchainStages(
        std::vector<std::string>& stages, std::vector<int>& enabled) const;
    void sendFilterchainSequence();
    void sendFilterchainEnabled();

public:
    explicit CthughaPanelFrame(const std::string& endpoint);
    virtual ~CthughaPanelFrame();
};

#endif
