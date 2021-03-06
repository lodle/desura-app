/*
Desura is the leading indie game distribution platform
Copyright (C) 2011 Mark Chandler (Desura Net Pty Ltd)

$LicenseInfo:firstyear=2014&license=lgpl$
Copyright (C) 2014, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see <http://www.gnu.org/licenses/>
or write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
*/

#include "Common.h"
#include "gcUpdateForm.h"
#include "MainApp.h"
#include "managers/ConCommand.h"


extern ConCommand cc_restart_wait;
extern const char* GetAppVersion();


GCUpdateInfo::GCUpdateInfo(wxWindow* parent)
	: gcFrame(parent, wxID_ANY, wxT("#UF_TITLE"), wxDefaultPosition, wxSize( 445,300 ), wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL)
{
	Bind(wxEVT_CLOSE_WINDOW, &GCUpdateInfo::onFormClose, this);
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, &GCUpdateInfo::onButClick, this);

	SetTitle(Managers::GetString(L"#UF_TITLE"));

	m_labInfo = new wxStaticText( this, wxID_ANY, Managers::GetString(L"#UF_INFO"), wxDefaultPosition, wxDefaultSize, 0 );

	m_ieBrowser = new gcMiscWebControl( this, "about:blank", "DesuraUpdate");
	m_ieBrowser->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVECAPTION ) );
	m_ieBrowser->onPageLoadEvent += guiDelegate(this, &GCUpdateInfo::onPageLoad);

	m_butRestartNow = new gcButton( this, wxID_ANY, Managers::GetString(L"#UF_RESTART_NOW"), wxDefaultPosition, wxSize( 100,-1 ), 0 );
	m_butRestartLater = new gcButton( this, wxID_ANY, Managers::GetString(L"#UF_RESTART_LATER"), wxDefaultPosition, wxSize( 100,-1 ), 0 );


	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );
	bSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	bSizer1->Add( m_butRestartNow, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	bSizer1->Add( m_butRestartLater, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 3, 1, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableRow( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	fgSizer1->Add( m_labInfo, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	fgSizer1->Add( m_ieBrowser, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	fgSizer1->Add( bSizer1, 1, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	this->SetSizer( fgSizer1 );
	this->Layout();

	centerOnParent();
}

GCUpdateInfo::~GCUpdateInfo()
{
}

void GCUpdateInfo::setInfo(uint32 appver)
{
	if (appver == 0)
		appver = BUILDID_PUBLIC;

	m_ieBrowser->loadUrl(gcWString(L"{0}/{1}", GetWebCore()->getUrl(WebCore::AppChangeLog), appver));
}

void GCUpdateInfo::onFormClose(wxCloseEvent& event)
{
	g_pMainApp->closeForm(this->GetId());
}

void GCUpdateInfo::onButClick(wxCommandEvent& event)
{
	if (m_butRestartNow->GetId() == event.GetId())
	{
		Close();
		doRestart();
	}
	else if (m_butRestartLater->GetId()  == event.GetId())
	{
		Close();
	}
}

void GCUpdateInfo::onPageLoad()
{
	ERROR_OUTPUT("Showing update form");
	
	Show(true);

#ifdef WIN32
	FLASHWINFO info;

	memset(&info, 0, sizeof(FLASHWINFO));
	info.cbSize = sizeof(FLASHWINFO);
	info.hwnd = (HWND)GetHWND();
	info.dwFlags = FLASHW_TRAY|FLASHW_TIMERNOFG;
	info.uCount = 20;

	FlashWindowEx(&info);
#endif
}

void GCUpdateInfo::doRestart()
{
	cc_restart_wait();
}

bool GCUpdateInfo::Show(bool show)
{
	return gcFrame::Show(show);
}

void GCUpdateInfo::Raise()
{
	gcFrame::Raise();
}
