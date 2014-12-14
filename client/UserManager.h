/*
 * Copyright (C) 2011-2013 Alexey Solomin, a.rainman on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_USER_MANAGER_H
#define DCPLUSPLUS_DCPP_USER_MANAGER_H

#include "FavoriteManager.h"
#include "Wildcards.h"

class ChatMessage;

class UserManagerListener
{
	public:
		virtual ~UserManagerListener() { }
		template<int I> struct X
		{
			enum { TYPE = I };
		};
		
		typedef X<0> OutgoingPrivateMessage;
		typedef X<1> OpenHub;
		typedef X<2> CollectSummaryInfo;
#ifdef FLYLINKDC_USE_SQL_EXPLORER
		typedef X<3> BrowseSqlExplorer;
#endif
		
		virtual void on(OutgoingPrivateMessage, const UserPtr&, const string&, const tstring&) noexcept { }
		virtual void on(OpenHub, const string&) noexcept { }
		virtual void on(CollectSummaryInfo, const UserPtr&, const string& hubHint) noexcept { }
#ifdef FLYLINKDC_USE_SQL_EXPLORER
		virtual void on(BrowseSqlExplorer, const UserPtr&, const string&) noexcept { }
#endif
};

class UserManager : public Singleton<UserManager>, public Speaker<UserManagerListener>, private SettingsManagerListener
{
	public:
		void outgoingPrivateMessage(const UserPtr& user, const string& hubHint, const tstring& message)
		{
			fire(UserManagerListener::OutgoingPrivateMessage(), user, hubHint, message);
		}
		void openUserUrl(const UserPtr& user)
		{
			const string& url = FavoriteManager::getInstance()->getUserUrl(user);
			if (!url.empty())
			{
				fire(UserManagerListener::OpenHub(), url);
			}
		}
		void collectSummaryInfo(const UserPtr& user, const string& hubHint)
		{
			fire(UserManagerListener::CollectSummaryInfo(), user, hubHint);
		}
#ifdef FLYLINKDC_USE_SQL_EXPLORER
		void browseSqlExplorer(const UserPtr& user, const string& hubHint)
		{
			fire(UserManagerListener::BrowseSqlExplorer(), user, hubHint);
		}
#endif
		enum PasswordStatus
		{
			FIRST = -1,
			FREE = 0,
			CHECKED = 1,
			WAITING = 2,
		};
		static bool expectPasswordFromUser(const UserPtr& user);
		static PasswordStatus checkPrivateMessagePassword(const ChatMessage& pm); // !SMT!-PSW
#ifdef IRAINMAN_INCLUDE_USER_CHECK
		static void checkUser(const OnlineUserPtr& user);
#endif
		typedef StringSet IgnoreMap;
		
		static tstring getIgnoreListAsString();
		static void getIgnoreList(StringSet& p_ignoreList)
		{
			webrtc::ReadLockScoped l(*g_csIgnoreList);
			dcassert(g_ignoreListLoaded);
			p_ignoreList = g_ignoreList;
		}
		static void addToIgnoreList(const string& userName)
		{
			{
				webrtc::WriteLockScoped l(*g_csIgnoreList);
				dcassert(g_ignoreListLoaded);
				g_ignoreList.insert(userName);
			}
			saveIgnoreList();
		}
		static void removeFromIgnoreList(const string& userName)
		{
			{
				webrtc::WriteLockScoped l(*g_csIgnoreList);
				dcassert(g_ignoreListLoaded);
				g_ignoreList.erase(userName);
			}
			saveIgnoreList();
		}
		static bool isInIgnoreList(const string& nick)
		{
			// dcassert(!nick.empty());
			if (!g_isEmptyIgnoreList && !nick.empty())
			{
				dcassert(!nick.empty());
				webrtc::ReadLockScoped l(*g_csIgnoreList);
				dcassert(g_ignoreListLoaded);
				return g_ignoreList.find(nick) != g_ignoreList.cend();
			}
			else
			{
				return false;
			}
		}
		static void setIgnoreList(const IgnoreMap& newlist)
		{
			{
				webrtc::WriteLockScoped l(*g_csIgnoreList);
				g_ignoreList = newlist;
			}
			saveIgnoreList();
		}
		
		static bool protectedUserListEmpty()
		{
			return g_protectedUsersLower.empty();
		}
		static bool isInProtectedUserList(const string& userName)
		{
			webrtc::ReadLockScoped l(*g_csProtectedUsers);
			return Wildcard::patternMatchLowerCase(Text::toLower(userName), g_protectedUsersLower, false);
		}
		
		static bool g_isEmptyIgnoreList;
	private:
		static void saveIgnoreList();
		static IgnoreMap g_ignoreList;
		dcdrun(static bool g_ignoreListLoaded);
		
		typedef std::unordered_set<UserPtr, User::Hash> CheckedUserSet;
		typedef std::unordered_map<UserPtr, bool, User::Hash> WaitingUserMap;
		
		static CheckedUserSet checkedPasswordUsers;
		static WaitingUserMap waitingPasswordUsers;
		
		static FastCriticalSection g_csPsw;
		
		static std::unique_ptr<webrtc::RWLockWrapper> g_csIgnoreList;
		
		friend class Singleton<UserManager>;
		UserManager();
		~UserManager();
		
		void on(SettingsManagerListener::UsersChanges) noexcept;
		
		void on(SettingsManagerListener::Load, SimpleXML& /*xml*/)
		{
			on(SettingsManagerListener::UsersChanges());
		}
		
		static StringList g_protectedUsersLower;
		static std::unique_ptr<webrtc::RWLockWrapper> g_csProtectedUsers;
};

#endif // !defined(DCPLUSPLUS_DCPP_USER_MANAGER_H)
