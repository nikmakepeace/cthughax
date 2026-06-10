/** @file
 * Runtime controller for the wx control panel.
 */

#include "CthughaPanel.h"

#include "AudioProcessing.h"
#include "DisplayPresentationOptions.h"
#include "RuntimeCommandSink.h"
#include "RuntimeCommandTargets.h"
#include "SceneChoiceSelection.h"
#include "SceneVisualSelections.h"

#include <cstring>
#include <string>

namespace {

static const char* kSoundProcessingChoices[] = {
    "none",
    "Filter1",
    "Filter2",
    "FFT",
};

static wxString wxText(const char* text) {
    return wxString::FromUTF8(text != 0 ? text : "");
}

static int sameText(const char* lhs, const char* rhs) {
    return (lhs != 0) && (rhs != 0) && std::strcmp(lhs, rhs) == 0;
}

}

CthughaPanel::CthughaPanel(const ControlPanelRuntimePorts& ports_)
    : CthughaPanelBase(0, wxID_ANY, _("Cthugha Controls"))
    , ports(ports_)
    , syncing(0) {
    m_maxFps_spinCtrl->SetRange(0, 300);
    m_maxFps_spinCtrl->SetValue(25);

    bindSceneChoice(m_flame_choice, RuntimeSceneFlame);
    bindSceneChoice(m_translation_choice, RuntimeSceneTranslation);
    bindSceneChoice(m_image_choice, RuntimeSceneImage);
    bindSceneChoice(m_object_choice, RuntimeSceneObject);
    bindSceneChoice(m_waveTable_choice, RuntimeSceneTable);
    bindSceneChoice(m_waveScale_choice, RuntimeSceneWaveScale);
    bindSceneChoice(m_palette_choice, RuntimeScenePalette);

    m_soundProcessing_choice->Bind(wxEVT_CHOICE,
        &CthughaPanel::onSoundProcessing, this);
    m_flashlight_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanel::onFlashlight, this);
    m_maxFps_spinCtrl->Bind(wxEVT_SPINCTRL,
        &CthughaPanel::onMaxFps, this);
    m_maxFps_spinCtrl->Bind(wxEVT_KILL_FOCUS,
        &CthughaPanel::onMaxFpsFocusLost, this);
    Bind(wxEVT_CLOSE_WINDOW, &CthughaPanel::onClose, this);

    syncFromRuntime();
}

SceneOptionSelection* CthughaPanel::sceneSelection(
    RuntimeSceneTarget target) const {
    if (ports.sceneVisualSelections == 0)
        return 0;

    switch (target) {
    case RuntimeSceneFlame:
        return &ports.sceneVisualSelections->flame();
    case RuntimeSceneGeneralFlame:
        return &ports.sceneVisualSelections->generalFlame();
    case RuntimeSceneWave:
        return &ports.sceneVisualSelections->wave();
    case RuntimeSceneWaveScale:
        return &ports.sceneVisualSelections->waveScale();
    case RuntimeSceneObject:
        return &ports.sceneVisualSelections->object();
    case RuntimeSceneTranslation:
        return &ports.sceneVisualSelections->translation();
    case RuntimeSceneBorder:
        return &ports.sceneVisualSelections->border();
    case RuntimeSceneFlashlight:
        return &ports.sceneVisualSelections->flashlight();
    case RuntimeScenePalette:
        return &ports.sceneVisualSelections->palette();
    case RuntimeSceneTable:
        return &ports.sceneVisualSelections->table();
    case RuntimeSceneImage:
        return &ports.sceneVisualSelections->images();
    }

    return 0;
}

void CthughaPanel::bindSceneChoice(
    wxChoice* choice, RuntimeSceneTarget target) {
    choice->Bind(wxEVT_CHOICE,
        [this, target](wxCommandEvent& event) {
            onSceneChoice(event, target);
        });
}

void CthughaPanel::syncSceneChoice(
    wxChoice* choice, SceneOptionSelection* selection) {
    int count = selection != 0 ? selection->entryCount() : 0;

    if (int(choice->GetCount()) != count) {
        choice->Clear();
        for (int i = 0; i < count; i++) {
            const SceneChoice* entry = selection->choiceAt(i);
            choice->Append(wxText(entry != 0 ? entry->name() : "unknown"));
        }
    }

    if (selection != 0
        && selection->currentValue() >= 0
        && selection->currentValue() < count) {
        choice->SetSelection(selection->currentValue());
    } else {
        choice->SetSelection(wxNOT_FOUND);
    }
    choice->Enable(count > 0);
}

void CthughaPanel::syncSoundProcessing() {
    int choiceCount = int(sizeof(kSoundProcessingChoices)
        / sizeof(kSoundProcessingChoices[0]));
    if (int(m_soundProcessing_choice->GetCount()) != choiceCount) {
        m_soundProcessing_choice->Clear();
        for (int i = 0; i < choiceCount; i++)
            m_soundProcessing_choice->Append(wxText(kSoundProcessingChoices[i]));
    }

    const char* current = ports.audioProcessingSelector != 0
        ? ports.audioProcessingSelector->text()
        : 0;
    int selected = wxNOT_FOUND;
    for (int i = 0; i < choiceCount; i++) {
        if (sameText(current, kSoundProcessingChoices[i])) {
            selected = i;
            break;
        }
    }

    m_soundProcessing_choice->SetSelection(selected);
    m_soundProcessing_choice->Enable(ports.audioProcessingSelector != 0);
}

void CthughaPanel::syncFlashlight() {
    SceneOptionSelection* selection = sceneSelection(RuntimeSceneFlashlight);
    if (selection == 0) {
        m_flashlight_checkBox->SetValue(false);
        m_flashlight_checkBox->Enable(false);
        return;
    }

    m_flashlight_checkBox->SetValue(selection->currentValue() != 0);
    m_flashlight_checkBox->Enable(true);
}

void CthughaPanel::syncMaxFps() {
    if (ports.displaySettings == 0) {
        m_maxFps_spinCtrl->Enable(false);
        return;
    }

    m_maxFps_spinCtrl->SetValue(
        int(ports.displaySettings->maxFramesPerSecond));
    m_maxFps_spinCtrl->Enable(true);
}

void CthughaPanel::syncFromRuntime() {
    syncing = 1;

    syncSceneChoice(m_flame_choice, sceneSelection(RuntimeSceneFlame));
    syncSceneChoice(m_translation_choice,
        sceneSelection(RuntimeSceneTranslation));
    syncSceneChoice(m_image_choice, sceneSelection(RuntimeSceneImage));
    syncSceneChoice(m_object_choice, sceneSelection(RuntimeSceneObject));
    syncSceneChoice(m_waveTable_choice, sceneSelection(RuntimeSceneTable));
    syncSceneChoice(m_waveScale_choice, sceneSelection(RuntimeSceneWaveScale));
    syncSceneChoice(m_palette_choice, sceneSelection(RuntimeScenePalette));
    syncSoundProcessing();
    syncFlashlight();
    syncMaxFps();

    syncing = 0;
}

void CthughaPanel::onSceneChoice(
    wxCommandEvent& event, RuntimeSceneTarget target) {
    if (syncing || ports.runtimeCommands == 0)
        return;

    int selection = event.GetSelection();
    if (selection == wxNOT_FOUND)
        return;

    ports.runtimeCommands->apply(
        RuntimeCommand::activateScene(target, selection));
}

void CthughaPanel::onSoundProcessing(wxCommandEvent& event) {
    if (syncing || ports.runtimeCommands == 0)
        return;

    int selection = event.GetSelection();
    if (selection == wxNOT_FOUND)
        return;

    std::string name(m_soundProcessing_choice->GetString(selection).mb_str());
    ports.runtimeCommands->apply(
        RuntimeCommand::changeSoundProcessingTo(name.c_str()));
}

void CthughaPanel::onFlashlight(wxCommandEvent&) {
    if (syncing || ports.runtimeCommands == 0)
        return;

    ports.runtimeCommands->apply(RuntimeCommand::changeSceneTo(
        RuntimeSceneFlashlight,
        m_flashlight_checkBox->GetValue() ? "on" : "off"));
}

void CthughaPanel::onMaxFps(wxCommandEvent&) {
    if (syncing || ports.runtimeCommandRouter == 0
        || ports.displaySettings == 0)
        return;

    std::string value = std::to_string(m_maxFps_spinCtrl->GetValue());
    ports.runtimeCommandRouter->changeOptionTo(
        ports.displaySettings->maxFramesPerSecond, value.c_str());
}

void CthughaPanel::onMaxFpsFocusLost(wxFocusEvent& event) {
    if (!syncing) {
        wxCommandEvent command;
        onMaxFps(command);
    }
    event.Skip();
}

void CthughaPanel::onClose(wxCloseEvent& event) {
    Hide();
    if (event.CanVeto())
        event.Veto();
}
