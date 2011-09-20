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
		virtual void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark,std::size_t Position);
		virtual void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder,std::size_t Position);

		virtual void	OnBookmarkModified(const GUID &guid);
		virtual void	OnBookmarkFolderModified(const GUID &guid);

		virtual void	OnBookmarkRemoved(const GUID &guid);
		virtual void	OnBookmarkFolderRemoved(const GUID &guid);
	};

	struct SerializedData_t
	{
		void	*pData;
		UINT	uSize;
	};

	const int VERSION_NUMBER_MISMATCH = 1;
}

class CBookmarkFolder
{
public:

	static CBookmarkFolder	Create(const std::wstring &strName);
	static CBookmarkFolder	*CreateNew(const std::wstring &strName);
	static CBookmarkFolder	Unserialize(void *pSerializedData);
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

	NBookmark::SerializedData_t	Serialize() const;

private:

	enum InitializationType_t
	{
		INITIALIZATION_TYPE_NORMAL,
		INITIALIZATION_TYPE_UNSERIALIZE,
		INITIALIZATION_TYPE_REGISTRY
	};

	struct BookmarkFolderSerialized_t
	{
		UINT		uSize;

		GUID		guid;

		TCHAR		Name[256];

		FILETIME	ftCreated;
		FILETIME	ftModified;
	};

	CBookmarkFolder(const std::wstring &str,InitializationType_t InitializationType);
	CBookmarkFolder(void *pSerializedData);

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
	CBookmark(void *pSerializedData);
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

	/* Note that this method will allocate data
	using new; it is up to the caller to delete it. */
	NBookmark::SerializedData_t	Serialize() const;

private:

	/* Whenever a member is added, removed
	or changed within this class, it must
	be added, removed or changed within this
	structure. */
	struct BookmarkSerialized_t
	{
		/* Size of the structure. Used to ensure binary
		compatibility. When unserializing a bookmark,
		if this size does not match the internal size
		of this structure, the data will be rejected.
		Note that this element MUST appear first in this
		structure. */
		UINT		uSize;

		GUID		guid;

		TCHAR		Name[256];
		TCHAR		Location[256];
		TCHAR		Description[256];

		int			iVisitCount;
		FILETIME	ftLastVisited;

		FILETIME	ftCreated;
		FILETIME	ftModified;
	};

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

	CBookmarkItemNotifier();

	CBookmarkItemNotifier(const CBookmarkItemNotifier &);
	CBookmarkItemNotifier & operator=(const CBookmarkItemNotifier &);

	void	NotifyObservers(NotificationType_t NotificationType,const CBookmarkFolder *pParentBookmarkFolder,const CBookmarkFolder *pBookmarkFolder,const CBookmark *pBookmark,const GUID *pguid,std::size_t Position);

	std::list<NBookmark::IBookmarkItemNotification *>	m_listObservers;
};

#endif