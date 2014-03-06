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
	UserCore::MCFManagerI *mm = getUserCore()->getInternal()->getMCFManager();
	UserCore::Item::ItemInfoI* pItem = getItemInfo();

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

	m_bUnAuthed = dp->isUnAuthed();

	validateHeader(build, branch);

	if (isStopped())
		return;

	startToolDownload();

	UserCore::Item::BranchInfoI* curBranch = pItem->getCurrentBranch();

	if (!curBranch)
		throw gcException(ERR_NULLHANDLE, "Current branch is nullptr");

	gcString savePath = mm->getMcfPath(getItemId(), curBranch->getBranchId(), build, m_bUnAuthed);
		
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
	getItemHandle()->getInternal()->resetStage(true);
}

void ComboDownloadInstallTask::onStop()
{
	UserCore::ItemTask::BaseItemTask::onStop();
}

void ComboDownloadInstallTask::cancel()
{
	onStop();
	getItemHandle()->getInternal()->resetStage(true);
}

void ComboDownloadInstallTask::onProgress(MCFCore::Misc::ProgressInfo& p)
{
	if (m_bDownloading && p.flag == 1 && !m_bUpdating)
	{
		m_bDownloading = false;

		auto pItem = getItemInfo();
		pItem->delSFlag(UserCore::Item::ItemInfoI::STATUS_DOWNLOADING);
		pItem->addSFlag(UserCore::Item::ItemInfoI::STATUS_INSTALLING);
	}

	onMcfProgressEvent(p);
	getItemInfo()->getInternal()->setPercent(p.percent);
}

void ComboDownloadInstallTask::updateStatusFlags()
{
	auto pItem = getItemInfo();
	uint32 flags = UserCore::Item::ItemInfoI::STATUS_DELETED|UserCore::Item::ItemInfoI::STATUS_LINK|UserCore::Item::ItemInfoI::STATUS_VERIFING|UserCore::Item::ItemInfoI::STATUS_PAUSED|UserCore::Item::ItemInfoI::STATUS_PRELOADED;

	pItem->getInternal()->setPercent(0);
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
	auto pItem = getItemInfo();
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

	auto pItem = getItemInfo();

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

	if (m_bToolDownloadComplete)
		getItemHandle()->getInternal()->completeStage(false);
	else
		getItemHandle()->getInternal()->goToStageDownloadTools(false, m_ToolTTID);
}


void ComboDownloadInstallTask::startToolDownload()
{
	//dont download tools for preorders just yet
	if (getItemInfo()->getCurrentBranch()->isPreOrder())
		return;

	std::vector<DesuraId> toolList;
	getItemInfo()->getCurrentBranch()->getToolList(toolList);

	if (toolList.size() == 0)
		return;

	auto pToolManager =  getUserCore()->getToolManager();

	if (!pToolManager->areAllToolsValid(toolList))
	{
		pToolManager->reloadTools(getItemId());
		getItemInfo()->getCurrentBranch()->getToolList(toolList);

		if (!pToolManager->areAllToolsValid(toolList))
			throw gcException(ERR_INVALID, "Tool ids cannot be resolved into tools.");
	}

	m_bToolDownloadComplete = false;

	UserCore::Misc::ToolTransaction* tt = new UserCore::Misc::ToolTransaction();

	tt->onCompleteEvent += delegate(this, &ComboDownloadInstallTask::onToolComplete);
	tt->toolsList = toolList;
	
	m_ToolTTID = pToolManager->downloadTools(tt);
}

void ComboDownloadInstallTask::onToolComplete()
{
	m_bToolDownloadComplete = true;
}


#if defined(WITH_GTEST) && defined(WITH_GMOCK)

#include "TaskTestingBase.h"

namespace UnitTest
{
	using namespace ::testing;

	class ComboDownloadInstallTaskFixture : public BaseTaskTestingFixture<>
	{
	public:
		ComboDownloadInstallTaskFixture()
			:  m_Task(&m_ItemHandle, m_Branch, m_Build)
		{
			m_Task.setUserCore(&m_User);
			m_Task.setWebCore(&m_WebCore);
			m_Task.m_hMCFile.setHandle(&m_Mcf);

			ON_CALL(m_Mcf, downloadAndInstall(_)).WillByDefault(Invoke(this, &ComboDownloadInstallTaskFixture::downloadAndInstall));
			ON_CALL(m_McfManager, getMcfPath(_, _, _, _)).WillByDefault(Return(m_strMcfSavePath));

			m_Task.onMcfProgressEvent += delegate(this, &ComboDownloadInstallTaskFixture::onTaskProgress);
		}

		~ComboDownloadInstallTaskFixture()
		{
			m_Task.m_hMCFile.releaseHandle();
		}

		void downloadAndInstall(const char* szPath)
		{
			if (m_bFail)
			{
				gcException e;
				m_McfErrorEvent(e);
				return;
			}

			MCFCore::Misc::ProgressInfo p;

			p.percent = 50;
			p.flag = 0;

			m_McfProgressEvent(p);

			p.percent = 100;
			p.flag = 1;

			m_McfProgressEvent(p);
		}

		void run(bool bFail = false)
		{
			if (bFail)
			{
				m_bFail = true;
				m_Task.doRun();
			}
			else
			{
				EXPECT_CALL(m_GameExplorerManager, addItem(m_Id));
				EXPECT_CALL(m_ServiceMain, setUninstallRegKey(m_Id.toInt64(), m_nInsSize));
				EXPECT_CALL(m_ServiceMain, fixFolderPermissions(_));

				m_Task.doRun();

				ASSERT_TRUE(m_ItemInfo.isInstalled());
				ASSERT_TRUE(m_ItemInfo.isLaunchable());
				ASSERT_FALSE(m_ItemInfo.isUpdating());				
			}
		}

		uint32 downloadTools(UserCore::Misc::ToolTransaction* tt)
		{
			if (m_bCompleteToolDownload)
				tt->onCompleteEvent();

			delete tt;
			return 1;
		}

		void setupUpgrade(const char* szItemInfo)
		{
			m_ItemInfo.addSFlag(UserCore::Item::ItemInfoI::STATUS_READY|UserCore::Item::ItemInfoI::STATUS_UPDATEAVAL|UserCore::Item::ItemInfoI::STATUS_INSTALLED|UserCore::Item::ItemInfoI::STATUS_ONCOMPUTER);
			m_ItemInfo.setInstalledMcf(m_Branch, MCFBuild::BuildFromInt(1));
			m_bUpgrading = true;

			XML::gcXMLDocument doc;
			doc.LoadBuffer(szItemInfo, strlen(szItemInfo));
			m_ItemInfo.getInternal()->getBranchOrCurrent(m_Branch)->processUpdateXml(doc.GetRoot("game").FirstChildElement("branches").FirstChildElement("branch"));
		}

		void setupUnAuthed()
		{
			m_bUnAuthedDownload = true;
		}

		void setupToolComplete()
		{
			m_bCompleteToolDownload = true;
		}

		void onTaskProgress(MCFCore::Misc::ProgressInfo &p)
		{
			if (m_bUpgrading)
			{
				ASSERT_FALSE(HasAnyFlags(m_ItemInfo.getStatus(), UserCore::Item::ItemInfoI::STATUS_DOWNLOADING));
				ASSERT_FALSE(HasAnyFlags(m_ItemInfo.getStatus(), UserCore::Item::ItemInfoI::STATUS_UPLOADING));
			}
		}

		bool m_bUpgrading = false;
		bool m_bCompleteToolDownload = false;
		bool m_bFail = false;
		const gcString m_strMcfSavePath = "C:\\TestPath";
		ComboDownloadInstallTask m_Task;
	};

	TEST_F(ComboDownloadInstallTaskFixture, New_NoTools)
	{
		EXPECT_CALL(m_ItemHandleInternal, completeStage(false));

		setup(g_szItemInfo_Default);
		run();
	}

	TEST_F(ComboDownloadInstallTaskFixture, New_NoTools_Preload)
	{
		EXPECT_CALL(m_ItemHandleInternal, completeStage(false));

		setup(g_szItemInfo_Preload);
		run();
	}

	TEST_F(ComboDownloadInstallTaskFixture, New_NoTools_Preorder)
	{
		EXPECT_CALL(m_ItemHandleInternal, completeStage(false));

		setup(g_szItemInfo_Preorder);
		run();
	}

	TEST_F(ComboDownloadInstallTaskFixture, New_NoTools_UnAuth)
	{
		EXPECT_CALL(m_McfManager, getMcfPath(_, _, _, true)).WillRepeatedly(Return(m_strMcfSavePath));
		EXPECT_CALL(m_ItemHandleInternal, completeStage(false));

		setup(g_szItemInfo_Default);
		setupUnAuthed();

		run();
	}

	TEST_F(ComboDownloadInstallTaskFixture, New_Tools_InvalidTools_DlFin)
	{
		EXPECT_CALL(m_ToolManager, reloadTools(_));
		EXPECT_CALL(m_ToolManager, areAllToolsValid(_))
			.WillOnce(Return(false))
			.WillOnce(Return(true));
		EXPECT_CALL(m_ToolManager, downloadTools(_)).WillOnce(Invoke(this, &ComboDownloadInstallTaskFixture::downloadTools));

		EXPECT_CALL(m_ItemHandleInternal, completeStage(false));

		setup(g_szItemInfo_Default_Tools);
		setupToolComplete();

		run();
	}

	TEST_F(ComboDownloadInstallTaskFixture, New_Tools_DlFin)
	{
		EXPECT_CALL(m_ToolManager, reloadTools(_)).Times(0);
		EXPECT_CALL(m_ToolManager, areAllToolsValid(_)).WillOnce(Return(true));
		EXPECT_CALL(m_ToolManager, downloadTools(_)).WillOnce(Invoke(this, &ComboDownloadInstallTaskFixture::downloadTools));

		EXPECT_CALL(m_ItemHandleInternal, completeStage(false));

		setup(g_szItemInfo_Default_Tools);
		setupToolComplete();

		run();
	}

	TEST_F(ComboDownloadInstallTaskFixture, New_Tools_DlNotFin)
	{
		EXPECT_CALL(m_ToolManager, reloadTools(_)).Times(0);
		EXPECT_CALL(m_ToolManager, areAllToolsValid(_)).WillOnce(Return(true));
		EXPECT_CALL(m_ToolManager, downloadTools(_)).WillOnce(Invoke(this, &ComboDownloadInstallTaskFixture::downloadTools));

		EXPECT_CALL(m_ItemHandleInternal, completeStage(_)).Times(0);
		EXPECT_CALL(m_ItemHandleInternal, goToStageDownloadTools(An<bool>(), An<ToolTransactionId>()));

		setup(g_szItemInfo_Default_Tools);

		run();
	}

	TEST_F(ComboDownloadInstallTaskFixture, Upgrade_NoTools)
	{
		EXPECT_CALL(m_ItemHandleInternal, completeStage(false));

		setup(g_szItemInfo_Default);
		setupUpgrade(g_szItemInfo_Default);

		run();
	}

	TEST_F(ComboDownloadInstallTaskFixture, Upgrade_NoTools_UnAuth)
	{
		EXPECT_CALL(m_McfManager, getMcfPath(_, _, _, true)).WillRepeatedly(Return(m_strMcfSavePath));
		EXPECT_CALL(m_ItemHandleInternal, completeStage(false));

		setup(g_szItemInfo_Default);
		setupUpgrade(g_szItemInfo_Default);
		setupUnAuthed();

		run();
	}

	TEST_F(ComboDownloadInstallTaskFixture, New_NoTools_Fail)
	{
		EXPECT_CALL(m_ItemHandleInternal, resetStage(true));
		setup(g_szItemInfo_Default);

		run(true);
	}


	TEST_F(ComboDownloadInstallTaskFixture, Upgrade_NoTools_Fail)
	{
		EXPECT_CALL(m_ItemHandleInternal, resetStage(true));

		setup(g_szItemInfo_Default);
		setupUpgrade(g_szItemInfo_Default);

		run(true);
	}
}

#endif

