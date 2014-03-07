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

#ifndef DESURA_COMBODLINTCONTROLLER_H
#define DESURA_COMBODLINTCONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseMCFThread.h"
#include "WGTControllerI.h"
#include "ProviderManager.h"

#include <atomic>

namespace MCFCore
{

	namespace Thread
	{
		class WGTWorkerInfo;


		//! Web Get Thread controller. Downloads mcf from mcf servers and installs the content to a folder
		//!
		class ComboDLINTController : public MCFCore::Thread::BaseMCFThread, protected WGTControllerI
		{
		public:
			//! Constructor
			//!
			//! @param source Download provider list
			//! @param numWorkers Number of children to spawn to download the mcf
			//! @param caller Parent Mcf
			//! @param checkMcf Check the Mcf for downloaded chunks before starting
			//!
			ComboDLINTController(std::shared_ptr<MCFCore::Misc::DownloadProvidersI> pDownloadProviders, uint16 numWorkers, MCFCore::MCF* caller, const char* szInstallPath);
			~ComboDLINTController();

			//! Provider event
			//!
			Event<MCFCore::Misc::DP_s> onProviderEvent;

		protected:
			Misc::WGTSuperBlock* newTask(uint32 id, uint32 &status) override;
			uint32 getStatus(uint32 id) override;
			void reportError(uint32 id, gcException &e) override;
			void reportProgress(uint32 id, uint64 ammount) override;
			void reportNegProgress(uint32 id, uint64 ammount) override;
			void workerFinishedBlock(uint32 id, Misc::WGTBlock* block) override;
			void workerFinishedSuperBlock(uint32 id) override;
			void pokeThread() override;


			void run() override;
			void onStop() override;

		private:
			const gcString m_szInstallPath;

			MCFCore::Misc::ProviderManager m_ProvManager;

			std::deque<Misc::WGTSuperBlock*> m_vSuperBlockList;
			atomic<bool> m_bDoingStop = false;

			::Thread::WaitCondition m_WaitCondition;
		};

	}
}

#endif