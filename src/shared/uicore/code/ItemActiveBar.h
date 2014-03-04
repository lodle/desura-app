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

#ifndef DESURA_ITEMACTIVEBAR_H
#define DESURA_ITEMACTIVEBAR_H
#ifdef _WIN32
#pragma once
#endif

#include "wx_controls/gcPanel.h"
#include "gcJSEventCache.h"

namespace UserCore
{
	namespace Item
	{
		class ItemHandleI;
		namespace Helper
		{
			class ItemHandleHelperI;
		}	
	}
}

class gcWebControlI;

class ItemActiveHelperParent
{
public:
	virtual void postEvent(DesuraId id, const char* szEventName, uint32 nArg)=0;
	virtual void postEvent(DesuraId id, const char* szEventName, const char* szArg)=0;
};

class ItemActiveBar : public gcPanel, protected ItemActiveHelperParent
{
public:
	ItemActiveBar(wxWindow* parent);
	~ItemActiveBar();

protected:
	void postEvent(const char* name, const char* arg1, const char* arg2);
	void doneLoading();

	void onNewItem(DesuraId &id);

	void setupHandle(UserCore::Item::ItemHandleI* pItemHandle);


	void postEvent(DesuraId id, const char* szEventName, uint32 nArg) override;
	void postEvent(DesuraId id, const char* szEventName, const char* szArg) override;

private:
	gcWebControlI* m_pWebControl;

	std::map<uint64, UserCore::Item::Helper::ItemHandleHelperI*> m_pHelperMap;

	JSEventMap m_JSEventMap;
	bool m_bGlobalItemUpdate;

	std::mutex m_HelperLock;
};


#endif