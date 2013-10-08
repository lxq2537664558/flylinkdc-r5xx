/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_SHARE_MANAGER_H
#define DCPLUSPLUS_DCPP_SHARE_MANAGER_H

#include <boost/atomic.hpp>

#include "SearchManager.h"
#include "LogManager.h"
#include "HashManager.h"
#include "QueueManagerListener.h"
#include "BloomFilter.h"
#include "Pointer.h"

#ifdef _WIN32
# include <ShlObj.h> //[+]PPA
#endif

#ifdef STRONG_USE_DHT
namespace dht
{
class IndexManager;
}
#endif

STANDARD_EXCEPTION_ADD_INFO(ShareException); // [!] FlylinkDC++

class SimpleXML;
class Client;
class File;
class OutputStream;
class MemoryInputStream;

struct ShareLoader;
class ShareManager : public Singleton<ShareManager>, private SettingsManagerListener, private BASE_THREAD, private TimerManagerListener,
	private HashManagerListener, private QueueManagerListener
{
	public:
		/**
		 * @param aDirectory Physical directory location
		 * @param aName Virtual name
		 */
		void addDirectory(const string& realPath, const string &virtualName);
		void removeDirectory(const string& realPath);
		void renameDirectory(const string& realPath, const string& virtualName);
		
		string toRealPath(const TTHValue& tth) const; // !PPA!
		
		bool destinationShared(const string& file_or_dir_name) const // [+] IRainman opt.
		{
			SharedLock l(cs);
			for (auto i = shares.cbegin(); i != shares.cend(); ++i)
				if (strnicmp(i->first, file_or_dir_name, i->first.size()) == 0 && file_or_dir_name[i->first.size() - 1] == PATH_SEPARATOR)
					return true;
			return false;
		}
		
		bool getRealPathAndSize(const TTHValue& tth, string& path, int64_t& size) const // [+] IRainman TODO.
		{
			SharedLock l(cs);
			const auto i = tthIndex.find(tth);
			if (i != tthIndex.cend())
			{
				try
				{
					const Directory::File& f = *i->second;
					path = f.getRealPath();
					size = f.getSize();
					return true;
				}
				catch (const ShareException&) { }
			}
			return false;
		}
#ifdef _DEBUG
		string toVirtual(const TTHValue& tth) const;
#endif
		string toReal(const string& virtualFile
#ifdef IRAINMAN_INCLUDE_HIDE_SHARE_MOD
		              , bool ishidingShare
#endif
		             );
		TTHValue getTTH(const string& virtualFile) const;
		
		void refresh(bool dirs = false, bool aUpdate = true, bool block = false) noexcept;
		void setDirty()
		{
			xmlDirty = true;
			m_isNeedsUpdateShareSize = true;
		}
		void setPurgeTTH()
		{
			m_sweep_path = true;
		}
		
		
		bool isShareFolder(const string& path, bool thoroughCheck = false) const;
		int64_t removeExcludeFolder(const string &path, bool returnSize = true);
		int64_t addExcludeFolder(const string &path);
		
		void search(SearchResultList& l, const string& aString, Search::SizeModes aSizeMode, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) noexcept;
		void search(SearchResultList& l, const StringList& params, StringList::size_type maxResults, StringSearch::List& reguest) noexcept; // [!] IRainman-S add StringSearch::List& reguest
		
		bool findByRealPathName(const string& realPathname, TTHValue* outTTHPtr, string* outfilenamePtr = NULL, int64_t* outSizePtr = NULL); // [+] SSA
		
		StringPairList getDirectories() const noexcept;
		
		MemoryInputStream* generatePartialList(const string& dir, bool recurse
#ifdef IRAINMAN_INCLUDE_HIDE_SHARE_MOD
		                                       , bool ishidingShare
#endif
		                                      ) const;
		                                      
		MemoryInputStream* getTree(const string& virtualFile) const;
		
		AdcCommand getFileInfo(const string& aFile);
		// [!] IRainman opt.
		int64_t getShareSize()
		{
			dcassert(m_CurrentShareSize != -1); // TODO - ���. ������� �������� ������ ���� �� ������ internal_calcShareSize
#ifndef FLYLINKDC_HE
			internal_calcShareSize(); // [+]PPA
#endif
			return m_CurrentShareSize;
		}
	private:
		void internal_calcShareSize() noexcept;
	public:
		// [~] IRainman opt.
		int64_t getShareSize(const string& realPath) const noexcept;
		
		size_t getSharedFiles() const noexcept
		{
		    // [-] SharedLock l(cs); [-] IRainman opt.
		    return tthIndex.size();
		}
		
		string getShareSizeString()
		{
			return Util::toString(getShareSize());
		}
		string getShareSizeString(const string& aDir) const
		{
			return Util::toString(getShareSize(aDir));
		}
		
		void getBloom(ByteVector& v, size_t k, size_t m, size_t h) const;
		
		static SearchManager::TypeModes getFType(const string& fileName) noexcept;
		string validateVirtual(const string& aVirt) const noexcept;
		bool hasVirtual(const string& name) const noexcept;
		
		void incHits()
		{
			++hits;
		}
		
		const string& getOwnListFile()
		{
			generateXmlList();
			return bzXmlFile;
		}
		
		bool isTTHShared(const TTHValue& tth) const
		{
			if (!isShutdown())
			{
				SharedLock l(cs);
				return tthIndex.find(tth) != tthIndex.end();
			}
			return false;
		}
		
		GETSET(size_t, hits, Hits);
		GETSET(string, bzXmlFile, BZXmlFile);
		GETSET(int64_t, sharedSize, SharedSize);
		
	private:
#ifdef IRAINMAN_INCLUDE_HIDE_SHARE_MOD
		static string getEmptyBZXmlFile()
		{
			return Util::getConfigPath() + "Emptyfiles.xml.bz2";
		}
#endif
		static string getDefaultBZXmlFile() // [+] IRainman fix.
		{
			return Util::getConfigPath() + "files.xml.bz2";
		}
		struct AdcSearch;
		class Directory : public intrusive_ptr_base<Directory>
#ifdef _DEBUG
			, virtual NonDerivable<Directory>, boost::noncopyable // [+] IRainman fix.
#endif
		{
			public:
				typedef boost::intrusive_ptr<Directory> Ptr;
				typedef std::map<string, Ptr> Map;
				typedef Map::iterator MapIter;
				
				struct File
#ifdef _DEBUG
						: virtual NonDerivable<File>, boost::noncopyable // [+] IRainman fix.
#endif
				{
						struct StringComp
						{
								explicit StringComp(const string& s) : a(s) { }
								bool operator()(const File& b) const
								{
									return stricmp(a, b.getName()) == 0;
								}
							private:
								const string& a;
								void operator=(const StringComp&); // [!] IRainman fix.
						};
						struct FileTraits
						{
							size_t operator()(const File& a) const
							{
								return std::hash<std::string>()(a.getName());
							}
							bool operator()(const File &a, const File& b) const
							{
								return a.getName() == b.getName();
							}
						};
						typedef unordered_set<File, FileTraits, FileTraits> Set;
						
						
						File() : size(0), parent(0), hit(0), ts(0), ftype(SearchManager::TYPE_ANY) { }
						File(const string& aName, int64_t aSize, Directory::Ptr aParent, const TTHValue& aRoot, uint32_t aHit, uint32_t aTs,
						     SearchManager::TypeModes aftype, const CFlyMediaInfo& p_media) :
							tth(aRoot), size(aSize), parent(aParent.get()), hit(aHit), ts(aTs), ftype(aftype),
							m_media(p_media)
						{
							setName(aName);
						}
						File(const File& rhs) :
							ftype(rhs.ftype),
							m_media(rhs.m_media),
							ts(rhs.ts), hit(rhs.hit), tth(rhs.getTTH()), size(rhs.getSize()), parent(rhs.getParent()
							                                                                        )
						{
							setName(rhs.getName());
						}
						
						~File() { }
						
						File& operator=(const File& rhs)
						{
							setName(rhs.m_name);
							size = rhs.size;
							parent = rhs.parent;
							tth = rhs.tth;
							hit = rhs.hit;
							ts = rhs.ts;
							m_media = rhs.m_media;
							ftype = rhs.ftype;
							return *this;
						}
						
						bool operator==(const File& rhs) const
						{
							return getParent() == rhs.getParent() && (stricmp(getName(), rhs.getName()) == 0);
						}
						string getADCPath() const
						{
							return parent->getADCPath() + getName();
						}
						string getFullName() const
						{
							return parent->getFullName() + getName();
						}
						string getRealPath() const
						{
							return parent->getRealPath(getName());
						}
						
					private:
						string m_name;
						string m_low_name;
					public:
						const string& getName() const
						{
							return m_name;
						}
						const string& getLowName() const //[+]PPA http://flylinkdc.blogspot.com/2010/08/1.html
						{
							return m_low_name;
						}
						void setName(const string& p_name)
						{
							m_name = p_name;
							m_low_name = Text::toLower(m_name);
						}
						GETSET(int64_t, size, Size);
						GETSET(Directory*, parent, Parent);
						GETSET(uint32_t, hit, Hit);
						GETSET(uint32_t, ts, TS);
						CFlyMediaInfo m_media;
						const TTHValue& getTTH() const
						{
							return tth;
						}
						void setTTH(const TTHValue& p_tth)
						{
							tth = p_tth;
						}
						SearchManager::TypeModes getFType() const
						{
							return ftype;
						}
					private:
						TTHValue tth;
						SearchManager::TypeModes ftype;
						
				};
				
				Map directories;
				File::Set files;
				int64_t size;
				
				static Ptr create(const string& aName, const Ptr& aParent = Ptr())
				{
					return Ptr(new Directory(aName, aParent));
				}
				
				bool hasType(uint32_t type) const noexcept
				{
				    return ((type == SearchManager::TYPE_ANY) || (fileTypes & (1 << type)));
				}
				void addType(uint32_t type) noexcept;
				
				string getADCPath() const noexcept;
				string getFullName() const noexcept;
				string getRealPath(const std::string& path) const;
				
				int64_t getSize() const noexcept;
				
				void search(SearchResultList& aResults, StringSearch::List& aStrings, Search::SizeModes aSizeMode, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) const noexcept;
				void search(SearchResultList& aResults, AdcSearch& aStrings, StringList::size_type maxResults) const noexcept;
				
				void toXml(OutputStream& xmlFile, string& indent, string& tmp2, bool fullList) const;
				void filesToXml(OutputStream& xmlFile, string& indent, string& tmp2) const;
				
				File::Set::const_iterator findFile(const string& aFile) const
				{
					return find_if(files.begin(), files.end(), Directory::File::StringComp(aFile));
				}
				
				void merge(const Ptr& source);
				
				const string& getName() const
				{
					return m_name;
				}
				const string& getLowName() const
				{
					return m_low_name;
				}
				void setName(const string& p_name)
				{
					m_name = p_name;
					m_low_name = Text::toLower(m_name);
				}
				GETSET(Directory*, parent, Parent);
			private:
				friend void intrusive_ptr_release(intrusive_ptr_base<Directory>*);
				
				Directory(const string& aName, const Ptr& aParent);
				~Directory() { } // [3] https://www.box.net/shared/fa054c428ddf0a88fb0e
				// 2012-04-29_06-52-32_7DCSOGEGBL7SCFPXL7QNF2EUVF3XY22Y6PVKEWQ_4F9F3ED2_crash-stack-r502-beta23-build-9860.dmp
				// 2012-05-11_23-53-01_A2IPHZCLMTKWDAQMIBJIHD4HBRRH5LUIVPTZYSA_F48E1EF7_crash-stack-r502-beta26-build-9946.dmp
				
				string m_name;
				string m_low_name;  //[+]PPA http://flylinkdc.blogspot.com/2010/08/1.html
				/** Set of flags that say which SearchManager::TYPE_* a directory contains */
				uint8_t fileTypes; // SearchManager::TypeModes
				
		};
		
		friend class Directory;
		friend struct ShareLoader;
		
		friend class Singleton<ShareManager>;
		ShareManager();
		~ShareManager();
		
		struct AdcSearch
		{
			AdcSearch(const StringList& params);
			
			bool isExcluded(const string& str);
			bool hasExt(const string& name);
			
			StringSearch::List* include;
			StringSearch::List includeX;
			StringSearch::List exclude;
			StringList ext;
			StringList noExt;
			
			int64_t gt;
			int64_t lt;
			
			TTHValue root;
			bool hasRoot;
			
			bool isDirectory;
		};
		
		boost::atomic_flag updateXmlListInProcess; // [+] IRainman opt.
		
		int64_t xmlListLen;
		TTHValue xmlRoot;
		int64_t bzXmlListLen;
		TTHValue bzXmlRoot;
		unique_ptr<File> bzXmlRef;
		
		bool xmlDirty;
		bool forceXmlRefresh; /// bypass the 15-minutes guard
		bool refreshDirs;
		bool update;
		bool initial;
		
		int listN;
		
		boost::atomic_flag refreshing;
		
		uint64_t lastXmlUpdate;
		uint64_t lastFullUpdate;
		
		mutable SharedCriticalSection cs; // [!] IRainman opt.
		
		// List of root directory items
		typedef std::list<Directory::Ptr> DirList;
		DirList directories;
		
		/** Map real name to virtual name - multiple real names may be mapped to a single virtual one */
		StringMap shares;
		
#ifdef STRONG_USE_DHT
		friend class ::dht::IndexManager;
#endif
		
		typedef unordered_map<TTHValue, Directory::File::Set::const_iterator> HashFileMap;
		
		HashFileMap tthIndex;
		//[+]IRainman opt.
		bool m_isNeedsUpdateShareSize;
		int64_t m_CurrentShareSize;
		static bool g_isShutdown;
		//[~]IRainman
		
		BloomFilter<5> m_bloom;
		
		Directory::File::Set::const_iterator findFile(const string& virtualFile) const;
		
		Directory::Ptr buildTree(const string& aName, const Directory::Ptr& aParent, bool p_is_job);
#ifdef PPA_INCLUDE_ONLINE_SWEEP_DB
		bool m_sweep_guard;
#endif
		bool m_sweep_path;
		bool checkHidden(const string& aName) const;
		//[+]IRainman
		bool checkSystem(const string& aName) const;
		bool checkVirtual(const string& aName) const;
		bool checkAttributs(const string& aName) const;
		//[~]IRainman
		void rebuildIndices();
		
		void updateIndices(Directory& aDirectory);
		void updateIndices(Directory& dir, const Directory::File::Set::iterator& i);
		
		Directory::Ptr merge(const Directory::Ptr& directory);
		
		void generateXmlList();
		StringList m_notShared;
		bool loadCache() noexcept;
		DirList::const_iterator getByVirtual(const string& virtualName) const noexcept;
		pair<Directory::Ptr, string> splitVirtual(const string& virtualPath) const;
		string findRealRoot(const string& virtualRoot, const string& virtualLeaf) const;
		
		Directory::Ptr getDirectory(const string& fname) const;
		
		int run();
	public:
#ifdef USE_REBUILD_MEDIAINFO
		struct MediainfoFile
		{
			string m_path;
			string m_file_name;
			__int64  m_size;
			__int64  m_path_id;
			TTHValue m_tth;
		};
		typedef std::vector<MediainfoFile> MediainfoFileArray;
		__int64 rebuildMediainfo(CFlyLog& p_log);
		__int64 rebuildMediainfo(Directory& p_dir, CFlyLog& p_log, ShareManager::MediainfoFileArray& p_result);
#endif
		void shutdown()//[+]IRainman stopping buildTree
		{
			dcassert(!isShutdown());
			g_isShutdown = true;
		}
		static bool isShutdown() // TODO ������ � ������������ ����� - ������ Singleton-�
		{
			return g_isShutdown;
		}
		
		bool isFileInSharedDirectory(const string& p_fname) const
		{
			return getDirectory(p_fname) != NULL;
		}
		
	private:
		// QueueManagerListener
		void on(QueueManagerListener::FileMoved, const string& n) noexcept;
		
		// HashManagerListener
		void on(HashManagerListener::TTHDone, const string& fname, const TTHValue& root,
		        int64_t aTimeStamp, const CFlyMediaInfo& p_out_media, int64_t p_size) noexcept;
		        
		// SettingsManagerListener
		void on(SettingsManagerListener::Save, SimpleXML& xml) noexcept
		{
			save(xml);
		}
		void on(SettingsManagerListener::Load, SimpleXML& xml) noexcept
		{
			on(SettingsManagerListener::ShareChanges());
			load(xml);
		}
		// [+] IRainman opt.
		void on(SettingsManagerListener::ShareChanges) noexcept
		{
			auto skipList = SPLIT_SETTING(SKIPLIST_SHARE);
			
			FastLock l(m_csSkipList);
			swap(m_skipList, skipList);
		}
		
		bool isInSkipList(const string& lowerName) const
		{
			FastLock l(m_csSkipList);
			return Wildcard::patternMatchLowerCase(lowerName, m_skipList); // https://www.box.net/shared/vrbq7dqu5gqzdiu4gjop
		}
		bool skipListEmpty() const
		{
			return m_skipList.empty();
		}
		StringList m_skipList;
		mutable FastCriticalSection m_csSkipList;
		// [~] IRainman opt.
		
		// TimerManagerListener
		void on(TimerManagerListener::Minute, uint64_t tick) noexcept;
		void load(SimpleXML& aXml);
		void save(SimpleXML& aXml);
		
};

#endif // !defined(SHARE_MANAGER_H)

/**
 * @file
 * $Id: ShareManager.h 568 2011-07-24 18:28:43Z bigmuscle $
 */