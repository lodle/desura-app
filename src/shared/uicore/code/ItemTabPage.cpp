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
#include "ItemTabPage.h"
#include "ItemToolBarControl.h"
#include "Managers.h"
#include "MainApp.h"

#include "gcWebHost.h"
#include "gcWebControl.h"


ItemTabPage::ItemTabPage(wxWindow* parent, gcWString homePage) 
	: HtmlTabPage(parent, homePage, ITEMS)
	, m_PingTimer(this)
{
	m_bNotifiedOfLowSpace = false;
	m_pItemControlBar = new ItemToolBarControl(parent);
	m_pItemControlBar->onSearchEvent += guiDelegate(this, &ItemTabPage::onSearchStr);

	m_pWebControl = nullptr;

	if (m_pWebControl)
	{
		m_pWebControl->onPageStartEvent -= delegate(&m_pControlBar->onPageStartLoadingEvent);
		m_pWebControl->onPageLoadEvent -= delegate(&m_pControlBar->onPageEndLoadingEvent);

		m_pWebControl->onPageStartEvent += delegate(&m_pItemControlBar->onPageStartLoadingEvent);
		m_pWebControl->onPageLoadEvent += delegate(&m_pItemControlBar->onPageEndLoadingEvent);
	}

	killControlBar();

	m_pItemControlBar->onButtonClickedEvent += guiDelegate(this, &ItemTabPage::onButtonClicked);
	m_JSEventMap.setValid(true);

	Bind(wxEVT_TIMER, &ItemTabPage::onPingTimer, this);
}

ItemTabPage::~ItemTabPage()
{
	m_PingTimer.Stop();

	m_JSEventMap.setValid(false);
	m_JSEventMap.reset();

	m_pItemControlBar->onButtonClickedEvent -= guiDelegate(this, &ItemTabPage::onButtonClicked);
	m_pItemControlBar->onSearchEvent -= guiDelegate(this, &ItemTabPage::onSearchStr);

	m_pWebControl->onPageStartEvent -= delegate(&m_pItemControlBar->onPageStartLoadingEvent);
	m_pWebControl->onPageLoadEvent -= delegate(&m_pItemControlBar->onPageEndLoadingEvent);

	if (GetUserCore())
	{
		std::vector<UserCore::Item::ItemInfoI*> aList;
		GetUserCore()->getItemManager()->getAllItems(aList);

		for (size_t x=0; x<aList.size(); x++)
			*aList[x]->getInfoChangeEvent() -= guiDelegate(this, &ItemTabPage::onItemUpdate);

		*GetUserCore()->getLowSpaceEvent() -= guiDelegate(this, &ItemTabPage::onLowDiskSpace);
		*GetUserCore()->getForcedUpdatePollEvent() -= guiDelegate(this, &ItemTabPage::onUpdatePoll);
	}

	if (GetUploadMng())
	{
		size_t count = GetUploadMng()->getCount();

		for (size_t x=0; x<count; x++)
		{
			UserCore::Misc::UploadInfoThreadI* item = GetUploadMng()->getItem(x);

			gcString key = item->getKey();
			*item->getUploadProgressEvent() -= guiExtraDelegate(this, &ItemTabPage::onUploadProgress, key);
			*item->getActionEvent() -= guiExtraDelegate(this, &ItemTabPage::onUploadAction, key);

			item->getUploadProgressEvent()->flush();
			item->getActionEvent()->flush();
		}

		*GetUploadMng()->getUpdateEvent() -= guiDelegate(this, &ItemTabPage::onUploadUpdate);
		GetUploadMng()->getUpdateEvent()->flush();
	}
}

void ItemTabPage::onFind()
{
	if (m_pItemControlBar)
		m_pItemControlBar->focusSearch();
}

void ItemTabPage::postEvent(const char* name, const char* arg1, const char* arg2)
{
	gcWebControl* webCtrl = dynamic_cast<gcWebControl*>(m_pWebControl);

	if (!webCtrl)
		return;

	BrowserUICallback(new JSCallback(m_JSEventMap, m_bGlobalItemUpdate, webCtrl->getJSContext(), name, arg1, arg2));
}

BaseToolBarControl* ItemTabPage::getToolBarControl()
{
	return m_pItemControlBar;
}

void ItemTabPage::newBrowser(const char* homeUrl)
{
	if (m_pWebControl)
		return;

	gcWebControl* host = new gcWebControl(this, homeUrl);

	m_pWebPanel = host;
	m_pWebControl = host; 
}

void ItemTabPage::constuctBrowser()
{
	HtmlTabPage::constuctBrowser();

	m_pWebControl->onPageStartEvent += delegate(&m_pItemControlBar->onPageStartLoadingEvent);
	m_pWebControl->onPageLoadEvent += delegate(&m_pItemControlBar->onPageEndLoadingEvent);
	m_pWebControl->onPageLoadEvent += delegate(this, &ItemTabPage::doneLoading);

	if (!GetUserCore())
		return;

	UserCore::ItemManagerI *im = GetUserCore()->getItemManager();

	if (im)
	{
		*im->getOnUpdateEvent() += guiDelegate(this, &ItemTabPage::onItemsUpdate);
		*im->getOnRecentUpdateEvent() += guiDelegate(this, &ItemTabPage::onRecentUpdate);
		*im->getOnFavoriteUpdateEvent() += guiDelegate(this, &ItemTabPage::onFavoriteUpdate);
		*im->getOnNewItemEvent() += guiDelegate(this, &ItemTabPage::onNewItem);

		onItemsUpdate();
	}

	*GetUserCore()->getLowSpaceEvent() += guiDelegate(this, &ItemTabPage::onLowDiskSpace);
	*GetUserCore()->getForcedUpdatePollEvent() += guiDelegate(this, &ItemTabPage::onUpdatePoll);
	*GetUploadMng()->getUpdateEvent() += guiDelegate(this, &ItemTabPage::onUploadUpdate);
	onUploadUpdate();

	*GetUserCore()->getLoginItemsLoadedEvent() += guiDelegate(this, &ItemTabPage::onLoginItemsLoaded);

	GetJSBinding()->onPingEvent += guiDelegate(this, &ItemTabPage::onPing);
	m_PingTimer.Start(15 * 1000);
}


void ItemTabPage::reset()
{
	postEvent("onTabClicked", "game");
}

void ItemTabPage::onSearchText(const wchar_t* value)
{
	postEvent("onSearch", gcString(value).c_str());
}

void ItemTabPage::onButtonClicked(int32& id)
{ 
	switch (id)
	{
	case BUTTON_EXPAND:
		postEvent("onExpand");
		break;

	case BUTTON_CONTRACT:
		postEvent("onContract");
		break;

	case BUTTON_GAME:
		postEvent("onTabClicked", "game");
		break;
		
	case BUTTON_FAV:
		postEvent("onTabClicked", "fav");
		break;

	case BUTTON_TOOL:
		postEvent("onTabClicked", "tool");
		break;

	case BUTTON_DEV:
		postEvent("onTabClicked", "dev");
		break;
	}
}

void ItemTabPage::doneLoading()
{
	m_JSEventMap.reset();

	if (GetUserCore() && GetUserCore()->isDelayLoading())
		postEvent("onDelayLoad");
}

void ItemTabPage::onLoginItemsLoaded()
{
	postEvent("onDelayLoadDone");
	postEvent("onItemListUpdated");
}

void ItemTabPage::onItemsUpdate()
{
	std::vector<UserCore::Item::ItemInfoI*> aList;
	GetUserCore()->getItemManager()->getAllItems(aList);

	for (size_t x=0; x<aList.size(); x++)
		*aList[x]->getInfoChangeEvent() += guiDelegate(this, &ItemTabPage::onItemUpdate);

	postEvent("onItemListUpdated");
}

void ItemTabPage::onRecentUpdate(DesuraId &id)
{
	postEvent("onRecentUpdate", id.toString().c_str());
}

void ItemTabPage::onFavoriteUpdate(DesuraId &id)
{
	postEvent("onFavoriteUpdate", id.toString().c_str());
}

void ItemTabPage::onItemUpdate(UserCore::Item::ItemInfoI::ItemInfo_s& info)
{
	postEvent("onItemUpdate", info.id.toString().c_str(), gcString("{0}", info.changeFlags).c_str());
}

void ItemTabPage::onUploadUpdate()
{
	size_t count = GetUploadMng()->getCount();

	for (size_t x=0; x<count; x++)
	{
		UserCore::Misc::UploadInfoThreadI* item = GetUploadMng()->getItem(x);

		gcString key = item->getKey();

		if (item->isDeleted())
		{
			*item->getUploadProgressEvent()  -= guiExtraDelegate(this, &ItemTabPage::onUploadProgress, key);
			*item->getActionEvent() -= guiExtraDelegate(this, &ItemTabPage::onUploadAction, key);
		}
		else
		{
			*item->getUploadProgressEvent()  += guiExtraDelegate(this, &ItemTabPage::onUploadProgress, key);
			*item->getActionEvent() += guiExtraDelegate(this, &ItemTabPage::onUploadAction, key);
		}
	}

	postEvent("onUploadUpdate", "all");
}

void ItemTabPage::onUploadProgress(gcString hash, UserCore::Misc::UploadInfo& info)
{
	uint32 prog = 0;

	m_UploadMutex.lock();
	prog = m_vUploadProgress[hash];
	m_vUploadProgress[hash] = info.percent;
	m_UploadMutex.unlock();

	if (prog != info.percent)
		postEvent("onUploadProgress", hash.c_str());
}

void ItemTabPage::onUploadAction(gcString hash)
{
	postEvent("onUploadUpdate", hash.c_str());
}

void ItemTabPage::onUpdatePoll(std::tuple<gcOptional<bool>, gcOptional<bool>, gcOptional<bool>> &info)
{
	auto forcedUpdate = std::get<0>(info);

	if (forcedUpdate && *forcedUpdate)
		postEvent("onUpdatePoll", "all");
}

void ItemTabPage::onNewItem(DesuraId &id)
{
	postEvent("onNewItemAdded", id.toString().c_str());
}

void ItemTabPage::onSearchStr(gcString &text)
{
	postEvent("onSearch", text.c_str());
}

void ItemTabPage::onShowAlert(const gcString &text, uint32 time)
{
	postEvent("onShowAlert", text.c_str(), gcString("{0}", time).c_str());
}

void ItemTabPage::onLowDiskSpace(std::pair<bool,char> &info)
{
	if (m_bNotifiedOfLowSpace)
		return;

	m_bNotifiedOfLowSpace = true;

#ifdef WIN32
	gcString text;
	
	if (info.first)
		text = gcString(Managers::GetString("#IF_LOWSPACE_SYS"), info.second);
	else
		text = gcString(Managers::GetString("#IF_LOWSPACE_CACHE"), info.second);

	onShowAlert(text, 0);
#endif
}

void ItemTabPage::onPingTimer(wxTimerEvent&)
{
	if (!m_bPingBack)
	{
		Warning("Item tab page did not ping back after 15 seconds\n");
		m_pWebControl->refresh();
	}
	else
	{
		postEvent("onPing");
	}
		
	m_bPingBack = false;
}

void ItemTabPage::onPing()
{
	m_bPingBack = true;
}
