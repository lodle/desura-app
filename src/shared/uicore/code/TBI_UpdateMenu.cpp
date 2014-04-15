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
#include "TBI_UpdateMenu.h"

#include "MainApp.h"
#include "wx_controls/gcCustomMenu.h"

#include "ButtonStrip.h"

enum
{
	mcMENU_MESSAGE = 800,
	mcMENU_UPDATE,
	mcMENU_CART,
	mcMENU_GAMEUPDATE,
	mcMENU_MODUPDATE,
};


wxMenu* TBIUpdateMenu::createMenu(uint32 &lastMenuId)
{
	calcUpdates();
	gcMenu* menu = new gcMenu();

	gcWString m(L"{0} - {1}", Managers::GetString(L"#TB_MESSAGE"), messageCount);
	gcWString u(L"{0} - {1}", Managers::GetString(L"#TB_UPDATES"), updateCount);
	gcWString g(L"{0} - {1}", Managers::GetString(L"#TB_GAMES"), gameUpdateCount);
	gcWString mo(L"{0} - {1}", Managers::GetString(L"#TB_MODS"), modUpdateCount);
	gcWString c(L"{0} - {1}", Managers::GetString(L"#TB_CART"), cartCount);

	menu->Append(mcMENU_GAMEUPDATE, g.c_str());
	menu->Append(mcMENU_MODUPDATE, mo.c_str());
	menu->AppendSeparator();
	menu->Append(mcMENU_MESSAGE, m.c_str());
	menu->Append(mcMENU_UPDATE, u.c_str());
	menu->Append(mcMENU_CART, c.c_str());

	return menu;
}

void TBIUpdateMenu::onMenuSelect(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case mcMENU_MESSAGE:
		g_pMainApp->loadUrl(GetWebCore()->getUrl(WebCore::Inbox).c_str(), COMMUNITY);
		break;

	case mcMENU_UPDATE:
		g_pMainApp->loadUrl(GetWebCore()->getUrl(WebCore::Updates).c_str(), COMMUNITY);
		break;

	case mcMENU_CART:
		g_pMainApp->loadUrl(GetWebCore()->getUrl(WebCore::Cart).c_str(), COMMUNITY);
		break;

	case mcMENU_GAMEUPDATE:
	case mcMENU_MODUPDATE:
		g_pMainApp->handleInternalLink(DesuraId(), ACTION_PLAY);
		break;
	}
}

const wchar_t* TBIUpdateMenu::getMenuName()
{
	calcUpdates();
	return m_szMenuName.c_str();
}

void TBIUpdateMenu::calcUpdates()
{
	UserCore::UserI* user = GetUserCore();

	if (!user)
	{
		messageCount = 0;
		updateCount = 0;
		cartCount = 0;
	}
	else
	{
		messageCount = user->getPmCount();
		updateCount = user->getUpCount();
		cartCount = user->getCartCount();
	}

	gameUpdateCount = 0;
	modUpdateCount = 0;

	auto pItemManager = getItemManager();

	if (!pItemManager)
		return;

	std::vector<UserCore::Item::ItemInfoI*> gList;
	pItemManager->getGameList(gList);

	for (size_t x=0; x<gList.size(); x++)
	{
		if (HasAnyFlags(gList[x]->getStatus(), UserCore::Item::ItemInfoI::STATUS_UPDATEAVAL))
			gameUpdateCount++;

		std::vector<UserCore::Item::ItemInfoI*> mList;
		pItemManager->getModList(gList[x]->getId(), mList);

		for (size_t y=0; y<mList.size(); y++)
		{
			if (HasAnyFlags(mList[y]->getStatus(), UserCore::Item::ItemInfoI::STATUS_UPDATEAVAL))
				gameUpdateCount++;
		}
	}

	uint32 numUpdates = messageCount + updateCount + gameUpdateCount + modUpdateCount;
	m_szMenuName = gcWString(L"{0} ({1})", Managers::GetString(L"#TB_STATUS"), numUpdates);
}
