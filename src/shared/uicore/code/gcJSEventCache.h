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

#ifndef DESURA_JSEVENTCACHE_H
#define DESURA_JSEVENTCACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "gcJSBinding.h"

#include "cef_desura_includes/ChromiumBrowserI.h"

extern DesuraJSBinding *GetJSBinding();
extern void BrowserUICallback(ChromiumDLL::CallbackI* callback);

class JSEventMap
{
public:
	ChromiumDLL::JSObjHandle findEventFunction(const gcString &name, ChromiumDLL::JSObjHandle root)
	{
		std::lock_guard<std::mutex> al(m_EventLock);

		if (!s_bMapValid)
			return nullptr;

		if (m_mEventMap.find(name) != m_mEventMap.end())
			return m_mEventMap[name];

		if (!root.get() || root->isNull())
			return nullptr;

		if (m_mEventMap.find("__desura__") == m_mEventMap.end())
			m_mEventMap["__desura__"] = root->getValue("desura");

		if (m_mEventMap.find("__events__") == m_mEventMap.end())
			m_mEventMap["__events__"] = m_mEventMap["__desura__"]->getValue("events");

		if (m_mEventMap.find("__internal__") == m_mEventMap.end())
			m_mEventMap["__internal__"] = m_mEventMap["__events__"]->getValue("internal");

		ChromiumDLL::JSObjHandle ret = m_mEventMap["__internal__"]->getValue(name.c_str());
		m_mEventMap[name] = ret;

		return ret;
	}

	void reset()
	{
		std::lock_guard<std::mutex> al(m_EventLock);
		m_mEventMap.clear();
	}

	bool isVaild()
	{
		std::lock_guard<std::mutex> al(m_EventLock);
		return s_bMapValid;
	}

	void setValid(bool bState)
	{
		std::lock_guard<std::mutex> al(m_EventLock);
		s_bMapValid = bState;
	}
private:
	std::mutex m_EventLock;
	std::map<gcString, ChromiumDLL::JSObjHandle> m_mEventMap;
	bool s_bMapValid = false;
};






class JSCallback : public ChromiumDLL::CallbackI
{
public:
	JSCallback(JSEventMap &eventMap, bool &bGlobalItemUpdate, ChromiumDLL::JavaScriptContextI* context, gcString name, const char* arg1, const char* arg2)
		: m_EventMap(eventMap)
		, m_bGlobalItemUpdate(bGlobalItemUpdate)
		, m_pContext(context)
		, m_szName(name)
	{
		m_uiNumArgs = 0;

		if (arg1)
		{
			m_szArg1 = arg1;
			m_uiNumArgs = 1;
		}

		if (arg2)
		{
			m_szArg2 = arg2;
			m_uiNumArgs = 2;
		}

		if (m_szName == "onItemListUpdated")
			m_bGlobalItemUpdate = true;
	}

	void destroy() override
	{
		if (m_pContext)
			m_pContext->destroy();

		delete this;
	}

	void run() override
	{
		try
		{
			doRun();
		}
		catch (...)
		{
			Warning(gcString("JSCallback {0} threw exception", m_szName));
		}

		if (m_szName == "onItemListUpdated")
			m_bGlobalItemUpdate = false;
	}

protected:
	void doRun()
	{
		if (!m_EventMap.isVaild())
			return;

		if (m_szName == "onItemUpdate" && m_bGlobalItemUpdate)
			return;

		if (!m_pContext)
			return;

		m_pContext->enter();
		ChromiumDLL::JSObjHandle funct = m_EventMap.findEventFunction(m_szName, m_pContext->getGlobalObject());

		if (funct.get())
		{
			ChromiumDLL::JSObjHandle* argv = nullptr;

			if (m_uiNumArgs > 0)
				argv = new ChromiumDLL::JSObjHandle[m_uiNumArgs];

			if (m_uiNumArgs >= 1)
				argv[0] = m_pContext->getFactory()->CreateString(m_szArg1.c_str());

			if (m_uiNumArgs >= 2)
				argv[1] = m_pContext->getFactory()->CreateString(m_szArg2.c_str());

			ChromiumDLL::JavaScriptFunctionArgs args;
			args.function = nullptr;
			args.context = m_pContext;
			args.argc = m_uiNumArgs;
			args.argv = argv;
			args.factory = nullptr;
			args.object = nullptr;

			ChromiumDLL::JSObjHandle ret = funct->executeFunction(&args);
			delete[] argv;
		}

		m_pContext->exit();
	}

private:
	uint32 m_uiNumArgs;

	gcString m_szArg1;
	gcString m_szArg2;

	ChromiumDLL::JavaScriptContextI* m_pContext;
	gcString m_szName;

	JSEventMap &m_EventMap;
	bool &m_bGlobalItemUpdate;
};


#endif