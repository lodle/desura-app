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
#include "ComboDLINTController.h"
#include "WGTWorker.h"

using namespace MCFCore::Thread;

ComboDLINTController::ComboDLINTController(std::shared_ptr<MCFCore::Misc::DownloadProvidersI> pDownloadProviders, uint16 numWorkers, MCFCore::MCF* caller, const char* szInstallPath)
	: MCFCore::Thread::BaseMCFThread(numWorkers, caller, "Combo DL IN Controller Thread")
	, m_ProvManager(pDownloadProviders)
	, m_szInstallPath(szInstallPath)
{
	m_ProvManager.onProviderEvent += delegate(&onProviderEvent);
	setPriority(BELOW_NORMAL);
}

ComboDLINTController::~ComboDLINTController()
{
	join();

	if (m_bDoingStop)
		gcSleep(500);


}

Misc::WGTSuperBlock* ComboDLINTController::newTask(uint32 id, uint32 &status)
{
}

uint32 ComboDLINTController::getStatus(uint32 id)
{
}

void ComboDLINTController::reportError(uint32 id, gcException &e)
{
}

void ComboDLINTController::reportProgress(uint32 id, uint64 ammount)
{
}

void ComboDLINTController::reportNegProgress(uint32 id, uint64 ammount)
{
}

void ComboDLINTController::workerFinishedBlock(uint32 id, Misc::WGTBlock* block)
{
}

void ComboDLINTController::workerFinishedSuperBlock(uint32 id)
{
}

void ComboDLINTController::pokeThread()
{
}

void ComboDLINTController::run()
{
	scanForWork();

	if (m_vSuperBlockList.size() > 0)
	{
		m_pUPThread->start();

		startDLWorkers();

		while (!isStopped() && waitingForDownload())
		{
			doPause();
			saveBuffers();

			if (m_iRunningWorkers == 0 && !isQuedBlocks())
				break;

			if (!isQuedBlocks() && m_iRunningWorkers > 0 && !isStopped())
			{
				if (!m_ProvManager.hasValidAgents())
					break;

				m_WaitCondition.wait(5);
			}
		}

		m_pUPThread->stop();
	}



	doInstall();
}

void ComboDLINTController::onStop()
{
	m_bDoingStop = true;

	BaseMCFThread::onStop();

	//stop workers



	m_bDoingStop = false;
}
