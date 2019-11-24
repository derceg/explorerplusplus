// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>
#include <vector>
#include <boost/variant.hpp>

class CBookmark;
class CBookmarkFolder;

namespace NBookmark
{
	__interface IBookmarkItemNotification
	{
		void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark,std::size_t Position);
		void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder,std::size_t Position);

		void	OnBookmarkModified(const GUID &guid);
		void	OnBookmarkFolderModified(const GUID &guid);

		void	OnBookmarkRemoved(const GUID &guid);
		void	OnBookmarkFolderRemoved(const GUID &guid);
	};
}

typedef boost::variant<CBookmarkFolder, CBookmark> VariantBookmark;

class CBookmarkFolder
{
public:

	static CBookmarkFolder	Create(const std::wstring &strName,GUID &guid);
	static CBookmarkFolder	Create(const std::wstring &strName);
	static CBookmarkFolder	*CreateNew(const std::wstring &strName,GUID &guid);
	static CBookmarkFolder	*CreateNew(const std::wstring &strName);
	static CBookmarkFolder	UnserializeFromRegistry(const std::wstring &strKey);

	void			SerializeToRegistry(const std::wstring &strKey);

	GUID			GetGUID() const;

	std::wstring	GetName() const;
	void			SetName(const std::wstring &strName);

	FILETIME		GetDateCreated() const;
	FILETIME		GetDateModified() const;

	bool			HasChildren() const;

	/* Returns true if this folder has *at least*
	one child folder. */
	bool			HasChildFolder() const;

	std::list<VariantBookmark>::iterator	begin();
	std::list<VariantBookmark>::iterator	end();

	std::list<VariantBookmark>::const_iterator	begin() const;
	std::list<VariantBookmark>::const_iterator	end() const;

	void			InsertBookmark(const CBookmark &Bookmark);
	void			InsertBookmark(const CBookmark &Bookmark,std::size_t Position);
	void			InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder);
	void			InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder,std::size_t Position);

	void			RemoveBookmark();
	void			RemoveBookmarkFolder();

private:

	enum InitializationType_t
	{
		INITIALIZATION_TYPE_NORMAL,
		INITIALIZATION_TYPE_UNSERIALIZE,
		INITIALIZATION_TYPE_REGISTRY
	};

	CBookmarkFolder(const std::wstring &str,InitializationType_t InitializationType,GUID *guid);

	void			Initialize(const std::wstring &strName,GUID *guid);
	void			InitializeFromRegistry(const std::wstring &strKey);

	void			UpdateModificationTime();

	GUID			m_guid;

	std::wstring	m_strName;

	/* Keeps track of the number of child
	folders that are added. Used purely as
	an optimization for the HasChildFolder()
	method above. */
	int				m_nChildFolders;

	FILETIME		m_ftCreated;
	FILETIME		m_ftModified;

	/* List of child folders and bookmarks. Note that
	the ordering within this list defines the ordering
	between child items (i.e. there is no explicit
	ordering). */
	std::list<VariantBookmark>	m_ChildList;
};

class CBookmark
{
public:

	static CBookmark Create(const std::wstring &strName, const std::wstring &strLocation, const std::wstring &strDescription);
	static CBookmark UnserializeFromRegistry(const std::wstring &strKey);

	void			SerializeToRegistry(const std::wstring &strKey);

	GUID			GetGUID() const;

	std::wstring	GetName() const;
	std::wstring	GetLocation() const;
	std::wstring	GetDescription() const;

	void			SetName(const std::wstring &strName);
	void			SetLocation(const std::wstring &strLocation);
	void			SetDescription(const std::wstring &strDescription);

	int				GetVisitCount() const;
	FILETIME		GetDateLastVisited() const;

	void			UpdateVisitCount();

	FILETIME		GetDateCreated() const;
	FILETIME		GetDateModified() const;

private:

	CBookmark(const std::wstring &strKey);
	CBookmark(const std::wstring &strName, const std::wstring &strLocation, const std::wstring &strDescription);

	void			InitializeFromRegistry(const std::wstring &strKey);

	void			UpdateModificationTime();

	GUID			m_guid;

	std::wstring	m_strName;
	std::wstring	m_strLocation;
	std::wstring	m_strDescription;

	int				m_iVisitCount;
	FILETIME		m_ftLastVisited;

	FILETIME		m_ftCreated;
	FILETIME		m_ftModified;
};

class CBookmarkItemNotifier
{
public:

	static CBookmarkItemNotifier &GetInstance();

	void	AddObserver(NBookmark::IBookmarkItemNotification *pbin);
	void	RemoveObserver(NBookmark::IBookmarkItemNotification *pbin);

	void	NotifyObserversBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark,std::size_t Position);
	void	NotifyObserversBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder,std::size_t Position);
	void	NotifyObserversBookmarkModified(const GUID &guid);
	void	NotifyObserversBookmarkFolderModified(const GUID &guid);
	void	NotifyObserversBookmarkRemoved(const GUID &guid);
	void	NotifyObserversBookmarkFolderRemoved(const GUID &guid);

private:

	enum NotificationType_t
	{
		NOTIFY_BOOKMARK_ADDED,
		NOTIFY_BOOKMARK_FOLDER_ADDED,
		NOTIFY_BOOKMARK_MODIFIED,
		NOTIFY_BOOKMARK_FOLDER_MODIFIED,
		NOTIFY_BOOKMARK_REMOVED,
		NOTIFY_BOOMARK_FOLDER_REMOVED
	};

	CBookmarkItemNotifier() = default;

	CBookmarkItemNotifier(const CBookmarkItemNotifier &);
	CBookmarkItemNotifier & operator=(const CBookmarkItemNotifier &);

	void	NotifyObservers(NotificationType_t NotificationType,const CBookmarkFolder *pParentBookmarkFolder,const CBookmarkFolder *pBookmarkFolder,const CBookmark *pBookmark,const GUID *pguid,std::size_t Position);

	std::list<NBookmark::IBookmarkItemNotification *>	m_listObservers;
};