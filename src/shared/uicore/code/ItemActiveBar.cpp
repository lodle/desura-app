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
#include "ItemActiveBar.h"

#include "gcWebControl.h"
#include "MainApp.h"

#include "usercore/UserCoreI.h"
#include "usercore/ItemHandleI.h"
#include "usercore/ItemHelpersI.h"

using namespace UserCore::Item;
using namespace UserCore::Item::Helper;

namespace
{
	class ItemActiveBarHelper : public ItemHandleHelperI
	{
	public:
		ItemActiveBarHelper(ItemActiveHelperParent* pParent, DesuraId id)
			: m_pHost(pParent)
			, m_Id(id)
		{
		}

		void onComplete(uint32 status) override
		{
			m_pHost->postEvent(m_Id, "onActiveComplete", status);
		}

		void onComplete(gcString& string) override
		{
			m_pHost->postEvent(m_Id, "onActiveComplete", string.c_str());
		}

		void onMcfProgress(MCFCore::Misc::ProgressInfo& info) override
		{
			auto infoJSON = gcString("{\"done\": {0}, \"total\": {1}, \"rate\" : {2}, \"flag\": {3}, ", info.doneAmmount, info.totalAmmount, info.rate);
			infoJSON += gcString("\"hour\": {0}, \"min\": {1}, \"percent\" : {2}}", info.flag, info.hour, info.min, info.percent);

			m_pHost->postEvent(m_Id, "onActiveProgress", infoJSON.c_str());
		}

		void onProgressUpdate(uint32 progress) override
		{
			m_pHost->postEvent(m_Id, "onActiveProgress", progress);
		}

		void onError(gcException e) override
		{
			m_pHost->postEvent(m_Id, "onActiveError", e.getErrMsg());
		}

		void onNeedWildCard(WCSpecialInfo& info) override
		{
		}

		void onDownloadProvider(UserCore::Misc::GuiDownloadProvider &provider) override
		{
		}

		void onVerifyComplete(UserCore::Misc::VerifyComplete& info) override
		{
		}

		uint32 getId() override
		{
			return m_nStatus;
		}

		void setId(uint32 id) override
		{
			m_nStatus = id;
		}

		void onPause(bool state) override
		{
			m_bPaused = state;
			m_pHost->postEvent(m_Id, "onActivePause", state);
		}

		void onStageChange(ITEM_STAGE &stage)
		{
			m_nStage = stage;
			m_pHost->postEvent(m_Id, "onActiveStageChange", (uint32)stage);
		}

		void onErrorRef(gcException &e)
		{
			onError(e);
		}

	protected:


	private:
		ItemActiveHelperParent *m_pHost;

		DesuraId m_Id;

		bool m_bPaused = false;
		uint32 m_nStatus = 0;
		ITEM_STAGE m_nStage;
	};
}


ItemActiveBar::ItemActiveBar(wxWindow* parent)
	: gcPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 50))
{
	m_JSEventMap.setValid(true);

	const char* url = GetGCThemeManager()->getWebPage("activebar");

	auto pWebControl = new gcWebControl(this, url);

	m_pWebControl = pWebControl;
	m_pWebControl->onPageLoadEvent += delegate(this, &ItemActiveBar::doneLoading);

	auto pBSBrowserSizer = new wxBoxSizer( wxVERTICAL );
	pBSBrowserSizer->Add(pWebControl, 1, wxEXPAND, 0);

	this->SetSizer( pBSBrowserSizer );
	this->Layout();


	*GetUserCore()->getItemManager()->getOnNewItemEvent() += delegate(this, &ItemActiveBar::onNewItem);

	GetUserCore()->getItemManager()->for_each_handle([&](UserCore::Item::ItemHandleI* pItemHandle){
		setupHandle(pItemHandle);
	});
}

ItemActiveBar::~ItemActiveBar()
{
	for (auto i : m_pHelperMap)
		delete i.second;
}

void ItemActiveBar::setupHandle(UserCore::Item::ItemHandleI* pItemHandle)
{
	std::lock_guard<std::mutex> guard(m_HelperLock);

	auto id = pItemHandle->getItemInfo()->getId();

	if (m_pHelperMap.find(id.toInt64()) != m_pHelperMap.end())
		return;

	auto helper = new ItemActiveBarHelper(this, id);

	*pItemHandle->getChangeStageEvent() += delegate(helper, &ItemActiveBarHelper::onStageChange);
	*pItemHandle->getErrorEvent() += delegate(helper, &ItemActiveBarHelper::onErrorRef);

	pItemHandle->addHelper(helper);

	m_pHelperMap[id.toInt64()] = helper;
}

void ItemActiveBar::onNewItem(DesuraId &id)
{
	setupHandle(GetUserCore()->getItemManager()->findItemHandle(id));
}

void ItemActiveBar::postEvent(DesuraId id, const char* szEventName, uint32 nArg)
{
	auto strId = id.toString();
	gcString szArg("{0}", nArg);
	postEvent(szEventName, strId.c_str(), szArg.c_str());
}

void ItemActiveBar::postEvent(DesuraId id, const char* szEventName, const char* szArg)
{
	auto strId = id.toString();
	postEvent(szEventName, strId.c_str(), szArg);
}

void ItemActiveBar::postEvent(const char* name, const char* arg1, const char* arg2)
{
	gcWebControl* webCtrl = dynamic_cast<gcWebControl*>(m_pWebControl);

	if (!webCtrl)
		return;

	BrowserUICallback(new JSCallback(m_JSEventMap, m_bGlobalItemUpdate, webCtrl->getJSContext(), name, arg1, arg2));
}

void ItemActiveBar::doneLoading()
{
	m_JSEventMap.reset();
}