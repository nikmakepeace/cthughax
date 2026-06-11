/** @file
 * wx control panel frame that mirrors visualiser-owned runtime state.
 */

#include "CthughaPanelFrame.h"

#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
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

static const int labelFlashTicks = 16;

static wxColour labelFlashColour() {
    return wxColour(255, 239, 128);
}

static int blendedChannel(int base, int flash, int remaining, int total) {
    if (total <= 0)
        return base;
    return base + ((flash - base) * remaining) / total;
}

static wxColour blendedColour(
    const wxColour& base, const wxColour& flash, int remaining, int total) {
    return wxColour(
        blendedChannel(base.Red(), flash.Red(), remaining, total),
        blendedChannel(base.Green(), flash.Green(), remaining, total),
        blendedChannel(base.Blue(), flash.Blue(), remaining, total));
}

}

CthughaPanelFrame::LabelFlashState::LabelFlashState()
    : surface(0)
    , label(0)
    , baseBackground()
    , remainingTicks(0) { }

CthughaPanelFrame::LabelFlashState::LabelFlashState(
    wxWindow* surface_, wxStaticText* label_,
    const wxColour& baseBackground_)
    : surface(surface_)
    , label(label_)
    , baseBackground(baseBackground_)
    , remainingTicks(0) { }

CthughaPanelFrame::CthughaPanelFrame(const std::string& endpoint)
    : CthughaPanelBase(0, wxID_ANY, wxT("Cthugha Control"))
    , client(new ControlPanelClient(endpoint))
    , pollTimer(this)
    , catalogNames()
    , labelFlashStates()
    , receivedState(0)
    , everConnected(0)
    , updatingControls(0) {
    CreateStatusBar();
    repairGeneratedLayout();
    initializeLabelFlashes();
    setControlsEnabled(0);
    bindControlEvents();
    updateSliderTexts();
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

void CthughaPanelFrame::initializeLabelFlashes() {
    registerFlashLabel("scene.wave", "Wave");
    registerFlashLabel("scene.flame", "Flame");
    registerFlashLabel("scene.translation", "Translation");
    registerFlashLabel("scene.image", "Image");
    registerFlashLabel("scene.object", "Object");
    registerFlashLabel("scene.table", "Wave table");
    registerFlashLabel("scene.waveScale", "Wave scale");
    registerFlashLabel("display.screen", "Screen");
    registerFlashLabel("audio.processing", "Sound processing");
    registerFlashLabel("scene.palette", "Palette");
    registerFlashLabel("scene.flashlight", "Flashlight");
    registerFlashLabel("autoChange.mode", "Autochange");
    registerFlashLabel(
        "autoChange.cumulativeFireLevel", "Fire threshold");
    registerFlashLabel("audio.fireSource", "Fire source");
    registerFlashLabel("audio.fireSensitivity", "Fire sensitivity");
    registerFlashLabel("display.maxFps", "Max FPS");
}

void CthughaPanelFrame::registerFlashLabel(
    const char* key, const char* label) {
    wxStaticText* text = findStaticTextByLabel(utf8ToWx(label));
    if (text == 0)
        return;

    wxWindow* surface = wrapFlashLabel(text);
    if (surface == 0)
        surface = text;

    wxColour background = surface->GetBackgroundColour();
    if (!background.IsOk() && surface->GetParent() != 0)
        background = surface->GetParent()->GetBackgroundColour();
    if (!background.IsOk())
        background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);

    surface->SetBackgroundStyle(wxBG_STYLE_COLOUR);
    surface->SetBackgroundColour(background);
    text->SetBackgroundStyle(wxBG_STYLE_COLOUR);
    text->SetBackgroundColour(background);
    labelFlashStates[key] = LabelFlashState(surface, text, background);
}

wxStaticText* CthughaPanelFrame::findStaticTextByLabel(
    const wxString& label) const {
    wxWindowList& children = m_scrolledWindow1->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end();
         ++it) {
        wxStaticText* text = dynamic_cast<wxStaticText*>(*it);
        if (text != 0 && text->GetLabelText() == label)
            return text;
    }
    return 0;
}

wxWindow* CthughaPanelFrame::wrapFlashLabel(wxStaticText* label) {
    if (label == 0 || label->GetParent() != m_scrolledWindow1)
        return 0;

    wxSizer* parentSizer = label->GetContainingSizer();
    if (parentSizer == 0)
        parentSizer = m_scrolledWindow1->GetSizer();
    if (parentSizer == 0)
        return 0;

    wxSizerItem* item = parentSizer->GetItem(label, false);
    if (item == 0)
        return 0;

    size_t index = 0;
    int found = 0;
    wxSizerItemList& children = parentSizer->GetChildren();
    for (wxSizerItemList::compatibility_iterator node = children.GetFirst();
         node != nullptr; node = node->GetNext(), index++) {
        if (node->GetData() == item) {
            found = 1;
            break;
        }
    }
    if (!found)
        return 0;

    int proportion = item->GetProportion();
    int flag = item->GetFlag();
    int border = item->GetBorder();
    wxSize minSize = label->GetMinSize();
    if (!minSize.IsFullySpecified())
        minSize = label->GetBestSize();

    if (!parentSizer->Detach(label))
        return 0;

    wxPanel* panel = new wxPanel(m_scrolledWindow1, wxID_ANY);
    panel->SetBackgroundStyle(wxBG_STYLE_COLOUR);
    panel->SetMinSize(minSize);

    label->Reparent(panel);
    wxBoxSizer* labelSizer = new wxBoxSizer(wxHORIZONTAL);
    labelSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 3);
    panel->SetSizer(labelSizer);
    labelSizer->Fit(panel);

    parentSizer->Insert(index, panel, proportion, flag, border);
    m_scrolledWindow1->FitInside();
    Layout();
    return panel;
}

void CthughaPanelFrame::flashLabel(const char* key) {
    if (!receivedState)
        return;

    std::map<std::string, LabelFlashState>::iterator it
        = labelFlashStates.find(key);
    if (it == labelFlashStates.end())
        return;

    it->second.remainingTicks = labelFlashTicks;
    applyLabelFlash(it->second);
}

void CthughaPanelFrame::updateLabelFlashes() {
    for (std::map<std::string, LabelFlashState>::iterator it
             = labelFlashStates.begin();
         it != labelFlashStates.end(); ++it) {
        LabelFlashState& state = it->second;
        if (state.remainingTicks <= 0)
            continue;

        state.remainingTicks--;
        if (state.remainingTicks <= 0)
            restoreLabelFlash(state);
        else
            applyLabelFlash(state);
    }
}

void CthughaPanelFrame::applyLabelFlash(LabelFlashState& state) {
    if (state.surface == 0)
        return;
    wxColour colour = blendedColour(
        state.baseBackground, labelFlashColour(), state.remainingTicks,
        labelFlashTicks);
    state.surface->SetBackgroundColour(colour);
    state.surface->Refresh();
    if (state.label != 0) {
        state.label->SetBackgroundColour(colour);
        state.label->Refresh();
    }
}

void CthughaPanelFrame::restoreLabelFlash(LabelFlashState& state) {
    if (state.surface == 0)
        return;
    state.surface->SetBackgroundColour(state.baseBackground);
    state.surface->Refresh();
    if (state.label != 0) {
        state.label->SetBackgroundColour(state.baseBackground);
        state.label->Refresh();
    }
}

void CthughaPanelFrame::flashIfChanged(
    const char* key, const std::string& before, const std::string& after) {
    if (before != after)
        flashLabel(key);
}

void CthughaPanelFrame::flashIfChanged(
    const char* key, int before, int after) {
    if (before != after)
        flashLabel(key);
}

void CthughaPanelFrame::bindControlEvents() {
    m_wave_choice->Bind(wxEVT_CHOICE,
        &CthughaPanelFrame::onChoiceChanged, this);
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
    m_lockWave_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockFlame_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockTranslation_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockImage_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockObject_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockWaveTable_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockWaveScale_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockScreen_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockSoundProcessing_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockPalette_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_lockFlashlight_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onLockChanged, this);
    m_flashlight_checkBox->Bind(wxEVT_CHECKBOX,
        &CthughaPanelFrame::onFlashlightChanged, this);
    m_autochangeAll_radioBtn->Bind(wxEVT_RADIOBUTTON,
        &CthughaPanelFrame::onAutoChangeModeChanged, this);
    m_autochangeLittle_radioBtn->Bind(wxEVT_RADIOBUTTON,
        &CthughaPanelFrame::onAutoChangeModeChanged, this);
    m_autochangeNone_radioBtn->Bind(wxEVT_RADIOBUTTON,
        &CthughaPanelFrame::onAutoChangeModeChanged, this);
    m_fireThreshold_slider->Bind(wxEVT_SLIDER,
        &CthughaPanelFrame::onFireThresholdChanged, this);
    m_fireSensitivity_slider->Bind(wxEVT_SLIDER,
        &CthughaPanelFrame::onFireSensitivityChanged, this);
    m_maxFps_slider->Bind(wxEVT_SLIDER,
        &CthughaPanelFrame::onMaxFpsChanged, this);
}

void CthughaPanelFrame::setControlsEnabled(int enabled) {
    m_wave_choice->Enable(enabled != 0);
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
    m_lockWave_checkBox->Enable(enabled != 0);
    m_lockFlame_checkBox->Enable(enabled != 0);
    m_lockTranslation_checkBox->Enable(enabled != 0);
    m_lockImage_checkBox->Enable(enabled != 0);
    m_lockObject_checkBox->Enable(enabled != 0);
    m_lockWaveTable_checkBox->Enable(enabled != 0);
    m_lockWaveScale_checkBox->Enable(enabled != 0);
    m_lockScreen_checkBox->Enable(enabled != 0);
    m_lockSoundProcessing_checkBox->Enable(enabled != 0);
    m_lockPalette_checkBox->Enable(enabled != 0);
    m_lockFlashlight_checkBox->Enable(enabled != 0);
    m_flashlight_checkBox->Enable(enabled != 0);
    m_autochangeAll_radioBtn->Enable(enabled != 0);
    m_autochangeLittle_radioBtn->Enable(enabled != 0);
    m_autochangeNone_radioBtn->Enable(enabled != 0);
    m_fireLevel_gauge->Enable(enabled != 0);
    m_fireThreshold_slider->Enable(enabled != 0);
    m_fireSensitivity_slider->Enable(enabled != 0);
    m_maxFps_slider->Enable(enabled != 0);
}

void CthughaPanelFrame::updateEnabledState() {
    setControlsEnabled(client->connected() && receivedState);
}

void CthughaPanelFrame::bringToForeground() {
    if (IsIconized())
        Iconize(false);
    if (!IsShown())
        Show(true);
    Raise();
    SetFocus();
    RequestUserAttention(wxUSER_ATTENTION_INFO);
}

void CthughaPanelFrame::onPollTimer(wxTimerEvent&) {
    updateLabelFlashes();
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

void CthughaPanelFrame::onLockChanged(wxCommandEvent& event) {
    if (updatingControls)
        return;
    wxCheckBox* checkBox = dynamic_cast<wxCheckBox*>(event.GetEventObject());
    std::string target = targetForLock(checkBox);
    if (!target.empty())
        client->sendSetBool(target.c_str(), checkBox->IsChecked());
}

void CthughaPanelFrame::onFlashlightChanged(wxCommandEvent&) {
    if (updatingControls)
        return;
    client->sendSetBool("scene.flashlight",
        m_flashlight_checkBox->IsChecked());
}

void CthughaPanelFrame::onAutoChangeModeChanged(wxCommandEvent&) {
    if (updatingControls)
        return;
    sendAutoChangeMode();
}

void CthughaPanelFrame::onFireThresholdChanged(wxCommandEvent&) {
    updateSliderText(m_fireThreshold_slider, m_fireThreshold_text);
    if (updatingControls)
        return;
    client->sendSetNumber("autoChange.cumulativeFireLevel",
        m_fireThreshold_slider->GetValue());
}

void CthughaPanelFrame::onFireSensitivityChanged(wxCommandEvent&) {
    updateSliderText(m_fireSensitivity_slider, m_fireSensitivity_text);
    if (updatingControls)
        return;
    client->sendSetNumber("audio.fireSensitivity",
        m_fireSensitivity_slider->GetValue());
}

void CthughaPanelFrame::onMaxFpsChanged(wxCommandEvent&) {
    updateSliderText(m_maxFps_slider, m_maxFps_text);
    if (updatingControls)
        return;
    client->sendSetNumber("display.maxFps", m_maxFps_slider->GetValue());
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
    } else if (type == "focus") {
        bringToForeground();
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
    updateCatalogForTarget("scene.wave", m_wave_choice, *targets);
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
    const ControlJsonValue* locks = message.member("locks");

    updatingControls = 1;
    applyChoiceState("scene.wave", m_wave_choice, stringMember(scene, "wave"));
    applyChoiceState(
        "scene.flame", m_flame_choice, stringMember(scene, "flame"));
    applyChoiceState("scene.translation", m_translation_choice,
        stringMember(scene, "translation"));
    applyChoiceState(
        "scene.image", m_image_choice, stringMember(scene, "image"));
    applyChoiceState(
        "scene.object", m_object_choice, stringMember(scene, "object"));
    applyChoiceState(
        "scene.table", m_waveTable_choice, stringMember(scene, "table"));
    applyChoiceState("scene.waveScale", m_waveScale_choice,
        stringMember(scene, "waveScale"));
    applyChoiceState(
        "display.screen", m_screen_choice, stringMember(display, "screen"));
    applyChoiceState("audio.processing", m_soundProcessing_choice,
        stringMember(audio, "processing"));
    applyChoiceState("audio.fireSource", m_fireSource_choice,
        stringMember(audio, "fireSource"));
    applyChoiceState(
        "scene.palette", m_palette_choice, stringMember(scene, "palette"));
    applyCheckBoxState(
        "scene.flashlight", m_flashlight_checkBox,
        boolLikeMember(scene, "flashlight"));
    applyLocks(locks);
    flashIfChanged("autoChange.mode", currentAutoChangeMode(),
        autoChangeModeOf(autoChange));
    applyAutoChangeMode(autoChange);
    int fireThreshold = intMember(autoChange, "cumulativeFireLevel",
        m_fireThreshold_slider->GetValue());
    int cumulativeFireLevel = intMember(audio, "cumulativeFireLevel", 0);
    updateFireLevel(cumulativeFireLevel, fireThreshold);
    int thresholdMaximum = fireThreshold > 5000 ? fireThreshold : 5000;
    m_fireThreshold_slider->SetMax(thresholdMaximum);
    applySliderState("autoChange.cumulativeFireLevel", m_fireThreshold_slider,
        clampInt(fireThreshold, 0, thresholdMaximum));
    int fireSensitivity = clampInt(
        intMember(audio, "fireSensitivity",
            m_fireSensitivity_slider->GetValue()),
        0, 100);
    applySliderState(
        "audio.fireSensitivity", m_fireSensitivity_slider, fireSensitivity);
    int maxFps = intMember(display, "maxFps", m_maxFps_slider->GetValue());
    int maxFpsMaximum = maxFps > 120 ? maxFps : 120;
    m_maxFps_slider->SetMax(maxFpsMaximum);
    applySliderState(
        "display.maxFps", m_maxFps_slider,
        clampInt(maxFps, 5, maxFpsMaximum));
    updateSliderTexts();
    updatingControls = 0;

    receivedState = 1;
    SetStatusText(wxT("Connected"));
    updateEnabledState();
}

void CthughaPanelFrame::applyChoiceState(
    const char* target, wxChoice* choice, const std::string& value) {
    if (!value.empty())
        flashIfChanged(target, currentChoiceValue(target, choice), value);
    selectChoiceValue(target, choice, value);
}

void CthughaPanelFrame::applyCheckBoxState(
    const char* key, wxCheckBox* checkBox, int checked) {
    int value = checked != 0 ? 1 : 0;
    flashIfChanged(key, checkBox->GetValue() ? 1 : 0, value);
    checkBox->SetValue(value != 0);
}

void CthughaPanelFrame::applySliderState(
    const char* key, wxSlider* slider, int value) {
    flashIfChanged(key, slider->GetValue(), value);
    slider->SetValue(value);
}

void CthughaPanelFrame::applyLocks(const ControlJsonValue* locks) {
    applyCheckBoxState(
        "scene.wave", m_lockWave_checkBox,
        boolLikeMember(locks, "scene.wave"));
    applyCheckBoxState(
        "scene.flame", m_lockFlame_checkBox,
        boolLikeMember(locks, "scene.flame"));
    applyCheckBoxState("scene.translation", m_lockTranslation_checkBox,
        boolLikeMember(locks, "scene.translation"));
    applyCheckBoxState(
        "scene.image", m_lockImage_checkBox,
        boolLikeMember(locks, "scene.image"));
    applyCheckBoxState(
        "scene.object", m_lockObject_checkBox,
        boolLikeMember(locks, "scene.object"));
    applyCheckBoxState(
        "scene.table", m_lockWaveTable_checkBox,
        boolLikeMember(locks, "scene.table"));
    applyCheckBoxState("scene.waveScale", m_lockWaveScale_checkBox,
        boolLikeMember(locks, "scene.waveScale"));
    applyCheckBoxState("display.screen", m_lockScreen_checkBox,
        boolLikeMember(locks, "display.screen"));
    applyCheckBoxState("audio.processing", m_lockSoundProcessing_checkBox,
        boolLikeMember(locks, "audio.processing"));
    applyCheckBoxState(
        "scene.palette", m_lockPalette_checkBox,
        boolLikeMember(locks, "scene.palette"));
    applyCheckBoxState("scene.flashlight", m_lockFlashlight_checkBox,
        boolLikeMember(locks, "scene.flashlight"));
}

void CthughaPanelFrame::applyAutoChangeMode(
    const ControlJsonValue* autoChange) {
    int enabled = boolLikeMember(autoChange, "enabled");
    int changeLittle = boolLikeMember(autoChange, "changeLittle");
    if (!enabled) {
        m_autochangeNone_radioBtn->SetValue(true);
    } else if (changeLittle) {
        m_autochangeLittle_radioBtn->SetValue(true);
    } else {
        m_autochangeAll_radioBtn->SetValue(true);
    }
}

std::string CthughaPanelFrame::currentAutoChangeMode() const {
    if (m_autochangeNone_radioBtn->GetValue())
        return "none";
    if (m_autochangeLittle_radioBtn->GetValue())
        return "little";
    return "all";
}

std::string CthughaPanelFrame::autoChangeModeOf(
    const ControlJsonValue* autoChange) const {
    int enabled = boolLikeMember(autoChange, "enabled");
    int changeLittle = boolLikeMember(autoChange, "changeLittle");
    if (!enabled)
        return "none";
    if (changeLittle)
        return "little";
    return "all";
}

void CthughaPanelFrame::updateSliderText(
    wxSlider* slider, wxStaticText* text) {
    if (slider == 0 || text == 0)
        return;
    text->SetLabel(wxString::Format(wxT("%d"), slider->GetValue()));
}

void CthughaPanelFrame::updateSliderTexts() {
    updateSliderText(m_fireThreshold_slider, m_fireThreshold_text);
    updateSliderText(m_fireSensitivity_slider, m_fireSensitivity_text);
    updateSliderText(m_maxFps_slider, m_maxFps_text);
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
    if (choice == m_wave_choice)
        return "scene.wave";
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

std::string CthughaPanelFrame::targetForLock(wxCheckBox* checkBox) const {
    if (checkBox == m_lockWave_checkBox)
        return "scene.wave.locked";
    if (checkBox == m_lockFlame_checkBox)
        return "scene.flame.locked";
    if (checkBox == m_lockTranslation_checkBox)
        return "scene.translation.locked";
    if (checkBox == m_lockImage_checkBox)
        return "scene.image.locked";
    if (checkBox == m_lockObject_checkBox)
        return "scene.object.locked";
    if (checkBox == m_lockWaveTable_checkBox)
        return "scene.table.locked";
    if (checkBox == m_lockWaveScale_checkBox)
        return "scene.waveScale.locked";
    if (checkBox == m_lockScreen_checkBox)
        return "display.screen.locked";
    if (checkBox == m_lockSoundProcessing_checkBox)
        return "audio.processing.locked";
    if (checkBox == m_lockPalette_checkBox)
        return "scene.palette.locked";
    if (checkBox == m_lockFlashlight_checkBox)
        return "scene.flashlight.locked";
    return "";
}

void CthughaPanelFrame::sendChoiceValue(
    const char* target, wxChoice* choice) {
    std::string value = currentChoiceValue(target, choice);
    if (!value.empty())
        client->sendSet(target, value);
}

void CthughaPanelFrame::sendAutoChangeMode() {
    if (m_autochangeNone_radioBtn->GetValue()) {
        client->sendSetBool("autoChange.enabled", false);
        return;
    }

    client->sendSetBool("autoChange.enabled", true);
    client->sendSetBool("autoChange.changeLittle",
        m_autochangeLittle_radioBtn->GetValue());
}
