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

#ifndef DESURA_UFPROGRESSPAGE_H
#define DESURA_UFPROGRESSPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseInstallPage.h"
#include "wx_controls/gcControls.h"
#include "wx_controls/gcSpinnerProgBar.h"
#include "mcfcore/ProgressInfo.h"


namespace UI
{
namespace Forms
{
namespace ItemFormPage
{

class UninstallProgressPage : public BaseInstallPage  
{
public:
	UninstallProgressPage(wxWindow* parent, wxString label);
	~UninstallProgressPage();

	virtual void init();

protected:
	gcStaticText* m_labInfo;
	gcSpinnerProgBar* m_pbProgress;
	gcButton* m_butClose;
	
	virtual void onComplete(uint32&);
	virtual void onError(gcException& e);
	virtual void onMcfProgress(MCFCore::Misc::ProgressInfo& info);

	void onButtonClicked( wxCommandEvent& event );
	void completeUninstall();

private:
	bool m_bRemoveAll;
	bool m_bRemoveAcc;
};

}
}
}

#endif //DESURA_UFPROGRESSPAGE_H
