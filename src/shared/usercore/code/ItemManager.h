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

#ifndef DESURA_ITEMMANAGER_H
#define DESURA_ITEMMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "usercore/ItemManagerI.h"
#include "BaseManager.h"
#include "usercore/ItemInfoI.h"
#include "UpdateThread_Old.h"

#include "ItemInfo.h"
#include "ItemHandle.h"

namespace XML
{
	class gcXMLElement;
}

class InfoMaps;

namespace UserCore
{

namespace Item
{
	class ItemTaskGroup;
}

class ItemManager : public ItemManagerI, public BaseManager<UserCore::Item::ItemHandle>
{
public:
	ItemManager(User* user);
	~ItemManager();

	typedef std::map<uint64, UserCore::Item::ItemHandle*>::iterator itemIterator;

	virtual void loadItems();
	virtual void saveItems();

	virtual bool isInstalled(DesuraId id);
	virtual void removeItem(DesuraId id);

	virtual void getCIP(DesuraId id, char** buff);
	virtual void retrieveItemInfo(DesuraId id, uint32 statusOveride = 0, WildcardManager* pWildCard = nullptr, MCFBranch branch = MCFBranch(), MCFBuild mcfBuild = MCFBuild(), bool reset = false);
	virtual void retrieveItemInfoAsync(DesuraId id, bool addToAccount);

	virtual uint32 getDevItemCount();

	virtual UserCore::Item::ItemInfoI* findItemInfo(DesuraId id);
	virtual UserCore::Item::ItemHandleI* findItemHandle(DesuraId id);

	virtual uint32 getCount();
	virtual UserCore::Item::ItemInfoI* getItemInfo(uint32 index);
	virtual UserCore::Item::ItemHandleI* getItemHandle(uint32 index);

	virtual void getAllItems(std::vector<UserCore::Item::ItemInfoI*> &aList);
	virtual void getGameList(std::vector<UserCore::Item::ItemInfoI*> &gList, bool includeDeleted = false);
	virtual void getModList(DesuraId gameId, std::vector<UserCore::Item::ItemInfoI*> &mList, bool includeDeleted = false);
	virtual void getDevList(std::vector<UserCore::Item::ItemInfoI*> &dList);
	virtual void getLinkList(std::vector<UserCore::Item::ItemInfoI*> &lList);
	virtual void getFavList(std::vector<UserCore::Item::ItemInfoI*> &fList);
	virtual void getRecentList(std::vector<UserCore::Item::ItemInfoI*> &rList);
	virtual void getNewItems(std::vector<UserCore::Item::ItemInfoI*> &tList);

	virtual void itemsNeedUpdate(const XML::gcXMLElement &itemsNode);
	virtual void itemsNeedUpdate2(const XML::gcXMLElement &platformsNode);

	virtual EventV* getOnUpdateEvent();
	virtual Event<DesuraId>* getOnRecentUpdateEvent();
	virtual Event<DesuraId>* getOnFavoriteUpdateEvent();
	virtual Event<DesuraId>* getOnNewItemEvent();

	virtual void setFavorite(DesuraId id, bool fav);
	virtual void setRecent(DesuraId id);
	virtual void setInstalledMod(DesuraId parentId, DesuraId modId);
	
	virtual void checkItems();

	void setNew(DesuraId &id);

	UserCore::Item::ItemInfo* findItemInfoNorm(DesuraId id);
	UserCore::Item::ItemHandle* findItemHandleNorm(DesuraId id);

	void enableSave();
	UserCore::Item::ItemTaskGroup* newTaskGroup(uint32 type);

	virtual DesuraId addLink(const char* name, const char* exe, const char* args);
	virtual void updateLink(DesuraId id, const char* args);

	virtual bool isKnownBranch(MCFBranch branch, DesuraId id);
	virtual bool isItemFavorite(DesuraId id);
	
	virtual void regenLaunchScripts();
	
	void saveItem(UserCore::Item::ItemInfoI* pItem) override;

protected:
	class ParseInfo;

	void parseXml(uint16 statusOverride = 0);

	void parseLoginXml(const XML::gcXMLElement &gamesNode, const XML::gcXMLElement &devNodes);
	void parseLoginXml2(const XML::gcXMLElement &gamesNode, const XML::gcXMLElement &platformNodes);
	void postParseLoginXml();

	void parseGamesXml(ParseInfo &pi);
	void parseGameXml(DesuraId id, ParseInfo &pi);

	void parseModsXml(UserCore::Item::ItemInfo* parent, ParseInfo &pi);
	void parseModXml(UserCore::Item::ItemInfo* parent, DesuraId id, ParseInfo &pi);

	void parseItemUpdateXml(const char* area, const XML::gcXMLElement &itemsNode);

	UserCore::Item::ItemInfo* createNewItem(DesuraId pid, DesuraId id, ParseInfo &pi);
	void updateItem(UserCore::Item::ItemInfo* info, ParseInfo &pi);

	DesuraId getParentId(const XML::gcXMLElement &gameNode, const XML::gcXMLElement &infoNode);

	void processLeftOvers(InfoMaps &maps, bool addMissing);

	//! Finds the index of an item given the internal id
	//!
	//! @param internId Internal id of item
	//! @return Item index
	//!
	uint32 findItemIndex(DesuraId id);


	void loadDbItems();
	void saveDbItems(bool fullSave = false);
	void updateItemIds();

	friend class User;
	friend class UpdateThreadOld;

	EventV onUpdateEvent;
	Event<DesuraId> onRecentUpdateEvent;
	Event<DesuraId> onFavoriteUpdateEvent;
	Event<DesuraId> onNewItemEvent;

	void migrateOldItemInfo(const char* olddb, const char* newdb);
	void migrateStandaloneFiles();

	void generateInfoMaps(const XML::gcXMLElement &gamesNode, InfoMaps* maps);


	void parseKnownBranches(const XML::gcXMLElement &gamesNode);

	void onNewItem(DesuraId id);
	bool isDelayLoading();

	void loadFavList();

private:
	uint32 m_uiDelayCount;
	gcString m_szAppPath;

	bool m_bEnableSave;
	bool m_bFirstLogin;

	User* m_pUser;

	std::mutex m_BranchLock;
	std::map<MCFBranch, DesuraId> m_mBranchMapping;

	std::mutex m_FavLock;
	std::vector<DesuraId> m_vFavList;

	std::mutex m_ItemLock;
};




inline uint32 ItemManager::getCount()
{
	std::lock_guard<std::mutex> guard(m_ItemLock);
	return BaseManager::getCount();
}

inline UserCore::Item::ItemInfoI* ItemManager::getItemInfo(uint32 index)
{
	UserCore::Item::ItemHandleI* handle = getItemHandle(index);

	if (handle)
		return handle->getItemInfo();

	return nullptr;
}

inline UserCore::Item::ItemHandleI* ItemManager::getItemHandle(uint32 index)
{
	std::lock_guard<std::mutex> guard(m_ItemLock);
	return BaseManager::getItem(index);
}


inline EventV* ItemManager::getOnUpdateEvent()
{
	return &onUpdateEvent;
}

inline Event<DesuraId>* ItemManager::getOnRecentUpdateEvent()
{
	return &onRecentUpdateEvent;
}

inline Event<DesuraId>* ItemManager::getOnFavoriteUpdateEvent()
{
	return &onFavoriteUpdateEvent;
}

inline Event<DesuraId>* ItemManager::getOnNewItemEvent()
{
	return &onNewItemEvent;
}

}

#endif //DESURA_ITEMMANAGER_H
