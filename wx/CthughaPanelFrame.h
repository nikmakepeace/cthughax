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

class CthughaPanelFrame : public CthughaPanelBase {
    std::unique_ptr<ControlPanelClient> client;
    wxTimer pollTimer;
    std::map<std::string, std::vector<std::string> > catalogNames;
    int receivedState;
    int everConnected;
    int updatingControls;

    void repairGeneratedLayout();
    void bindControlEvents();
    void setControlsEnabled(int enabled);
    void updateEnabledState();
    void onPollTimer(wxTimerEvent& event);
    void onChoiceChanged(wxCommandEvent& event);
    void onFlashlightChanged(wxCommandEvent& event);
    void onAutoChangeChanged(wxCommandEvent& event);
    void onFireThresholdChanged(wxCommandEvent& event);
    void onFireSensitivityChanged(wxCommandEvent& event);
    void onMaxFpsSpin(wxSpinEvent& event);
    void onMaxFpsText(wxCommandEvent& event);

    void handleClientEvent(const ControlPanelClientEvent& event);
    void handleProtocolMessage(const ControlJsonValue& message);
    void applyCatalogs(const ControlJsonValue& message);
    void applyState(const ControlJsonValue& message);

    void updateCatalogForTarget(const char* target, wxChoice* choice,
        const ControlJsonValue& targets);
    void selectChoiceValue(const char* target, wxChoice* choice,
        const std::string& value);
    void updateFireLevel(int cumulativeFireLevel, int threshold);
    std::string currentChoiceValue(const char* target, wxChoice* choice) const;
    std::string targetForChoice(wxChoice* choice) const;
    void sendChoiceValue(const char* target, wxChoice* choice);
    void sendMaxFps();

public:
    explicit CthughaPanelFrame(const std::string& endpoint);
    virtual ~CthughaPanelFrame();
};

#endif
