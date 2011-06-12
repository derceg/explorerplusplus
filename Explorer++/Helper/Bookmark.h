#ifndef BOOKMARK_INCLUDED
#define BOOKMARK_INCLUDED

#include <list>
#include <vector>
#include <boost/variant.hpp>

class CBookmark;
class CBookmarkFolder;

namespace NBookmark
{
	__interface IBookmarkItemNotification
	{
		virtual void	OnBookmarkItemModified(const GUID &guid);

		virtual void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark);
		virtual void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder);

		virtual void	OnBookmarkRemoved(const GUID &guid);
		virtual void	OnBookmarkFolderRemoved(const GUID &guid);
	};
}

class CBookmarkFolder
{
public:

	static CBookmarkFolder	Create(const std::wstring &strName);
	static CBookmarkFolder	*CreateNew(const std::wstring &strName);
	static CBookmarkFolder	UnserializeFromRegistry(const std::wstring &strKey);

	~CBookmarkFolder();

	void			SerializeToRegistry(const std::wstring &strKey);

	GUID			GetGUID() const;

	std::wstring	GetName() const;
	void			SetName(const std::wstring &strName);

	FILETIME		GetDateCreated() const;
	FILETIME		GetDateModified() const;

	/* Returns true if this folder has *at least*
	one child folder. */
	bool			HasChildFolder() const;

	std::list<boost::variant<CBookmarkFolder,CBookmark>>::iterator	begin();
	std::list<boost::variant<CBookmarkFolder,CBookmark>>::iterator	end();

	std::list<boost::variant<CBookmarkFolder,CBookmark>>::const_iterator	begin() const;
	std::list<boost::variant<CBookmarkFolder,CBookmark>>::const_iterator	end() const;

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
		INITIALIZATION_TYPE_REGISTRY
	};

	CBookmarkFolder(const std::wstring &str,InitializationType_t InitializationType);

	void			Initialize(const std::wstring &strName);
	void			InitializeFromRegistry(const std::wstring &strKey);

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
	std::list<boost::variant<CBookmarkFolder,CBookmark>>	m_ChildList;
};

class CBookmark
{
public:

	CBookmark(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription);
	~CBookmark();

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

	~CBookmarkItemNotifier();

	static CBookmarkItemNotifier &GetInstance();

	void	AddObserver(NBookmark::IBookmarkItemNotification *pbin);
	void	RemoveObserver(NBookmark::IBookmarkItemNotification *pbin);

	void	NotifyObserversBookmarkItemModified(const GUID &guid);
	void	NotifyObserversBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark);
	void	NotifyObserversBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder);
	void	NotifyObserversBookmarkRemoved(const GUID &guid);
	void	NotifyObserversBookmarkFolderRemoved(const GUID &guid);

private:

	enum NotificationType_t
	{
		NOTIFY_BOOKMARK_ITEM_MODIFIED,
		NOTIFY_BOOKMARK_ADDED,
		NOTIFY_BOOKMARK_FOLDER_ADDED,
		NOTIFY_BOOKMARK_REMOVED,
		NOTIFY_BOOMARK_FOLDER_REMOVED
	};

	CBookmarkItemNotifier();

	CBookmarkItemNotifier(const CBookmarkItemNotifier &);
	CBookmarkItemNotifier & operator=(const CBookmarkItemNotifier &);

	void	NotifyObservers(NotificationType_t NotificationType,const CBookmarkFolder *pParentBookmarkFolder,const CBookmarkFolder *pBookmarkFolder,const CBookmark *pBookmark,const GUID *pguid);

	std::list<NBookmark::IBookmarkItemNotification *>	m_listObservers;
};

#endif