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
#include <wx/gauge.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class CthughaPanelBase
///////////////////////////////////////////////////////////////////////////////
class CthughaPanelBase : public wxFrame
{
	private:

	protected:
		wxScrolledWindow* m_scrolledWindow1;
		wxChoice* m_flame_choice;
		wxChoice* m_translation_choice;
		wxChoice* m_image_choice;
		wxChoice* m_object_choice;
		wxChoice* m_waveTable_choice;
		wxChoice* m_waveScale_choice;
		wxChoice* m_screen_choice;
		wxChoice* m_soundProcessing_choice;
		wxChoice* m_palette_choice;
		wxCheckBox* m_flashlight_checkBox;
		wxCheckBox* m_autoChange_checkBox;
		wxGauge* m_fireLevel_gauge;
		wxChoice* m_fireSource_choice;
		wxSlider* m_fireThreshold_slider;
		wxSlider* m_fireSensitivity_slider;
		wxSpinCtrl* m_maxFps_spinCtrl;

	public:

		CthughaPanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 460,800 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~CthughaPanelBase();

};
