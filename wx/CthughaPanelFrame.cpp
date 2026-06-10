/** @file
 * wx control panel frame that mirrors visualiser-owned runtime state.
 */

#include "CthughaPanelFrame.h"

#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/string.h>

namespace {

static std::string wxToUtf8(const wxString& value) {
    return value.ToStdString();
}

static wxString utf8ToWx(const std::string& value) {
    return wxString::FromUTF8(value.c_str());
}

static const ControlJsonValue* objectMember(
    const ControlJsonValue* value, const char* name) {
    return value != 0 ? value->member(name) : 0;
}

static std::string stringMember(
    const ControlJsonValue* value, const char* name) {
    const ControlJsonValue* member = objectMember(value, name);
    if (member == 0)
        return "";
    if (member->type() == ControlJsonValue::StringType)
        return member->asString();
    if (member->type() == ControlJsonValue::NumberType) {
        char text[64];
        snprintf(text, sizeof(text), "%.0f", member->asNumber());
        return text;
    }
    if (member->type() == ControlJsonValue::BoolType)
        return member->asBool() ? "on" : "off";
    return "";
}

static int boolLikeMember(const ControlJsonValue* value, const char* name) {
    const ControlJsonValue* member = objectMember(value, name);
    if (member == 0)
        return 0;
    if (member->type() == ControlJsonValue::BoolType)
        return member->asBool() ? 1 : 0;
    if (member->type() == ControlJsonValue::StringType) {
        std::string text = member->asString();
        return !(text.empty() || text == "off" || text == "false"
            || text == "0");
    }
    if (member->type() == ControlJsonValue::NumberType)
        return member->asNumber() != 0.0;
    return 0;
}

static int intMember(
    const ControlJsonValue* value, const char* name, int fallback) {
    const ControlJsonValue* member = objectMember(value, name);
    if (member == 0)
        return fallback;
    if (member->type() == ControlJsonValue::NumberType)
        return int(member->asNumber());
    return fallback;
}

static int clampInt(int value, int minimum, int maximum) {
    if (value < minimum)
        return minimum;
    if (value > maximum)
        return maximum;
    return value;
}

static const char* messageType(const ControlJsonValue& message) {
    const ControlJsonValue* type = message.member("type");
    if (type == 0 || type->type() != ControlJsonValue::StringType)
        return "";
    return type->asString().c_str();
}

}

CthughaPanelFrame::CthughaPanelFrame(const std::string& endpoint)
    : CthughaPanelBase(0, wxID_ANY, wxT("Cthugha Control"))
    , client(new ControlPanelClient(endpoint))
    , pollTimer(this)
    , catalogNames()
    , receivedState(0)
    , everConnected(0)
    , updatingControls(0) {
    CreateStatusBar();
    repairGeneratedLayout();
    setControlsEnabled(0);
    bindControlEvents();
    Bind(wxEVT_TIMER, &CthughaPanelFrame::onPollTimer, this,
        pollTimer.GetId());

    if (endpoint.empty()) {
        SetStatusText(wxT("No control endpoint"));
    } else {
        SetStatusText(wxT("Connecting"));
        client->start();
    }
    pollTimer.Start(33);
}

CthughaPanelFrame::~CthughaPanelFrame() {
    pollTimer.Stop();
    if (client.get() != 0)
        client->stop();
}

void CthughaPanelFrame::repairGeneratedLayout() {
    m_scrolledWindow1->FitInside();
    Layout();
}

void CthughaPanelFrame::bindControlEvents() {
    m_flame_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_translation_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_image_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_object_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_waveTable_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_waveScale_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_screen_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_soundProcessing_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_fireSource_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_palette_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
    m_flashlight_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onFlashlightChanged, this);
    m_autoChange_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onAutoChangeChanged, this);
    m_fireThreshold_slider->Bind(wxEVT_SLIDER,
        &CthughaPanelFrame::onFireThresholdChanged, this);
    m_fireSensitivity_slider->Bind(wxEVT_SLIDER,
        &CthughaPanelFrame::onFireSensitivityChanged, this);
    m_maxFps_spinCtrl->Bind(wxEVT_SPINCTRL,
        &CthughaPanelFrame::onMaxFpsSpin, this);
    m_maxFps_spinCtrl->Bind(wxEVT_TEXT,
        &CthughaPanelFrame::onMaxFpsText, this);
}

void CthughaPanelFrame::setControlsEnabled(int enabled) {
    m_flame_choice->Enable(enabled != 0);
    m_translation_choice->Enable(enabled != 0);
    m_image_choice->Enable(enabled != 0);
    m_object_choice->Enable(enabled != 0);
    m_waveTable_choice->Enable(enabled != 0);
    m_waveScale_choice->Enable(enabled != 0);
    m_screen_choice->Enable(enabled != 0);
    m_soundProcessing_choice->Enable(enabled != 0);
    m_fireSource_choice->Enable(enabled != 0);
    m_palette_choice->Enable(enabled != 0);
    m_flashlight_checkBox->Enable(enabled != 0);
    m_autoChange_checkBox->Enable(enabled != 0);
    m_fireLevel_gauge->Enable(enabled != 0);
    m_fireThreshold_slider->Enable(enabled != 0);
    m_fireSensitivity_slider->Enable(enabled != 0);
    m_maxFps_spinCtrl->Enable(enabled != 0);
}

void CthughaPanelFrame::updateEnabledState() {
    setControlsEnabled(client->connected() && receivedState);
}

void CthughaPanelFrame::onPollTimer(wxTimerEvent&) {
    std::vector<ControlPanelClientEvent> events = client->pollEvents();
    for (std::vector<ControlPanelClientEvent>::const_iterator it
             = events.begin();
         it != events.end(); ++it)
        handleClientEvent(*it);
}

void CthughaPanelFrame::onChoiceChanged(wxCommandEvent& event) {
    if (updatingControls)
        return;
    wxChoice* choice = dynamic_cast<wxChoice*>(event.GetEventObject());
    std::string target = targetForChoice(choice);
    if (!target.empty())
        sendChoiceValue(target.c_str(), choice);
}

void CthughaPanelFrame::onFlashlightChanged(wxCommandEvent&) {
    if (updatingControls)
        return;
    client->sendSetBool("scene.flashlight",
        m_flashlight_checkBox->IsChecked());
}

void CthughaPanelFrame::onAutoChangeChanged(wxCommandEvent&) {
    if (updatingControls)
        return;
    client->sendSetBool("autoChange.enabled",
        m_autoChange_checkBox->IsChecked());
}

void CthughaPanelFrame::onFireThresholdChanged(wxCommandEvent&) {
    if (updatingControls)
        return;
    client->sendSetNumber("autoChange.cumulativeFireLevel",
        m_fireThreshold_slider->GetValue());
}

void CthughaPanelFrame::onFireSensitivityChanged(wxCommandEvent&) {
    if (updatingControls)
        return;
    client->sendSetNumber("audio.fireSensitivity",
        m_fireSensitivity_slider->GetValue());
}

void CthughaPanelFrame::onMaxFpsSpin(wxSpinEvent&) {
    sendMaxFps();
}

void CthughaPanelFrame::onMaxFpsText(wxCommandEvent&) {
    sendMaxFps();
}

void CthughaPanelFrame::handleClientEvent(
    const ControlPanelClientEvent& event) {
    switch (event.type) {
    case ControlPanelClientEvent::Connected:
        everConnected = 1;
        SetStatusText(wxT("Connected"));
        updateEnabledState();
        break;
    case ControlPanelClientEvent::Disconnected:
        if (everConnected) {
            Close();
            return;
        }
        SetStatusText(wxT("Disconnected"));
        receivedState = 0;
        updateEnabledState();
        break;
    case ControlPanelClientEvent::Error:
        SetStatusText(utf8ToWx(event.text));
        updateEnabledState();
        break;
    case ControlPanelClientEvent::Message:
        handleProtocolMessage(event.message);
        break;
    }
}

void CthughaPanelFrame::handleProtocolMessage(
    const ControlJsonValue& message) {
    std::string type = messageType(message);
    if (type == "catalogs") {
        applyCatalogs(message);
    } else if (type == "state") {
        applyState(message);
    } else if (type == "ack") {
        SetStatusText(wxT("Connected"));
    } else if (type == "error") {
        SetStatusText(utf8ToWx(stringMember(&message, "message")));
    }
}

void CthughaPanelFrame::applyCatalogs(const ControlJsonValue& message) {
    const ControlJsonValue* targets = message.member("targets");
    if (targets == 0 || targets->type() != ControlJsonValue::ObjectType)
        return;

    updatingControls = 1;
    updateCatalogForTarget("scene.flame", m_flame_choice, *targets);
    updateCatalogForTarget(
        "scene.translation", m_translation_choice, *targets);
    updateCatalogForTarget("scene.image", m_image_choice, *targets);
    updateCatalogForTarget("scene.object", m_object_choice, *targets);
    updateCatalogForTarget("scene.table", m_waveTable_choice, *targets);
    updateCatalogForTarget("scene.waveScale", m_waveScale_choice, *targets);
    updateCatalogForTarget("display.screen", m_screen_choice, *targets);
    updateCatalogForTarget("audio.processing", m_soundProcessing_choice,
        *targets);
    updateCatalogForTarget("audio.fireSource", m_fireSource_choice, *targets);
    updateCatalogForTarget("scene.palette", m_palette_choice, *targets);
    updatingControls = 0;
}

void CthughaPanelFrame::applyState(const ControlJsonValue& message) {
    const ControlJsonValue* scene = message.member("scene");
    const ControlJsonValue* display = message.member("display");
    const ControlJsonValue* audio = message.member("audio");
    const ControlJsonValue* autoChange = message.member("autoChange");

    updatingControls = 1;
    selectChoiceValue("scene.flame", m_flame_choice,
        stringMember(scene, "flame"));
    selectChoiceValue("scene.translation", m_translation_choice,
        stringMember(scene, "translation"));
    selectChoiceValue("scene.image", m_image_choice,
        stringMember(scene, "image"));
    selectChoiceValue("scene.object", m_object_choice,
        stringMember(scene, "object"));
    selectChoiceValue("scene.table", m_waveTable_choice,
        stringMember(scene, "table"));
    selectChoiceValue("scene.waveScale", m_waveScale_choice,
        stringMember(scene, "waveScale"));
    selectChoiceValue("display.screen", m_screen_choice,
        stringMember(display, "screen"));
    selectChoiceValue("audio.processing", m_soundProcessing_choice,
        stringMember(audio, "processing"));
    selectChoiceValue("audio.fireSource", m_fireSource_choice,
        stringMember(audio, "fireSource"));
    selectChoiceValue("scene.palette", m_palette_choice,
        stringMember(scene, "palette"));
    m_flashlight_checkBox->SetValue(
        boolLikeMember(scene, "flashlight") != 0);
    m_autoChange_checkBox->SetValue(
        boolLikeMember(autoChange, "enabled") != 0);
    int fireThreshold = intMember(autoChange, "cumulativeFireLevel",
        m_fireThreshold_slider->GetValue());
    int cumulativeFireLevel = intMember(audio, "cumulativeFireLevel", 0);
    updateFireLevel(cumulativeFireLevel, fireThreshold);
    int thresholdMaximum = fireThreshold > 5000 ? fireThreshold : 5000;
    m_fireThreshold_slider->SetMax(thresholdMaximum);
    m_fireThreshold_slider->SetValue(
        clampInt(fireThreshold, 0, thresholdMaximum));
    m_fireSensitivity_slider->SetValue(clampInt(
        intMember(audio, "fireSensitivity",
            m_fireSensitivity_slider->GetValue()),
        0, 100));
    m_maxFps_spinCtrl->SetValue(
        intMember(display, "maxFps", m_maxFps_spinCtrl->GetValue()));
    updatingControls = 0;

    receivedState = 1;
    SetStatusText(wxT("Connected"));
    updateEnabledState();
}

void CthughaPanelFrame::updateCatalogForTarget(const char* target,
    wxChoice* choice, const ControlJsonValue& targets) {
    const ControlJsonValue* entries = targets.member(target);
    if (entries == 0 || entries->type() != ControlJsonValue::ArrayType)
        return;

    std::vector<std::string>& names = catalogNames[target];
    names.clear();
    choice->Clear();

    const std::vector<ControlJsonValue>& array = entries->asArray();
    for (std::vector<ControlJsonValue>::const_iterator it = array.begin();
         it != array.end(); ++it) {
        std::string name = stringMember(&*it, "name");
        std::string label = stringMember(&*it, "label");
        if (name.empty())
            name = label;
        if (label.empty())
            label = name;
        names.push_back(name);
        choice->Append(utf8ToWx(label));
    }
}

void CthughaPanelFrame::selectChoiceValue(
    const char* target, wxChoice* choice, const std::string& value) {
    if (value.empty())
        return;

    std::vector<std::string>& names = catalogNames[target];
    int selection = -1;
    for (size_t i = 0; i < names.size(); i++) {
        if (names[i] == value) {
            selection = int(i);
            break;
        }
    }

    if (selection < 0) {
        names.push_back(value);
        choice->Append(utf8ToWx(value));
        selection = int(names.size() - 1);
    }
    choice->SetSelection(selection);
}

void CthughaPanelFrame::updateFireLevel(
    int cumulativeFireLevel, int threshold) {
    int range = threshold > 0 ? threshold : 1;
    m_fireLevel_gauge->SetRange(range);
    m_fireLevel_gauge->SetValue(clampInt(cumulativeFireLevel, 0, range));
}

std::string CthughaPanelFrame::currentChoiceValue(
    const char* target, wxChoice* choice) const {
    int selection = choice->GetSelection();
    if (selection < 0)
        return "";

    std::map<std::string, std::vector<std::string> >::const_iterator it
        = catalogNames.find(target);
    if (it != catalogNames.end()
        && selection < int(it->second.size()))
        return it->second[size_t(selection)];

    return wxToUtf8(choice->GetString(selection));
}

std::string CthughaPanelFrame::targetForChoice(wxChoice* choice) const {
    if (choice == m_flame_choice)
        return "scene.flame";
    if (choice == m_translation_choice)
        return "scene.translation";
    if (choice == m_image_choice)
        return "scene.image";
    if (choice == m_object_choice)
        return "scene.object";
    if (choice == m_waveTable_choice)
        return "scene.table";
    if (choice == m_waveScale_choice)
        return "scene.waveScale";
    if (choice == m_screen_choice)
        return "display.screen";
    if (choice == m_soundProcessing_choice)
        return "audio.processing";
    if (choice == m_fireSource_choice)
        return "audio.fireSource";
    if (choice == m_palette_choice)
        return "scene.palette";
    return "";
}

void CthughaPanelFrame::sendChoiceValue(
    const char* target, wxChoice* choice) {
    std::string value = currentChoiceValue(target, choice);
    if (!value.empty())
        client->sendSet(target, value);
}

void CthughaPanelFrame::sendMaxFps() {
    if (updatingControls)
        return;
    client->sendSetNumber("display.maxFps", m_maxFps_spinCtrl->GetValue());
}
