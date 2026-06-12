///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/slider.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/gauge.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/listbox.h>
#include <wx/notebook.h>
#include <wx/statusbr.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class CthughaPanelBase
///////////////////////////////////////////////////////////////////////////////
class CthughaPanelBase : public wxFrame
{
	private:

	protected:
		wxNotebook* m_notebook5;
		wxPanel* m_sceneControls_panel;
		wxScrolledWindow* m_scrolledWindow1;
		wxStaticText* m_staticText171;
		wxStaticText* m_wave_staticText;
		wxChoice* m_wave_choice;
		wxCheckBox* m_lockWave_checkBox;
		wxChoice* m_flame_choice;
		wxCheckBox* m_lockFlame_checkBox;
		wxChoice* m_translation_choice;
		wxCheckBox* m_lockTranslation_checkBox;
		wxChoice* m_image_choice;
		wxCheckBox* m_lockImage_checkBox;
		wxChoice* m_object_choice;
		wxCheckBox* m_lockObject_checkBox;
		wxChoice* m_waveTable_choice;
		wxCheckBox* m_lockWaveTable_checkBox;
		wxChoice* m_waveScale_choice;
		wxCheckBox* m_lockWaveScale_checkBox;
		wxChoice* m_screen_choice;
		wxCheckBox* m_lockScreen_checkBox;
		wxChoice* m_soundProcessing_choice;
		wxCheckBox* m_lockSoundProcessing_checkBox;
		wxChoice* m_palette_choice;
		wxCheckBox* m_lockPalette_checkBox;
		wxCheckBox* m_flashlight_checkBox;
		wxCheckBox* m_lockFlashlight_checkBox;
		wxStaticLine* m_staticline1;
		wxStaticLine* m_staticline11;
		wxStaticLine* m_staticline12;
		wxStaticText* m_paletteSmoothing_staticText;
		wxSlider* m_paletteSmoothing_slider;
		wxStaticText* m_paletteSmoothing_text;
		wxRadioButton* m_autochangeAll_radioBtn;
		wxRadioButton* m_autochangeLittle_radioBtn;
		wxRadioButton* m_autochangeNone_radioBtn;
		wxGauge* m_fireLevel_gauge;
		wxChoice* m_fireSource_choice;
		wxSlider* m_fireThreshold_slider;
		wxStaticText* m_fireThreshold_text;
		wxSlider* m_fireSensitivity_slider;
		wxStaticText* m_fireSensitivity_text;
		wxSlider* m_maxFps_slider;
		wxStaticText* m_maxFps_text;
		wxPanel* m_filterChain_panel;
		wxListBox* m_filterchain_listBox;
		wxStatusBar* m_connected_statusBar;

	public:

		CthughaPanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 460,800 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~CthughaPanelBase();

};

