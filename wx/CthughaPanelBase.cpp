///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "CthughaPanelBase.h"

///////////////////////////////////////////////////////////////////////////

CthughaPanelBase::CthughaPanelBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableRow( 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow1 = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledWindow1->SetScrollRate( 5, 5 );
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText171 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Lock"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText171->Wrap( -1 );
	fgSizer2->Add( m_staticText171, 0, wxALL, 5 );

	m_staticText32 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Wave"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	fgSizer2->Add( m_staticText32, 0, wxALL, 5 );

	wxArrayString m_wave_choiceChoices;
	m_wave_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_wave_choiceChoices, 0 );
	m_wave_choice->SetSelection( 0 );
	fgSizer2->Add( m_wave_choice, 0, wxALL, 5 );

	m_lockWave_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockWave_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText4;
	m_staticText4 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Flame"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer2->Add( m_staticText4, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_flame_choiceChoices;
	m_flame_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_flame_choiceChoices, 0 );
	m_flame_choice->SetSelection( 0 );
	fgSizer2->Add( m_flame_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockFlame_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockFlame_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText7;
	m_staticText7 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Translation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer2->Add( m_staticText7, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_translation_choiceChoices;
	m_translation_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_translation_choiceChoices, 0 );
	m_translation_choice->SetSelection( 0 );
	fgSizer2->Add( m_translation_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockTranslation_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockTranslation_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText8;
	m_staticText8 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Image"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer2->Add( m_staticText8, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_image_choiceChoices;
	m_image_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_image_choiceChoices, 0 );
	m_image_choice->SetSelection( 0 );
	fgSizer2->Add( m_image_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockImage_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockImage_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText9;
	m_staticText9 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Object"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer2->Add( m_staticText9, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_object_choiceChoices;
	m_object_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_object_choiceChoices, 0 );
	m_object_choice->SetSelection( 0 );
	fgSizer2->Add( m_object_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockObject_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockObject_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText10;
	m_staticText10 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Wave table"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer2->Add( m_staticText10, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_waveTable_choiceChoices;
	m_waveTable_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_waveTable_choiceChoices, 0 );
	m_waveTable_choice->SetSelection( 0 );
	fgSizer2->Add( m_waveTable_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockWaveTable_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockWaveTable_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText14;
	m_staticText14 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Wave scale"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizer2->Add( m_staticText14, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_waveScale_choiceChoices;
	m_waveScale_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_waveScale_choiceChoices, 0 );
	m_waveScale_choice->SetSelection( 0 );
	fgSizer2->Add( m_waveScale_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockWaveScale_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockWaveScale_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText16;
	m_staticText16 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Screen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer2->Add( m_staticText16, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_screen_choiceChoices;
	m_screen_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_screen_choiceChoices, 0 );
	m_screen_choice->SetSelection( 0 );
	fgSizer2->Add( m_screen_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockScreen_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockScreen_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText11;
	m_staticText11 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Sound processing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer2->Add( m_staticText11, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_soundProcessing_choiceChoices;
	m_soundProcessing_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_soundProcessing_choiceChoices, 0 );
	m_soundProcessing_choice->SetSelection( 0 );
	fgSizer2->Add( m_soundProcessing_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockSoundProcessing_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockSoundProcessing_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText12;
	m_staticText12 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Palette"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer2->Add( m_staticText12, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_palette_choiceChoices;
	m_palette_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_palette_choiceChoices, 0 );
	m_palette_choice->SetSelection( 0 );
	fgSizer2->Add( m_palette_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockPalette_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockPalette_checkBox, 0, wxALL, 5 );

	wxStaticText* m_staticText13;
	m_staticText13 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Flashlight"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgSizer2->Add( m_staticText13, 0, wxALL|wxEXPAND, 5 );

	m_flashlight_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_flashlight_checkBox, 0, wxALL|wxEXPAND, 5 );

	m_lockFlashlight_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockFlashlight_checkBox, 0, wxALL, 5 );

	m_staticline1 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer2->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	m_staticline11 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer2->Add( m_staticline11, 0, wxEXPAND | wxALL, 5 );

	m_staticline12 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer2->Add( m_staticline12, 0, wxEXPAND | wxALL, 5 );

	wxStaticText* m_staticText17;
	m_staticText17 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Autochange"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	fgSizer2->Add( m_staticText17, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_autochangeAll_radioBtn = new wxRadioButton( m_scrolledWindow1, wxID_ANY, _("All"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer2->Add( m_autochangeAll_radioBtn, 0, wxALL, 5 );

	m_autochangeLittle_radioBtn = new wxRadioButton( m_scrolledWindow1, wxID_ANY, _("Little"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_autochangeLittle_radioBtn, 0, wxALL, 5 );

	m_autochangeNone_radioBtn = new wxRadioButton( m_scrolledWindow1, wxID_ANY, _("Off"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_autochangeNone_radioBtn, 0, wxALL, 5 );


	fgSizer2->Add( bSizer2, 1, wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticText* m_staticText18;
	m_staticText18 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Fire level"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer2->Add( m_staticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_fireLevel_gauge = new wxGauge( m_scrolledWindow1, wxID_ANY, 1000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	m_fireLevel_gauge->SetValue( 0 );
	fgSizer2->Add( m_fireLevel_gauge, 0, wxALL|wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticText* m_staticText21;
	m_staticText21 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Fire source"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizer2->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_fireSource_choiceChoices;
	m_fireSource_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fireSource_choiceChoices, 0 );
	m_fireSource_choice->SetSelection( 0 );
	fgSizer2->Add( m_fireSource_choice, 0, wxALL|wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticText* m_staticText19;
	m_staticText19 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Fire threshold"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	fgSizer2->Add( m_staticText19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_fireThreshold_slider = new wxSlider( m_scrolledWindow1, wxID_ANY, 1000, 0, 5000, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	fgSizer2->Add( m_fireThreshold_slider, 0, wxALL|wxEXPAND, 5 );

	m_fireThreshold_text = new wxStaticText( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fireThreshold_text->Wrap( -1 );
	fgSizer2->Add( m_fireThreshold_text, 0, wxALL, 5 );

	wxStaticText* m_staticText20;
	m_staticText20 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Fire sensitivity"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	fgSizer2->Add( m_staticText20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_fireSensitivity_slider = new wxSlider( m_scrolledWindow1, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	fgSizer2->Add( m_fireSensitivity_slider, 0, wxALL|wxEXPAND, 5 );

	m_fireSensitivity_text = new wxStaticText( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fireSensitivity_text->Wrap( -1 );
	fgSizer2->Add( m_fireSensitivity_text, 0, wxALL, 5 );

	wxStaticText* m_staticText15;
	m_staticText15 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Max FPS"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer2->Add( m_staticText15, 0, wxALL|wxEXPAND, 5 );

	m_maxFps_slider = new wxSlider( m_scrolledWindow1, wxID_ANY, 50, 5, 120, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	fgSizer2->Add( m_maxFps_slider, 0, wxALL, 5 );

	m_maxFps_text = new wxStaticText( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maxFps_text->Wrap( -1 );
	fgSizer2->Add( m_maxFps_text, 0, wxALL, 5 );


	m_scrolledWindow1->SetSizer( fgSizer2 );
	m_scrolledWindow1->Layout();
	fgSizer2->Fit( m_scrolledWindow1 );
	bSizer3->Add( m_scrolledWindow1, 1, wxEXPAND | wxALL, 5 );


	fgSizer1->Add( bSizer3, 1, wxEXPAND, 5 );


	this->SetSizer( fgSizer1 );
	this->Layout();

	this->Centre( wxBOTH );
}

CthughaPanelBase::~CthughaPanelBase()
{
}
