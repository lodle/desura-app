{
	'includes': [
		 '../../../build_lin/common.gypi',
		 '../../common/service_pipe.gypi',
	],
	'targets': [
	{
		'target_name': 'usercore',
		'type': 'shared_library',
		'defines' : [
			'DESURA_CLIENT',
		],
		'dependencies' : [
			'<(shared_dir)/webcore/webcore.gyp:webcore',
			'<(shared_dir)/mcfcore/mcfcore.gyp:mcfcore',
			'<(static_dir)/managers/managers.gyp:*',
			'<(static_dir)/util_thread/util_thread.gyp:threads',
			'<(static_dir)/util_fs/util_fs.gyp:util_fs',
			'<(third_party_dir)/libs.gyp:curl',
			'<(static_dir)/util/util.gyp:*',
			'<(third_party_dir)/sqlite/sqlite.gyp:sqlite',
			'<(third_party_dir)/sqlite3x/sqlite3x.gyp:sqlite3x',
			'<(third_party_dir)/tinyxml/tinyxml.gyp:tinyxml',			
			'<(common_dir)/gcJSBase.gyp:gcJSBase',
		],
		'include_dirs': [
			'./code',
			'./RES',
		],
		'sources': [
			'code/BaseItemServiceTask.cpp',
			'code/BaseItemTask.cpp',
			'code/BDManager.cpp',
			'code/BranchInfo.cpp',
			'code/CDKeyManager.cpp',
			'code/CIPManager.cpp',
			'code/ComplexLaunchServiceTask.cpp',
			'code/CreateMCFThread.cpp',
			'code/DownloadTask.cpp',
			'code/DownloadToolItemTask.cpp',
			'code/DownloadToolTask.cpp',
			'code/DownloadUpdateTask.cpp',
			'code/GatherInfoTask.cpp',
			'code/GatherInfoThread.cpp',
			'code/GetItemListThread.cpp',
			'code/InstallCheckTask.cpp',
			'code/InstalledWizardThread.cpp',
			'code/InstallInfo.cpp',
			'code/InstallServiceTask.cpp',
			'code/InstallToolTask.cpp',
			'code/ItemHandle.cpp',
			'code/ItemHandleEvents.cpp',
			'code/ItemHandle_nix.cpp',
			'code/ItemInfo.cpp',
			'code/ItemManager.cpp',
			'code/ItemTaskGroup.cpp',
			'code/ItemThread.cpp',
			'code/Log.cpp',
			'code/McfManager.cpp',
			'code/ToolInfo.cpp',
			'code/ToolInstallThread.cpp',
			'code/ToolInstallThread_nix.cpp',			
			'code/ToolManager.cpp',
			'code/ToolManager_lin.cpp',
			'code/ToolTransaction.cpp',
			'code/UIBaseServiceTask.cpp',
			'code/UIBranchServiceTask.cpp',
			'code/UIComplexModServiceTask.cpp',
			'code/UIPatchServiceTask.cpp',
			'code/UIServiceTask.cpp',
			'code/UIUpdateServiceTask.cpp',
			'code/UpdateThread.cpp',
			'code/UpdateThread_Old.cpp',
			'code/UploadInfoThread.cpp',
			'code/UploadManager.cpp',
			'code/UploadPrepThread.cpp',
			'code/UploadResumeThread.cpp',
			'code/UploadThread.cpp',
			'code/User.cpp',
			'code/UserCoreMain.cpp',
			'code/UserIPCPipeClient.cpp',
			'code/UserTask.cpp',
			'code/UserTasks.cpp',
			'code/UserThreadManager.cpp',
			'code/User_Login.cpp',
			'code/User_nix.cpp',
			'code/ValidateTask.cpp',
			'code/VerifyServiceTask.cpp',
			'code/VSBaseTask.cpp',
			'code/VSCheckInstall.cpp',
			'code/VSCheckMcf.cpp',
			'code/VSCheckMcfDownload.cpp',
			'code/VSDownloadMissing.cpp',
			'code/VSInstallMissing.cpp',
		],
	}],
}
