/** @file
 * Hand-written runtime controller for the wxFormBuilder panel.
 */

#ifndef CTHUGHA_WX_CTHUGHA_PANEL_H
#define CTHUGHA_WX_CTHUGHA_PANEL_H

#include "CthughaPanelBase.h"
#include "ControlPanelService.h"
#include "RuntimeCommand.h"

class SceneOptionSelection;

class CthughaPanel : public CthughaPanelBase {
    ControlPanelRuntimePorts ports;
    int syncing;

    SceneOptionSelection* sceneSelection(RuntimeSceneTarget target) const;
    void syncSceneChoice(wxChoice* choice, SceneOptionSelection* selection);
    void syncSoundProcessing();
    void syncFlashlight();
    void syncMaxFps();

    void bindSceneChoice(wxChoice* choice, RuntimeSceneTarget target);
    void onSceneChoice(wxCommandEvent& event, RuntimeSceneTarget target);
    void onSoundProcessing(wxCommandEvent& event);
    void onFlashlight(wxCommandEvent& event);
    void onMaxFps(wxCommandEvent& event);
    void onMaxFpsFocusLost(wxFocusEvent& event);
    void onClose(wxCloseEvent& event);

public:
    explicit CthughaPanel(const ControlPanelRuntimePorts& ports_);

    /** Refreshes all controls from the borrowed runtime ports. */
    void syncFromRuntime();
};

#endif
