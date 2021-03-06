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
#include "gcJSUploads.h"

#include "Managers.h"
#include "usercore/UploadInfoThreadI.h"

#include "MainApp.h"
#include "usercore/ItemManagerI.h"

#ifdef NIX
#include "util/UtilLinux.h"
#endif

#ifdef WIN32
  #include <Shellapi.h>
#endif

template <>
void FromJSObject<UserCore::Misc::UploadInfoThreadI*>(UserCore::Misc::UploadInfoThreadI* &upload, JSObjHandle& arg)
{
	if (arg->isObject())
		upload = arg->getUserObject<UserCore::Misc::UploadInfoThreadI>();
	else
		upload = nullptr;
}

REGISTER_JSEXTENDER(DesuraJSUploadInfo);

DesuraJSUploadInfo::DesuraJSUploadInfo() : DesuraJSBase("uploads", "native_binding_uploadinfo.js")
{
	REG_SIMPLE_JS_FUNCTION( getUploadFromId, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_FUNCTION( getUploadId, DesuraJSUploadInfo );

	REG_SIMPLE_JS_OBJ_FUNCTION( shouldDeleteMcf, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_VOIDFUNCTION( setDeleteMcf, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_FUNCTION( getItem, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_FUNCTION( getItemId, DesuraJSUploadInfo );
	
	REG_SIMPLE_JS_OBJ_VOIDFUNCTION( remove, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_VOIDFUNCTION( pause, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_VOIDFUNCTION( unpause, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_FUNCTION( isPaused, DesuraJSUploadInfo );

	REG_SIMPLE_JS_OBJ_FUNCTION( isComplete, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_FUNCTION( isValid, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_FUNCTION( hasError, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_FUNCTION( hasStarted, DesuraJSUploadInfo );

	REG_SIMPLE_JS_OBJ_VOIDFUNCTION( exploreMcf, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_FUNCTION( getProgress, DesuraJSUploadInfo );

	REG_SIMPLE_JS_OBJ_VOIDFUNCTION( cancel, DesuraJSUploadInfo );
	REG_SIMPLE_JS_OBJ_FUNCTION( getFileName, DesuraJSUploadInfo );

	m_uiIsValidHash = UTIL::MISC::RSHash_CSTR("isValid");
	m_uiGetUploadFromIdHash = UTIL::MISC::RSHash_CSTR("getUploadFromId");
}



bool DesuraJSUploadInfo::preExecuteValidation(const char* function, uint32 functionHash, JSObjHandle object, size_t argc, JSObjHandle* argv)
{
	if (argc == 0)
		return false;

	if (m_uiIsValidHash == functionHash || m_uiGetUploadFromIdHash == functionHash)
		return true;

	UserCore::Misc::UploadInfoThreadI* upload;
	FromJSObject(upload, argv[0]);
	return isValid(upload);
}


bool DesuraJSUploadInfo::isValid(UserCore::Misc::UploadInfoThreadI* upload)
{
	return upload?true:false;
}

void* DesuraJSUploadInfo::getUploadFromId(int32 uid)
{
	if (!GetUploadMng())
		return nullptr;

	void* ret = GetUploadMng()->findItem((uint32)uid);
	return ret;
}

bool DesuraJSUploadInfo::isComplete(UserCore::Misc::UploadInfoThreadI* upload)
{
	return upload->isCompleted();
}

bool DesuraJSUploadInfo::hasError(UserCore::Misc::UploadInfoThreadI* upload)
{
	return upload->hasError();
}

bool DesuraJSUploadInfo::hasStarted(UserCore::Misc::UploadInfoThreadI* upload)
{
	return upload->hasStarted();
}

bool DesuraJSUploadInfo::isPaused(UserCore::Misc::UploadInfoThreadI* upload)
{
	return upload->isPaused();
}

bool DesuraJSUploadInfo::shouldDeleteMcf(UserCore::Misc::UploadInfoThreadI* upload)
{
	return upload->shouldDelMcf();
}

void DesuraJSUploadInfo::setDeleteMcf(UserCore::Misc::UploadInfoThreadI* upload, bool del)
{
	upload->setDelMcf(del);
}



void DesuraJSUploadInfo::remove(UserCore::Misc::UploadInfoThreadI* upload)
{
	GetUploadMng()->removeUpload(upload->getKey(), true);
}

void DesuraJSUploadInfo::pause(UserCore::Misc::UploadInfoThreadI* upload)
{
	upload->pause();
}

void DesuraJSUploadInfo::unpause(UserCore::Misc::UploadInfoThreadI* upload)
{
	upload->unpause();
}

void DesuraJSUploadInfo::cancel(UserCore::Misc::UploadInfoThreadI* upload)
{
	upload->stop();
	GetUploadMng()->removeUpload(upload->getKey());
}

void DesuraJSUploadInfo::exploreMcf(UserCore::Misc::UploadInfoThreadI* upload)
{
	const char* file = upload->getFile();

	if (file)
	{
		UTIL::FS::Path path(file, "", true);
#ifdef WIN32
		ShellExecuteA(nullptr, "explore" , path.getFolderPath().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
		UTIL::LIN::launchFolder(path.getFolderPath().c_str());
#endif
	}
}



int32 DesuraJSUploadInfo::getProgress(UserCore::Misc::UploadInfoThreadI* upload)
{
	return upload->getProgress();
}


void* DesuraJSUploadInfo::getItem(UserCore::Misc::UploadInfoThreadI* upload)
{
	if (!GetUserCore() || !GetUserCore()->getItemManager())
		return nullptr;

	return GetUserCore()->getItemManager()->findItemInfo(upload->getItemId());
}

gcString DesuraJSUploadInfo::getFileName(UserCore::Misc::UploadInfoThreadI* upload)
{
	UTIL::FS::Path path = UTIL::FS::PathWithFile(upload->getFile());
	return path.getFile().getFile();
}

gcString DesuraJSUploadInfo::getUploadId(UserCore::Misc::UploadInfoThreadI* upload)
{
	return gcString("{0}", upload->getHash());
}

gcString DesuraJSUploadInfo::getItemId(UserCore::Misc::UploadInfoThreadI* upload)
{
	return upload->getItemId().toString();
}
