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

#ifndef DESURA_ITEMINFO_H
#define DESURA_ITEMINFO_H
#ifdef _WIN32
#pragma once
#endif

#include "usercore/ItemInfoI.h"
#include "managers/WildcardManager.h"
#include "BranchInfo.h"

namespace XML
{
	class gcXMLElement;
}

namespace sqlite3x
{
	class sqlite3_connection;
}

namespace UserCore
{
	class UserI;

	namespace Item
	{
		class BranchInfo;
		class BranchInstallInfo;

		class BranchItemInfoI
		{
		public:
			 virtual DesuraId getId()=0;
			 virtual uint32 getStatus()=0;
		};

		class ItemInfoInternalI
		{
		public:
			virtual void setPercent(uint8 percent)=0;
			virtual MCFBranch getBestBranch(MCFBranch branch)=0;
			virtual void resetInstalledMcf()=0;
			virtual void overideInstalledBuild(MCFBuild build)=0;
			virtual BranchInstallInfo* getBranchOrCurrent(MCFBranch branch)=0;
		};

		class ItemInfo : public ItemInfoI, public BranchItemInfoI, public ItemInfoInternalI
		{
		public:
			//! Constructor
			//!
			//! @param id Item internal id
			//!
			ItemInfo(UserCore::UserI *user, DesuraId id);

			//! Constructor
			//!
			//! @param id Item internal id
			//! @param parid Parent internal id
			//!
			ItemInfo(UserCore::UserI *user, DesuraId id, DesuraId parid);
			~ItemInfo();

			//inherited methods
			 void updated() override;
			 void addToAccount() override;
			 void removeFromAccount() override;

			 DesuraId getParentId() override;
			 DesuraId getId() override;
			 DesuraId getInstalledModId(MCFBranch branch = MCFBranch()) override;

			 uint32 getChangedFlags() override;
			 uint32 getStatus() override;

			 uint8 getPercent() override;
			 uint8 getPermissions() override;
			 uint8 getOptions() override;

			 bool isLaunchable() override;
			 bool isUpdating() override;
			 bool isInstalled() override;
			 bool isDownloadable() override;
			 bool isComplex() override;
			 bool isParentToComplex() override;
			 bool isFirstLaunch() override;

			 bool hasAcceptedEula() override;
			 bool compare(const char* filter) override;

			 void addSFlag(uint32 status) override;
			 void addPFlag(uint8 permission) override;
			 void addOFlag(uint8 option) override;

			 void delSFlag(uint32 status) override;
			 void delPFlag(uint8 permission) override;
			 void delOFlag(uint8 option) override;

			 const char* getRating() override;
			 const char* getDev() override;
			 const char* getName() override;
			 const char* getShortName() override;
			 const char* getPath(MCFBranch branch = MCFBranch()) override;
			 const char* getInsPrimary(MCFBranch branch = MCFBranch()) override;
			 const char* getIcon() override;
			 const char* getLogo() override;
			 const char* getIconUrl() override;
			 const char* getLogoUrl() override;
			 const char* getDesc() override;
			 const char* getTheme() override;
			 const char* getGenre() override;
			 const char* getProfile() override;
			 const char* getDevProfile() override;

			 const char* getPublisher() override;
			 const char* getPublisherProfile() override;

			 const char* getEulaUrl() override;
			 const char* getInstallScriptPath() override;

			 Event<ItemInfoI::ItemInfo_s>* getInfoChangeEvent() override;


			void overrideMcfBuild(MCFBuild build, MCFBranch branch = MCFBranch());
			 uint64 getInstallSize(MCFBranch branch = MCFBranch()) override;
			 uint64 getDownloadSize(MCFBranch branch = MCFBranch()) override;
			 MCFBuild getLastInstalledBuild(MCFBranch branch = MCFBranch()) override;
			 MCFBuild getInstalledBuild(MCFBranch branch = MCFBranch()) override;
			 MCFBuild getNextUpdateBuild(MCFBranch branch = MCFBranch()) override;
			 MCFBranch getInstalledBranch() override;
			 MCFBranch getLastInstalledBranch() override;
			 const char* getInstalledVersion(MCFBranch branch = MCFBranch()) override;


			 uint32 getBranchCount() override;
			 BranchInfoI* getBranch(uint32 index) override;
			 BranchInfoI* getCurrentBranch() override;
			 BranchInfoI* getBranchById(uint32 id) override;

			 void acceptEula() override;


			 uint32 getExeCount(bool setActive, MCFBranch branch = MCFBranch()) override;
			 void getExeList(std::vector<UserCore::Item::Misc::ExeInfoI*> &list, MCFBranch branch = MCFBranch()) override;
			 UserCore::Item::Misc::ExeInfoI* getActiveExe(MCFBranch branch = MCFBranch()) override;
			 void setActiveExe(const char* name, MCFBranch branch = MCFBranch()) override;

			 bool isFavorite() override;
			 void setFavorite(bool fav) override;

			//! Removes this item from the db
			//!
			//! @param db Sqlite db connection
			//!
			void deleteFromDb(sqlite3x::sqlite3_connection* db);

			//! Save regular changed vars to db
			//!
			//! @param db Sqlite db connection
			//!
			void saveDb(sqlite3x::sqlite3_connection* db);

			//! Save all vars to db
			//!
			//! @param db Sqlite db connection
			//!
			void saveDbFull(sqlite3x::sqlite3_connection* db);

			//! Load vars from db
			//!
			//! @param db Sqlite db connection
			//!
			void loadDb(sqlite3x::sqlite3_connection* db);


			//! Load data for this item from xml
			//!
			//! @param xmlNode Xml to get data from
			//! @param statusOveride New status flags to add when load is complete
			//! @param pWildCard Wildcard manager to resolve wildcards from
			//!
			void loadXmlData(uint32 platform, const XML::gcXMLElement &xmlNode, uint16 statusOveride, WildcardManager* pWildCard=nullptr, bool reset = false);


			//! hash for base manager
			//!
			//! @return Item hash
			//!
			uint64 getHash();

			//! Paused the item information update event
			//!
			void pauseCallBack();

			//! Resumes the item information update event
			//!
			void resumeCallBack();

			//! Is the item information update event active
			//!
			//! @return True for active, false if not
			//!
			bool isCallBackActive();


			//! Sets the Item progress percent
			//!
			//! @param percent Item percent
			//!
			void setPercent(uint8 percent) override;


			//! Sets the item name
			//!
			//! @param name Name
			//!
			void setName(const char* name);

			//! Sets the item icon
			//!
			//! @param icon Icon path
			//!
			void setIcon(const char* icon);

			//! Sets the item logo
			//!
			//! @param logo Logo path
			//!
			void setLogo(const char* logo);


			//! Sets the item icon url
			//!
			//! @param icon Icon url
			//! @param hash Icon hash
			//!
			void setIconUrl(const char* icon);

			//! Sets the item logo url
			//!
			//! @param logo Logo url
			//! @param hash Logo hash
			//!
			void setLogoUrl(const char* logo);


			void processUpdateXml(const XML::gcXMLElement &node);


			bool setInstalledMcf(MCFBranch branch, MCFBuild build);
			void overideInstalledBuild(MCFBuild build) override;

			void resetInstalledMcf() override;
			void overideFavorite(bool fav);

			//! Sets the id of the installed mod for this item. Use item manager instead to set this!!!!!
			//!
			//! @param id Mod id
			//!
			void setInstalledModId(DesuraId id, MCFBranch branch = MCFBranch());

			//! Overrides the parent id
			//!
			//! @param id Parent id
			//!
			void setParentId(DesuraId id);


			bool wasOnAccount();

			void migrateStandalone(MCFBranch branch, MCFBuild build);


			void setLinkInfo(const char* exe, const char* args);

			//! If given a global branch it will return the best branch for that global.
			//! If given an invalid branch it will select the best branch avaliable
			//! If given a valid non global branch it will return the same branch
			//!
			MCFBranch getBestBranch(MCFBranch branch) override;

			//! Given a list of branches it will select the best avliable
			//! If it cant work out best branch it will return 0
			//!
			MCFBranch selectBestBranch(const std::vector<BranchInfo*> &list);

			ItemInfoInternalI* getInternal() override
			{
				return this;
			}

			bool isDeleted();

		protected:
			//! Event handler for item information changed. Triggers when this item information gets updated
			//!
			Event<ItemInfoI::ItemInfo_s> onInfoChangeEvent;

			//! If the info changed event is active it gets called
			//!
			void onInfoChange();

			//! Triggers the info changed event
			//!
			void triggerCallBack();

			//! Gets the usercore handle
			//!
			//! @return UserCore
			//!
			UserCore::UserI* getUserCore();


			void broughtCheck();

			void processInfo(const XML::gcXMLElement &xmlEl);
			void processSettings(uint32 platform, const XML::gcXMLElement &setNode, WildcardManager* pWildCard, bool reset);

			void launchExeHack();

			void onBranchInfoChanged();
			bool shouldSaveDb(sqlite3x::sqlite3_connection* db);

			void loadBranchXmlData(const XML::gcXMLElement &branch);
	
			BranchInfo* getCurrentBranchFull();
			BranchInstallInfo* getBranchOrCurrent(MCFBranch branch) override;


		private:
			bool m_bPauseCallBack = false;
			bool m_bWasOnAccount = false;

			DesuraId m_iId;
			DesuraId m_iParentId;
	

			uint32 m_iChangedFlags = 0;
			uint32 m_iStatus = ItemInfoI::STATUS_UNKNOWN;

			uint32 m_INBranchIndex = -1;
			MCFBranch m_INBranch;
			MCFBranch m_LastBranch;

			uint8 m_iPermissions = 0;
			uint8 m_iOptions = ItemInfoI::OPTION_AUTOUPDATE;
			uint8 m_iPercent = 0;

			gcString m_szRating;	//rating
			gcString m_szDesc;		//short desc
			gcString m_szDev;		//developers name
			gcString m_szDevProfile;//developers profile
			gcString m_szPublisher;
			gcString m_szPublisherProfile;
			gcString m_szName;		//full name
			gcString m_szShortName;	//short name

			gcString m_szIcon;		//icon path or url
			gcString m_szLogo;		//logo path or url
			gcString m_szIconUrl;
			gcString m_szLogoUrl;

			gcString m_szTheme;		//items theme
			gcString m_szGenre;		//items genre
			gcString m_szProfile;	//url to profile
			gcString m_szEULAUrl;	//eula

			std::vector<BranchInfo*> m_vBranchList;
			std::map<uint32, BranchInstallInfo*> m_mBranchInstallInfo;

			UserCore::UserI *m_pUserCore; 
		};



		inline DesuraId ItemInfo::getParentId()
		{
			return m_iParentId;
		}

		inline DesuraId ItemInfo::getId()
		{
			return m_iId;
		}

		inline uint32 ItemInfo::getChangedFlags()
		{
			return m_iChangedFlags;
		}

		inline uint32 ItemInfo::getStatus()
		{
			return m_iStatus;
		}


		inline uint8 ItemInfo::getPercent()
		{
			return m_iPercent;
		}

		inline uint8 ItemInfo::getPermissions()
		{
			return m_iPermissions;
		}

		inline uint8 ItemInfo::getOptions()
		{
			return m_iOptions;
		}


		inline bool ItemInfo::isLaunchable()
		{
			return (m_iStatus & (UserCore::Item::ItemInfoI::STATUS_INSTALLED|UserCore::Item::ItemInfoI::STATUS_READY))?true:false;
		}

		inline bool ItemInfo::isInstalled()
		{
			return (m_iStatus & UserCore::Item::ItemInfoI::STATUS_INSTALLED)?true:false;
		}

		inline bool ItemInfo::isDownloadable()
		{
			return !HasAnyFlags(getStatus(), UserCore::Item::ItemInfoI::STATUS_NONDOWNLOADABLE|UserCore::Item::ItemInfoI::STATUS_LINK);
		}

		inline bool ItemInfo::isComplex()
		{
			return HasAllFlags(getStatus(), UserCore::Item::ItemInfoI::STATUS_INSTALLCOMPLEX);
		}

		inline bool ItemInfo::isParentToComplex()
		{
			return !getParentId().isOk() && getInstalledModId().isOk();
		}

		inline bool ItemInfo::isUpdating()
		{
			return (isInstalled() && HasAllFlags(getStatus(), UserCore::Item::ItemInfoI::STATUS_UPDATEAVAL));
		}

		inline bool ItemInfo::isFirstLaunch()
		{
			return HasAllFlags(getStatus(), UserCore::Item::ItemInfoI::STATUS_LAUNCHED) == false;
		}

		inline const char* ItemInfo::getRating()	
		{
			return m_szRating.c_str();
		}

		inline const char* ItemInfo::getDev()		
		{
			return m_szDev.c_str();
		}

		inline const char* ItemInfo::getName()		
		{
			return m_szName.c_str();
		}

		inline const char* ItemInfo::getShortName()	
		{
			return m_szShortName.c_str();
		}

		inline const char* ItemInfo::getIcon()		
		{
			return m_szIcon.c_str();
		}

		inline const char* ItemInfo::getLogo()		
		{
			return m_szLogo.c_str();
		}

		inline const char* ItemInfo::getIconUrl()	
		{
			return m_szIconUrl.c_str();
		}

		inline const char* ItemInfo::getLogoUrl()	
		{
			return m_szLogoUrl.c_str();
		}

		inline const char* ItemInfo::getDesc()		
		{
			return m_szDesc.c_str();
		}

		inline const char* ItemInfo::getTheme()		
		{
			return m_szTheme.c_str();
		}

		inline const char* ItemInfo::getGenre()		
		{
			return m_szGenre.c_str();
		}

		inline const char* ItemInfo::getProfile()	
		{
			return m_szProfile.c_str();
		}

		inline const char* ItemInfo::getDevProfile()
		{
			return m_szDevProfile.c_str();
		}

		inline const char* ItemInfo::getPublisher()
		{
			return m_szPublisher.c_str();
		}

		inline const char* ItemInfo::getPublisherProfile()
		{
			return m_szPublisherProfile.c_str();
		}

		inline const char* ItemInfo::getInstallScriptPath()
		{
			if (!getCurrentBranch())
				return nullptr;

			return getCurrentBranch()->getInstallScriptPath();
		}

		inline Event<ItemInfoI::ItemInfo_s>* ItemInfo::getInfoChangeEvent()
		{
			return &onInfoChangeEvent;
		}

		/////////////////////////////////////////////

		inline uint64 ItemInfo::getHash()
		{
			return getId().toInt64();
		}

		inline void ItemInfo::pauseCallBack()
		{
			m_bPauseCallBack = true;
		}

		inline void ItemInfo::resumeCallBack()
		{
			m_bPauseCallBack = false;
		}

		inline bool ItemInfo::isCallBackActive()
		{
			return m_bPauseCallBack;
		}

		inline void ItemInfo::setName(const char* name)		
		{
			m_szName = gcString(name);
		}

		inline UserCore::UserI* ItemInfo::getUserCore()
		{
			return m_pUserCore;
		}

		inline uint32 ItemInfo::getBranchCount()
		{
			return m_vBranchList.size();
		}

		inline MCFBranch ItemInfo::getInstalledBranch()
		{
			return m_INBranch;
		}

		inline MCFBranch ItemInfo::getLastInstalledBranch()
		{
			return m_LastBranch;
		}

		inline bool ItemInfo::wasOnAccount()
		{
			return m_bWasOnAccount;
		}

		inline bool ItemInfo::isDeleted()
		{
			return HasAllFlags(getStatus(), UserCore::Item::ItemInfoI::STATUS_DELETED);
		}
	}
}

#endif //DESURA_ItemInfo_H
