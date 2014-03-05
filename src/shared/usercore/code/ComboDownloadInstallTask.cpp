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
#include "ComboDownloadInstallTask.h"

#include "McfManager.h"
#include "MCFDownloadProviders.h"

#include "usercore/ItemInfoI.h"
#include "UserTasks.h"
#include "User.h"

#include "service_pipe/IPCServiceMain.h"
#include "GameExplorerManager.h"

using namespace UserCore::ItemTask;

ComboDownloadInstallTask::ComboDownloadInstallTask(UserCore::Item::ItemHandleI* handle, MCFBranch branch, MCFBuild build)
	: BaseItemTask(UserCore::Item::ITEM_STAGE::STAGE_COMBO_DL_IN, "ComboDownloadAndInstall", handle, branch, build)
{
	onErrorEvent += delegate(this, &ComboDownloadInstallTask::onError);
}

ComboDownloadInstallTask::~ComboDownloadInstallTask()
{

}

void ComboDownloadInstallTask::doRun()
{
	UserCore::MCFManager *mm = UserCore::GetMCFManager();
	UserCore::Item::ItemInfo* pItem = getItemInfo();

	if (!pItem)
		throw gcException(ERR_BADID);

	m_LastInsBuild = pItem->getInstalledBuild();

	MCFBuild build;
	MCFBranch branch;

	updateStatusFlags();

	m_hMCFile->setHeader(getItemId(), getMcfBranch(), getMcfBuild());
	m_hMCFile->getErrorEvent() += delegate(&onErrorEvent);
	m_hMCFile->getProgEvent() += delegate(this, &ComboDownloadInstallTask::onProgress);

	auto dp = std::make_shared<MCFDownloadProviders>(getWebCore(), getUserCore()->getUserId());
	MCFDownloadProviders::forceLoad(m_hMCFile, dp);

	validateHeader(build, branch);

	if (isStopped())
		return;

	UserCore::Item::BranchInfoI* curBranch = pItem->getCurrentBranch();

	if (!curBranch)
		throw gcException(ERR_NULLHANDLE, "Current branch is nullptr");

	gcString savePath = mm->getMcfPath(getItemId(), curBranch->getBranchId(), build);
		
	if (savePath == "")
		savePath = mm->newMcfPath(getItemId(), curBranch->getBranchId(), build, m_bUnAuthed);

	if (isStopped())
		return;

	m_hMCFile->setFile(savePath.c_str());
	m_hMCFile->saveMCFHeader();

	auto installPath = pItem->getPath();

	getUserCore()->getGameExplorerManager()->addItem(getItemId());

#ifdef WIN32
	getUserCore()->getServiceMain()->setUninstallRegKey(getItemId().toInt64(), m_hMCFile->getINSize());
	getUserCore()->getServiceMain()->fixFolderPermissions(installPath);
#endif

	if (isStopped())
		return;

	m_hMCFile->downloadAndInstall(savePath.c_str());

	if (isStopped())
		return;

	onComplete();
}

void ComboDownloadInstallTask::onPause()
{
	m_hMCFile->pause();
}

void ComboDownloadInstallTask::onUnpause() 
{
	m_hMCFile->unpause();
}

void ComboDownloadInstallTask::onError(gcException &e)
{
	Warning(gcString("Error in MCF Download And Install: {0}\n", e));
	m_bInError=true;
	getItemHandle()->completeStage(true);

	UserCore::Item::ItemInfo* pItem = getItemInfo();

	if (pItem)
	{
		if (m_bUpdating)
		{
			pItem->delSFlag(UserCore::Item::ItemInfoI::STATUS_UPDATING);
			pItem->addSFlag(UserCore::Item::ItemInfoI::STATUS_READY);
		}
		else
		{
			pItem->delSFlag(UserCore::Item::ItemInfoI::STATUS_DOWNLOADING);
		}
	}
}

void ComboDownloadInstallTask::onStop()
{
	UserCore::ItemTask::BaseItemTask::onStop();
}

void ComboDownloadInstallTask::cancel()
{
	onStop();
	getItemHandle()->resetStage(true);
}

void ComboDownloadInstallTask::onProgress(MCFCore::Misc::ProgressInfo& p)
{
	if (m_bDownloading && p.flag == 1)
	{
		m_bDownloading = false;

		UserCore::Item::ItemInfo* pItem = getItemInfo();
		pItem->delSFlag(UserCore::Item::ItemInfoI::STATUS_DOWNLOADING);
		pItem->addSFlag(UserCore::Item::ItemInfoI::STATUS_INSTALLING);
	}

	onMcfProgressEvent(p);
	getItemInfo()->setPercent(p.percent);
}

void ComboDownloadInstallTask::updateStatusFlags()
{
	UserCore::Item::ItemInfo* pItem = getItemInfo();
	uint32 flags = UserCore::Item::ItemInfoI::STATUS_DELETED|UserCore::Item::ItemInfoI::STATUS_LINK|UserCore::Item::ItemInfoI::STATUS_VERIFING|UserCore::Item::ItemInfoI::STATUS_PAUSED|UserCore::Item::ItemInfoI::STATUS_PRELOADED;

	pItem->setPercent(0);
	pItem->delSFlag(flags);

	uint32 num = 0;
	getUserCore()->getItemsAddedEvent()->operator()(num);

	MCFBuild build  = getMcfBuild();
	MCFBranch branch = getMcfBranch();

	m_bUpdating = pItem->isUpdating() && branch == pItem->getInstalledBranch() && (build == 0 || build == pItem->getNextUpdateBuild());

	if (m_bUpdating)
	{
		pItem->delSFlag(UserCore::Item::ItemInfoI::STATUS_READY);
		pItem->addSFlag(UserCore::Item::ItemInfoI::STATUS_UPDATING);
		pItem->delOFlag(UserCore::Item::ItemInfoI::OPTION_REMOVEFILES);
	}
	else
	{
		pItem->addSFlag(UserCore::Item::ItemInfoI::STATUS_ONCOMPUTER|UserCore::Item::ItemInfoI::STATUS_DOWNLOADING);
	}
}

void ComboDownloadInstallTask::validateHeader(MCFBuild &build, MCFBranch &branch)
{
	UserCore::Item::ItemInfo* pItem = getItemInfo();
	build = m_hMCFile->getHeader()->getBuild();
	branch = m_hMCFile->getHeader()->getBranch();

	if (getMcfBranch() != branch)
		throw gcException(ERR_BADID, "Branch from mcf is different to requested branch");

	this->m_uiMcfBuild = build;

	if (m_bUpdating && build <= pItem->getInstalledBuild())
		throw gcException(ERR_NOUPDATE, "The installed version is the same as the newest version. No Update available.");

	if (!pItem->setInstalledMcf(branch, build))
		throw gcException(ERR_BADID, "Failed to set branch id.");

	if (m_bUnAuthed)
		pItem->addSFlag(UserCore::Item::ItemInfoI::STATUS_UNAUTHED);
}

void ComboDownloadInstallTask::onComplete()
{
	if (m_bInError)
		return;

#ifdef NIX
	getItemHandle()->installLaunchScripts();
#endif

	UserCore::Item::ItemInfo *pItem = getItemInfo();

	if (pItem->isUpdating() && getMcfBuild() == pItem->getNextUpdateBuild())
		pItem->updated();

	pItem->delSFlag(UserCore::Item::ItemInfoI::STATUS_INSTALLING|UserCore::Item::ItemInfoI::STATUS_UPDATING|UserCore::Item::ItemInfoI::STATUS_DOWNLOADING);
	pItem->addSFlag(UserCore::Item::ItemInfoI::STATUS_INSTALLED|UserCore::Item::ItemInfoI::STATUS_READY);
	
	if (pItem->isUpdating())
		pItem->addSFlag(UserCore::Item::ItemInfoI::STATUS_NEEDCLEANUP);

	MCFCore::Misc::ProgressInfo temp;
	temp.percent = 100;

	onMcfProgressEvent(temp);

	UserCore::Item::ItemInfoI *item = getItemHandle()->getItemInfo();
	item->delSFlag(UserCore::Item::ItemInfoI::STATUS_PAUSABLE);

	bool verify = false;

	uint32 res = 0;
	onCompleteEvent(res);
	getItemHandle()->completeStage(false);
}



#if defined(WITH_GTEST) && defined(WITH_GMOCK)

#include <gtest/gtest.h>

#include "BranchInfo.h"
#include "BranchInstallInfo.h"
#include "ItemInfo.h"
#include "mcfcore/MCFI.h"
#include "mcfcore/MCFHeaderI.h"

using namespace IPC;
using namespace UserCore;
using namespace WebCore;
using namespace MCFCore;
using namespace UserCore::Item;

namespace UnitTest
{
	using namespace ::testing;

	class ComboDownloadInstallTaskFixture : public ::testing::Test
	{
	public:
		ComboDownloadInstallTaskFixture()
			: m_Id("123", "games")
			, m_Branch(MCFBranch::BranchFromInt(1))
			, m_Build(MCFBuild::BuildFromInt(2))
			, m_BranchInstallInfo(1, &m_ItemInfo)
			, m_BranchInfo(m_Branch, m_Id, &m_BranchInstallInfo, m_nUserId, 100)
			, m_ItemInfo(&m_User, m_Id)
			, m_Task(&m_ItemHandle, m_Branch, m_Build)
		{
			m_Task.setUserCore(&m_User);
			m_Task.setWebCore(&m_WebCore);
			m_Task.m_hMCFile.setHandle(&m_Mcf);

			ON_CALL(m_User, getServiceMain()).WillByDefault(Return(&m_ServiceMain));
			ON_CALL(m_User, getWebCore()).WillByDefault(Return(&m_WebCore));
			ON_CALL(m_User, getGameExplorerManager()).WillByDefault(Return(&m_GameExplorerManager));
			ON_CALL(m_User, getUserId()).WillByDefault(Return(m_nUserId));
			ON_CALL(m_User, getItemsAddedEvent()).WillByDefault(Return(&m_ItemAddedEvent));
			ON_CALL(m_Mcf, getHeader()).WillByDefault(Return(&m_McfHeader));

			ON_CALL(m_McfHeader, getBuild()).WillByDefault(Return(m_Build));
			ON_CALL(m_McfHeader, getBranch()).WillByDefault(Return(m_Branch));
			ON_CALL(m_McfHeader, getId()).WillByDefault(Return(m_Id.getItem()));
			ON_CALL(m_McfHeader, getType()).WillByDefault(Return(m_Id.getType()));

			ON_CALL(m_WebCore, getDownloadProviders(_, _, _, _)).WillByDefault(Invoke(this, &ComboDownloadInstallTaskFixture::getDownloadProviders));

		}

		~ComboDownloadInstallTaskFixture()
		{
			m_Task.m_hMCFile.setHandle(nullptr);
		}

		void getDownloadProviders(DesuraId id, XML::gcXMLDocument &xmlDocument, MCFBranch mcfBranch, MCFBuild mcfBuild)
		{
			const char* szProviders = 
				"<itemdownloadurl>"
				"	<status code=\"0\"/>"
				"	<item sitearea=\"games\" siteareaid=\"123\">"
				"		<name>Sample Mod</name>"
				"		<mcf build=\"2\" id=\"13\" branch=\"1\">"
				"		<urls>"
				"			<url>"
				"				<link>mcf://server:62001</link>"
				"				<provider>desura.com</provider>"
				"				<banner>http://www.desura.com/banner.png</banner>"
				"				<provlink>http://www.desura.com</provlink>"
				"			</url>"
				"		</urls>"
				"		<version>1.0</version>"
				"		<installsize>1024</installsize>"
				"		<filesize>1024</filesize>"
				"		<filehash>##############</filehash>"
				"		<authhash>##############</authhash>"
				"		<authed>1</authed>"
				"		</mcf>"
				"	</item>"
				"</itemdownloadurl>";

			xmlDocument.LoadBuffer(szProviders, strlen(szProviders));
		}

		void run()
		{
			m_Task.doRun();
		}

		void setup(const char* itemInfoXml, const char* branchInfoXml, const char* branchInstallInfoXml)
		{
			{
				XML::gcXMLDocument doc(itemInfoXml, strlen(itemInfoXml));
				m_ItemInfo.loadXmlData(100, doc.GetRoot("game"), 0);
			}

			{
				XML::gcXMLDocument doc(branchInfoXml, strlen(branchInfoXml));
				m_BranchInfo.loadXmlData(doc.GetRoot("branch"));
			}

			{
				WildcardManager wildcard;
				XML::gcXMLDocument doc(branchInstallInfoXml, strlen(branchInstallInfoXml));
				m_BranchInstallInfo.processSettings(doc.GetRoot("branchinstallinfo"), &wildcard, false, true, nullptr);
			}
		}

		const int m_nUserId = 666;
		Event<uint32> m_ItemAddedEvent;

		DesuraId m_Id; 
		MCFBranch m_Branch;
		MCFBuild m_Build;

		MCFHeaderMock m_McfHeader;

		BranchInstallInfo m_BranchInstallInfo;

		BranchInfo m_BranchInfo;
		ItemInfo m_ItemInfo;

		ItemHandleMock m_ItemHandle;
		UserMock m_User;
		WebCoreMock m_WebCore;
		ServiceMainMock m_ServiceMain;
		MCFMock m_Mcf;
		GameExplorerManagerMock m_GameExplorerManager;

		ComboDownloadInstallTask m_Task;
	};

	const char* g_szItemInfo =
		"<game siteareaid=\"123\">"
		"	<name>Test Game</name>"
		"	<nameid>test-game</nameid>"
		"</game>";

	const char* g_szBranchInfo =                                                
		"<branch id=\"1\" platformid=\"100\">"
		"	<name>Test</name>"
		"	<nameon>1</nameon>"
		"	<global>1</global>"
		"	<free>0</free>"
		"	<price>5,00€ EUR</price>"
		"	<cdkey>0</cdkey>"
		"	<demo>0</demo>"
		"	<test>0</test>"
		"	<inviteonly>1</inviteonly>"
		"	<regionlock>0</regionlock>"
		"	<preload>0</preload>"
		"	<onaccount>1</onaccount>"
		"</branch>";

	const char* g_szBranchInstallInfo =
		"<settings>"
		"	<executes>"
		"		<execute>"
		"			<name>Play</name>"
		"			<exe>play.exe</exe>"
		"			<args>args</args>"
		"		</execute>"
		"	</executes>"
		"	<installlocations>"
		"		<installlocation>"
		"			<check>%APPLICATION%/play.exe</check>"
		"			<path>%APPLICATION%</path>"
		"		</installlocation>"
		"	</installlocations>"
		"</settings>";

	TEST_F(ComboDownloadInstallTaskFixture, New_NoTools)
	{
		setup(g_szItemInfo, g_szBranchInfo, g_szBranchInstallInfo);

		run();

		ASSERT_TRUE(m_ItemInfo.isInstalled());
		ASSERT_TRUE(m_ItemInfo.isLaunchable());
	}
}

#endif

