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

#include "UtilBootloader.h"

#include <process.h>
#include <Psapi.h>

#include <vector>
#include <map>
#include <string>

#include <commctrl.h>

#include "SharedObjectLoader.h"
#include "third_party/PEImage.h"

HRESULT CreateProcessWithExplorerIL(LPWSTR szProcessName, LPWSTR szCmdLine, LPWSTR szWorkingDir);

typedef BOOL (WINAPI* WaitForDebuggerFunc)();
typedef BOOL (WINAPI* SetDllDirectoryFunc)(LPCTSTR lpPathName);


namespace BootLoaderUtil
{

bool IsExeRunning(char* pName)
{
	unsigned long aProcesses[1024], cbNeeded, cProcesses;
	if(!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		return false;

	unsigned long curPID = GetCurrentProcessId();

	cProcesses = cbNeeded / sizeof(unsigned long);
	for (unsigned int i = 0; i < cProcesses; i++)
	{
		if(aProcesses[i] == 0)
			continue;

		if (aProcesses[i] == curPID)
			continue;

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, aProcesses[i]);

		if (!hProcess)
			continue;

		char buffer[50] = {0};
		GetModuleBaseName(hProcess, 0, buffer, 50);
		CloseHandle(hProcess);
		
		if(strcmp(pName, buffer)==0)
			return true;
	}
	
	return false;
}

void GetLastFolder(char* dest, size_t destSize, const char* src)
{
	size_t len = strlen(src)+1;
	size_t y =0;

	for (size_t x=len-1; x>0; x--)
	{
		if (src[x] == '\\' || src[x] == '/')
		{
			strncpy_s(dest, destSize, src+x+1, y);
			dest[y] = '\0';
			break;
		}
		y++;
	}
}


bool RestartAsNormal(const char* args)
{
	wchar_t name[255];
	GetModuleFileNameW(nullptr, name, 255);

	wchar_t szWorkingDir[255];
	GetModuleFileNameW(nullptr, szWorkingDir, 255);

	size_t exePathLen = Safe::wcslen(szWorkingDir, 255);
	for (size_t x=exePathLen; x>0; x--)
	{
		if (szWorkingDir[x] == L'\\')
			break;
		else
			szWorkingDir[x] = L'\0';
	}

	wchar_t wArgs[255];
	wArgs[0] = 0;

	if (args)
	{
		for (size_t x=0; x<strlen(args); x++)
			wArgs[x] = (wchar_t)args[x];

		wArgs[strlen(args)] = 0;
	}

	HRESULT res = CreateProcessWithExplorerIL(name, wArgs, szWorkingDir);
	return SUCCEEDED(res);
}

bool RestartAsAdmin(const char* args)
{
	char exePath[255];
	GetModuleFileName(nullptr, exePath, 255);

	size_t exePathLen = strlen(exePath);
	for (size_t x=exePathLen; x>0; x--)
	{
		if (exePath[x] == '\\')
			break;
		else
			exePath[x] = '\0';
	}

	char name[255];
	GetModuleFileName(nullptr, name, 255);

	char restartArgs[255];
	
	if (args)
		strncpy_s(restartArgs, 255, args, 255);
	else
		restartArgs[0] =0;

	INT_PTR r = (INT_PTR)ShellExecute(nullptr, "runas", name, restartArgs, exePath, SW_SHOW);
	return !(r < 32);
}

bool Restart(const char* args, bool wait)
{
	char exePath[255];
	GetModuleFileName(nullptr, exePath, 255);

	size_t exePathLen = strlen(exePath);
	for (size_t x=exePathLen; x>0; x--)
	{
		if (exePath[x] == '\\')
			break;
		else
			exePath[x] = '\0';
	}

	char name[255];
	GetModuleFileName(nullptr, name, 255);

	PROCESS_INFORMATION ProcInfo = {0};
	STARTUPINFO StartupInfo = {0};

	char launchArg[255];

	if (args)
		_snprintf_s(launchArg, 255, _TRUNCATE, "\"%s\"%s %s", name, wait?" -wait":"", args);
	else
		_snprintf_s(launchArg, 255, _TRUNCATE, "\"%s\"%s", name, wait?" -wait":"");

	BOOL res = CreateProcess(nullptr, launchArg, nullptr, nullptr, false, NORMAL_PRIORITY_CLASS, nullptr, exePath, &StartupInfo, &ProcInfo );

	CloseHandle(ProcInfo.hProcess);
	CloseHandle(ProcInfo.hThread);

	return res?true:false;
}

bool StartProcess(const char* name, const char* args)
{
	char exePath[255];
	GetModuleFileName(nullptr, exePath, 255);

	size_t exePathLen = strlen(exePath);
	for (size_t x=exePathLen; x>0; x--)
	{
		if (exePath[x] == '\\')
			break;
		else
			exePath[x] = '\0';
	}

	PROCESS_INFORMATION ProcInfo = {0};
	STARTUPINFO StartupInfo = {0};

	char launchArg[255];

	if (args)
		_snprintf_s(launchArg, 255, _TRUNCATE, "\"%s\\%s\" -wait %s", exePath, name, args);
	else
		_snprintf_s(launchArg, 255, _TRUNCATE, "\"%s\\%s\" -wait", exePath, name);

	BOOL res = CreateProcess(nullptr, launchArg, nullptr, nullptr, false, NORMAL_PRIORITY_CLASS, nullptr, exePath, &StartupInfo, &ProcInfo );

	CloseHandle(ProcInfo.hProcess);
	CloseHandle(ProcInfo.hThread);

	return res?true:false;
}


void WaitForDebugger()
{
	HMODULE kernel32_dll = GetModuleHandle("kernel32.dll");
	if (kernel32_dll != nullptr)
	{
		WaitForDebuggerFunc waitfor_debugger = (WaitForDebuggerFunc)GetProcAddress(kernel32_dll, "IsDebuggerPresent");
	
		if (waitfor_debugger != nullptr) 
		{
			while( !waitfor_debugger() )
				Sleep( 500 );
		}
	}
}

void InitCommonControls()
{
	INITCOMMONCONTROLSEX InitCtrlEx;

	InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCtrlEx.dwICC  = ICC_PROGRESS_CLASS|ICC_STANDARD_CLASSES;
	InitCommonControlsEx(&InitCtrlEx);
}

bool SetDllDir(const char* dir)
{
	SharedObjectLoader sol;

	if (sol.load("kernel32.dll"))
	{
		SetDllDirectoryFunc set_dll_directory = sol.getFunction<SetDllDirectoryFunc>("SetDllDirectoryA");
	
		if (set_dll_directory && set_dll_directory(dir)) 
			return true;
	}

	return false;
}


bool CheckForOtherInstances(HINSTANCE hinstant)
{
	char buffer[MAX_PATH+1];
	GetModuleFileName(hinstant, buffer, sizeof(buffer));

	char exe[MAX_PATH];
	BootLoaderUtil::GetLastFolder(exe, MAX_PATH, buffer);

	return BootLoaderUtil::IsExeRunning(exe);
}


void WaitForOtherInstance(char* name)
{
	while (BootLoaderUtil::IsExeRunning(name))
	{
		Sleep(500);
	}
}


void WaitForOtherInstance(HINSTANCE hinstant)
{
	while (CheckForOtherInstances(hinstant))
	{
		Sleep(500);
	}
}


void SetCurrentDir()
{
	char exePath[255];
	GetModuleFileName(nullptr, exePath, 255);

	size_t exePathLen = strlen(exePath);
	for (size_t x=exePathLen; x>0; x--)
	{
		if (exePath[x] == '\\')
			break;
		else
			exePath[x] = '\0';
	}

	SetCurrentDirectory(exePath);
}

#define uint8 unsigned char

void PreReadImage(const char* file_path)
{
	unsigned int win = GetOSId();
	const DWORD actual_step_size = static_cast<DWORD>(1024*1024);

	if (win > WINDOWS_VISTA) 
	{
		// Vista+ branch. On these OSes, the forced reads through the DLL actually
		// slows warm starts. The solution is to sequentially read file contents
		// with an optional cap on total amount to read.
		HANDLE file = CreateFile(file_path,GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

		if (file != INVALID_HANDLE_VALUE)
			return;

		// Default to 1MB sequential reads.
		LPVOID buffer = ::VirtualAlloc(nullptr, actual_step_size, MEM_COMMIT, PAGE_READWRITE);

		if (buffer == nullptr)
		{
			CloseHandle(file);
			return;
		}

		DWORD len;
		size_t total_read = 0;

		while (::ReadFile(file, buffer, actual_step_size, &len, nullptr) && len > 0) 
		{
			total_read += static_cast<size_t>(len);
		}

		::VirtualFree(buffer, 0, MEM_RELEASE);
		CloseHandle(file);
	} 
	else 
	{
		// WinXP branch. Here, reading the DLL from disk doesn't do
		// what we want so instead we pull the pages into memory by loading
		// the DLL and touching pages at a stride.
		HMODULE dll_module = ::LoadLibraryExA(file_path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH|DONT_RESOLVE_DLL_REFERENCES);

		if (!dll_module)
			return;

		base::win::PEImage pe_image(dll_module);
	
		PIMAGE_NT_HEADERS nt_headers = pe_image.GetNTHeaders();
		size_t actual_size_to_read = nt_headers->OptionalHeader.SizeOfImage;
		volatile uint8* touch = reinterpret_cast<uint8*>(dll_module);
	
		size_t offset = 0;
		while (offset < actual_size_to_read) 
		{
			uint8 unused = *(touch + offset);
			offset += actual_step_size;
		}
	
		FreeLibrary(dll_module);
	}
}

}