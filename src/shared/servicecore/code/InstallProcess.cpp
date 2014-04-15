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
#include "InstallProcess.h"
#include "McfInit.h"
#include "InstallScriptRunTime.h"

InstallProcess::InstallProcess(const char* mcfpath, const char* inspath, const char* installScript, uint8 workercount, bool delfiles, bool makeWriteable) : Thread::BaseThread("InstallProcess Thread")
{
	m_szIPath = gcString(inspath);
	m_szMCFPath = gcString(mcfpath);

	m_uiWorkerCount = workercount;
	m_bDelFiles = delfiles;

	m_pMcfHandle = nullptr;
	m_bHashMissMatch = false;
	m_bMakeWriteable = makeWriteable;

	m_uiLastProg = 0;
	m_szInstallScript = installScript;

	m_bHasHadError = false;
}


InstallProcess::~InstallProcess()
{
	stop();

	if (m_pMcfHandle)
	{
		m_pMcfHandle->stop();
		mcfDelFactory(m_pMcfHandle);
	}
}

void InstallProcess::run()
{
	gcTrace("");

	if (m_szIPath == "" || m_szMCFPath == "")
	{
		gcException errNullPath(ERR_BADPATH, gcString("One of the paths for install was nullptr. [I:{0} M:{1}]", m_szIPath, m_szMCFPath));
		onErrorEvent(errNullPath);
		onFinishEvent();
		return;
	}

	UTIL::FS::recMakeFolder(m_szIPath.c_str());

#ifdef WIN32
	if (m_bMakeWriteable)
	{
		try
		{
			UTIL::WIN::changeFolderPermissions(gcWString(m_szIPath).c_str());
		}
		catch (gcException &e)
		{
			Warning("Failed to set path as writable for install: {0}\n", e);
		}
	}
#endif

	MCFCore::MCFI *mcfHandle = mcfFactory();
	m_pMcfHandle = mcfHandle;
	mcfHandle->setFile(m_szMCFPath.c_str());

	if (m_uiWorkerCount != 0)
		mcfHandle->setWorkerCount(m_uiWorkerCount);

	mcfHandle->getErrorEvent() += delegate(this, &InstallProcess::onError);
	mcfHandle->getProgEvent()  += delegate(this, &InstallProcess::onProgress);

	InstallScriptRunTime isrt(m_szInstallScript.c_str(), m_szIPath.c_str());

	try
	{
		mcfHandle->parseMCF();
		isrt.run("PreInstall");

		if (m_bDelFiles)
			UTIL::FS::delFolder(m_szIPath.c_str());

		UTIL::FS::makeFolder(m_szIPath.c_str());
		mcfHandle->saveFiles(m_szIPath.c_str());

		isrt.run("PostInstall");
	}
	catch (gcException &except)
	{
		onErrorEvent(except);
	}

	m_pMcfHandle=nullptr;
	mcfDelFactory(mcfHandle);

	if (m_bHashMissMatch)
	{
		gcException e(ERR_HASHMISSMATCH);
		onErrorEvent(e);
	}
	
	onCompleteEvent();
	onFinishEvent();
}

void InstallProcess::onStop()
{
	if (m_pMcfHandle)
		m_pMcfHandle->stop();
}

void InstallProcess::onPause()
{
	if (m_pMcfHandle)
		m_pMcfHandle->pause();
}

void InstallProcess::onUnpause()
{
	if (m_pMcfHandle)
		m_pMcfHandle->unpause();
}


void InstallProcess::onError(gcException& e)
{
	gcTrace("E: {0}", e);

	if (e.getErrId() == ERR_HASHMISSMATCH)
	{
		m_bHashMissMatch = true;
	}
	else
	{
		if (!m_bHasHadError)
			onErrorEvent(e);

		m_bHasHadError = true;
	}
}

void InstallProcess::onProgress(MCFCore::Misc::ProgressInfo& p)
{
	uint64 val = p.toInt64();

	if (val == m_uiLastProg)
		return;

	onProgressEvent(val);
	m_uiLastProg = val;
}