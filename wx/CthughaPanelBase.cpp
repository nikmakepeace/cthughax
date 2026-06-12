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
	fgSizer1 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableRow( 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_notebook5 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_sceneControls_panel = new wxPanel( m_notebook5, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_sceneControls_panel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow1 = new wxScrolledWindow( m_sceneControls_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
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

	m_wave_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Wave"), wxDefaultPosition, wxDefaultSize, 0 );
	m_wave_staticText->Wrap( -1 );
	fgSizer2->Add( m_wave_staticText, 0, wxALL, 5 );

	wxArrayString m_wave_choiceChoices;
	m_wave_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_wave_choiceChoices, 0 );
	m_wave_choice->SetSelection( 0 );
	fgSizer2->Add( m_wave_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockWave_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockWave_checkBox, 0, wxALL, 5 );

	wxStaticText* m_flame_staticText;
	m_flame_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Flame"), wxDefaultPosition, wxDefaultSize, 0 );
	m_flame_staticText->Wrap( -1 );
	fgSizer2->Add( m_flame_staticText, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_flame_choiceChoices;
	m_flame_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_flame_choiceChoices, 0 );
	m_flame_choice->SetSelection( 0 );
	fgSizer2->Add( m_flame_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockFlame_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockFlame_checkBox, 0, wxALL, 5 );

	wxStaticText* m_translation_staticText;
	m_translation_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Translation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_translation_staticText->Wrap( -1 );
	fgSizer2->Add( m_translation_staticText, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_translation_choiceChoices;
	m_translation_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_translation_choiceChoices, 0 );
	m_translation_choice->SetSelection( 0 );
	fgSizer2->Add( m_translation_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockTranslation_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockTranslation_checkBox, 0, wxALL, 5 );

	wxStaticText* m_image_staticText;
	m_image_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Image"), wxDefaultPosition, wxDefaultSize, 0 );
	m_image_staticText->Wrap( -1 );
	fgSizer2->Add( m_image_staticText, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_image_choiceChoices;
	m_image_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_image_choiceChoices, 0 );
	m_image_choice->SetSelection( 0 );
	fgSizer2->Add( m_image_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockImage_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockImage_checkBox, 0, wxALL, 5 );

	wxStaticText* m_object_staticText;
	m_object_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Object"), wxDefaultPosition, wxDefaultSize, 0 );
	m_object_staticText->Wrap( -1 );
	fgSizer2->Add( m_object_staticText, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_object_choiceChoices;
	m_object_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_object_choiceChoices, 0 );
	m_object_choice->SetSelection( 0 );
	fgSizer2->Add( m_object_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockObject_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockObject_checkBox, 0, wxALL, 5 );

	wxStaticText* m_waveTable_staticText;
	m_waveTable_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Wave table"), wxDefaultPosition, wxDefaultSize, 0 );
	m_waveTable_staticText->Wrap( -1 );
	fgSizer2->Add( m_waveTable_staticText, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_waveTable_choiceChoices;
	m_waveTable_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_waveTable_choiceChoices, 0 );
	m_waveTable_choice->SetSelection( 0 );
	fgSizer2->Add( m_waveTable_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockWaveTable_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockWaveTable_checkBox, 0, wxALL, 5 );

	wxStaticText* m_waveScale_staticText;
	m_waveScale_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Wave scale"), wxDefaultPosition, wxDefaultSize, 0 );
	m_waveScale_staticText->Wrap( -1 );
	fgSizer2->Add( m_waveScale_staticText, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_waveScale_choiceChoices;
	m_waveScale_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_waveScale_choiceChoices, 0 );
	m_waveScale_choice->SetSelection( 0 );
	fgSizer2->Add( m_waveScale_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockWaveScale_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockWaveScale_checkBox, 0, wxALL, 5 );

	wxStaticText* m_screen_staticText;
	m_screen_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Screen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_screen_staticText->Wrap( -1 );
	fgSizer2->Add( m_screen_staticText, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_screen_choiceChoices;
	m_screen_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_screen_choiceChoices, 0 );
	m_screen_choice->SetSelection( 0 );
	fgSizer2->Add( m_screen_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockScreen_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockScreen_checkBox, 0, wxALL, 5 );

	wxStaticText* m_soundProcessing_staticText;
	m_soundProcessing_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Sound processing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_soundProcessing_staticText->Wrap( -1 );
	fgSizer2->Add( m_soundProcessing_staticText, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_soundProcessing_choiceChoices;
	m_soundProcessing_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_soundProcessing_choiceChoices, 0 );
	m_soundProcessing_choice->SetSelection( 0 );
	fgSizer2->Add( m_soundProcessing_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockSoundProcessing_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockSoundProcessing_checkBox, 0, wxALL, 5 );

	wxStaticText* m_palette_staticText;
	m_palette_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Palette"), wxDefaultPosition, wxDefaultSize, 0 );
	m_palette_staticText->Wrap( -1 );
	fgSizer2->Add( m_palette_staticText, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_palette_choiceChoices;
	m_palette_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_palette_choiceChoices, 0 );
	m_palette_choice->SetSelection( 0 );
	fgSizer2->Add( m_palette_choice, 0, wxALL|wxEXPAND, 5 );

	m_lockPalette_checkBox = new wxCheckBox( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_lockPalette_checkBox, 0, wxALL, 5 );

	wxStaticText* m_flashlight_staticText;
	m_flashlight_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Flashlight"), wxDefaultPosition, wxDefaultSize, 0 );
	m_flashlight_staticText->Wrap( -1 );
	fgSizer2->Add( m_flashlight_staticText, 0, wxALL|wxEXPAND, 5 );

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

	m_paletteSmoothing_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Palette smoothing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_paletteSmoothing_staticText->Wrap( -1 );
	fgSizer2->Add( m_paletteSmoothing_staticText, 0, wxALL, 5 );

	m_paletteSmoothing_slider = new wxSlider( m_scrolledWindow1, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	fgSizer2->Add( m_paletteSmoothing_slider, 0, wxALL|wxEXPAND, 5 );

	m_paletteSmoothing_text = new wxStaticText( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_paletteSmoothing_text->Wrap( -1 );
	fgSizer2->Add( m_paletteSmoothing_text, 0, wxALL, 5 );

	wxStaticText* m_autochange_staticText;
	m_autochange_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Autochange"), wxDefaultPosition, wxDefaultSize, 0 );
	m_autochange_staticText->Wrap( -1 );
	fgSizer2->Add( m_autochange_staticText, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_autochangeAll_radioBtn = new wxRadioButton( m_scrolledWindow1, wxID_ANY, _("Many"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer2->Add( m_autochangeAll_radioBtn, 0, wxALL, 5 );

	m_autochangeLittle_radioBtn = new wxRadioButton( m_scrolledWindow1, wxID_ANY, _("One"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_autochangeLittle_radioBtn, 0, wxALL, 5 );

	m_autochangeNone_radioBtn = new wxRadioButton( m_scrolledWindow1, wxID_ANY, _("None"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_autochangeNone_radioBtn, 0, wxALL, 5 );


	fgSizer2->Add( bSizer2, 1, wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticText* m_fireLevel_staticText;
	m_fireLevel_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Fire level"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fireLevel_staticText->Wrap( -1 );
	fgSizer2->Add( m_fireLevel_staticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_fireLevel_gauge = new wxGauge( m_scrolledWindow1, wxID_ANY, 1000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	m_fireLevel_gauge->SetValue( 0 );
	fgSizer2->Add( m_fireLevel_gauge, 0, wxALL|wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticText* m_fireSource_staticText;
	m_fireSource_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Fire source"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fireSource_staticText->Wrap( -1 );
	fgSizer2->Add( m_fireSource_staticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_fireSource_choiceChoices;
	m_fireSource_choice = new wxChoice( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fireSource_choiceChoices, 0 );
	m_fireSource_choice->SetSelection( 0 );
	fgSizer2->Add( m_fireSource_choice, 0, wxALL|wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticText* m_fireThreshold_staticText;
	m_fireThreshold_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Fire threshold"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fireThreshold_staticText->Wrap( -1 );
	fgSizer2->Add( m_fireThreshold_staticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_fireThreshold_slider = new wxSlider( m_scrolledWindow1, wxID_ANY, 1000, 0, 5000, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	fgSizer2->Add( m_fireThreshold_slider, 0, wxALL|wxEXPAND, 5 );

	m_fireThreshold_text = new wxStaticText( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fireThreshold_text->Wrap( -1 );
	fgSizer2->Add( m_fireThreshold_text, 0, wxALL, 5 );

	wxStaticText* m_fireSensitivity_staticText;
	m_fireSensitivity_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Fire sensitivity"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fireSensitivity_staticText->Wrap( -1 );
	fgSizer2->Add( m_fireSensitivity_staticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_fireSensitivity_slider = new wxSlider( m_scrolledWindow1, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	fgSizer2->Add( m_fireSensitivity_slider, 0, wxALL|wxEXPAND, 5 );

	m_fireSensitivity_text = new wxStaticText( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fireSensitivity_text->Wrap( -1 );
	fgSizer2->Add( m_fireSensitivity_text, 0, wxALL, 5 );

	wxStaticText* m_maxFps_staticText;
	m_maxFps_staticText = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Max FPS"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxFps_staticText->Wrap( -1 );
	fgSizer2->Add( m_maxFps_staticText, 0, wxALL|wxEXPAND, 5 );

	m_maxFps_slider = new wxSlider( m_scrolledWindow1, wxID_ANY, 50, 5, 120, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	fgSizer2->Add( m_maxFps_slider, 0, wxALL|wxEXPAND, 5 );

	m_maxFps_text = new wxStaticText( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maxFps_text->Wrap( -1 );
	fgSizer2->Add( m_maxFps_text, 0, wxALL, 5 );


	m_scrolledWindow1->SetSizer( fgSizer2 );
	m_scrolledWindow1->Layout();
	fgSizer2->Fit( m_scrolledWindow1 );
	bSizer3->Add( m_scrolledWindow1, 1, wxEXPAND | wxALL, 5 );


	m_sceneControls_panel->SetSizer( bSizer3 );
	m_sceneControls_panel->Layout();
	bSizer3->Fit( m_sceneControls_panel );
	m_notebook5->AddPage( m_sceneControls_panel, _("Scene Controls"), true );
	m_filterChain_panel = new wxPanel( m_notebook5, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_filterchain_listBox = new wxListBox( m_filterChain_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_MULTIPLE|wxLB_NEEDED_SB );
	bSizer5->Add( m_filterchain_listBox, 0, wxALL|wxEXPAND, 5 );


	m_filterChain_panel->SetSizer( bSizer5 );
	m_filterChain_panel->Layout();
	bSizer5->Fit( m_filterChain_panel );
	m_notebook5->AddPage( m_filterChain_panel, _("Filter Chain"), false );

	fgSizer1->Add( m_notebook5, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( fgSizer1 );
	this->Layout();
	m_connected_statusBar = this->CreateStatusBar( 1, wxSTB_DEFAULT_STYLE|wxSTB_SIZEGRIP, wxID_ANY );

	this->Centre( wxBOTH );
}

CthughaPanelBase::~CthughaPanelBase()
{
}
