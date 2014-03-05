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

#ifndef DESURA_COMBODOWNLOADINSTALLTASK_H
#define DESURA_COMBODOWNLOADINSTALLTASK_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseItemTask.h"

namespace UnitTest
{
	class ComboDownloadInstallTaskFixture;
}

namespace UserCore
{
	namespace ItemTask
	{
		class ComboDownloadInstallTask : public UserCore::ItemTask::BaseItemTask
		{
		public:
			ComboDownloadInstallTask(UserCore::Item::ItemHandleI* handle, MCFBranch branch, MCFBuild build = MCFBuild());
			virtual ~ComboDownloadInstallTask();

		protected:
			void doRun() override;

			void onStop() override;
			void onPause() override;
			void onUnpause() override;
			void cancel() override;


			void onProgress(MCFCore::Misc::ProgressInfo& p);
			void updateStatusFlags();
			void validateHeader(MCFBuild &build, MCFBranch &branch);
			void onError(gcException &e);
			void onComplete();

		private:
			friend class UnitTest::ComboDownloadInstallTaskFixture;

			bool m_bUpdating = false;
			bool m_bUnAuthed = false;
			bool m_bInError = false;

			bool m_bDownloading = true;

			MCFBuild m_LastInsBuild;
		};
	}
}


#endif