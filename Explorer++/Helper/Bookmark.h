#ifndef BOOKMARK_INCLUDED
#define BOOKMARK_INCLUDED

#include <list>
#include <boost/variant.hpp>

#define BOOKMARK_TYPE_FOLDER	0
#define BOOKMARK_TYPE_BOOKMARK	1

/* Basic structure used to export bookmark
information. */
typedef struct
{
	/* These attributes are used for both bookmark
	folders and bookmarks themselves. */
	TCHAR			szItemName[256];
	TCHAR			szItemDescription[512];
	BOOL			bShowOnToolbar;
	int				Type;

	/* These attributes are used ONLY for
	bookmarks. */
	TCHAR			szLocation[MAX_PATH];

	/* Used to keep track of which bookmark this
	item refers to internally. This item should
	NOT be used or modified by any external sources. */
	void			*pHandle;
} Bookmark_t;

class CBookmark
{
public:

	CBookmark(void);
	~CBookmark(void);

	void	GetRoot(Bookmark_t *pRoot);
	void	CreateNewBookmark(void *pParentHandle,Bookmark_t *pFolder);
	void	RetrieveBookmark(void *pBookmarkHandle,Bookmark_t *pBookmark);

private:

	typedef struct
	{
		/* The name of this item. */
		TCHAR			szItemName[256];
		TCHAR			szItemDescription[512];

		BOOL			bShowOnToolbar;

		/* The type of the current item. Either
		a folder (BookmarkInternal_t) or bookmark
		(BookmarkNew_t). */
		int				Type;

		/* Pointer to data for the current item. */
		TCHAR			szLocation[MAX_PATH];

		/* Pointers to the parent, first child,
		previous sibling and next sibling
		of this item. */
		void			*Parent;
		void			*PreviousSibling;
		void			*NextSibling;
		void			*FirstChild;
	} BookmarkInternal_t;

	/* The root of the bookmark heirarchy. */
	BookmarkInternal_t m_Root;

	void	ImportBookmarkInternal(BookmarkInternal_t *pbi,Bookmark_t *pBookmark);
	void	ExportBookmarkInternal(BookmarkInternal_t *pbi,Bookmark_t *pBookmark);
};

namespace NBookmarks
{
	enum BookmarkType_t
	{
		TYPE_BOOKMARK,
		TYPE_FOLDER
	};
}

class Bookmark;
class BookmarkFolder;

class BookmarkFolder
{
public:

	BookmarkFolder(const std::wstring &strName);
	~BookmarkFolder();

	UINT			GetID();

	std::wstring	GetName();

	void			SetName(const std::wstring &strName);

	/* Returns true if this folder has *at least*
	one child folder. */
	bool			HasChildFolder();

	std::list<boost::variant<BookmarkFolder,Bookmark>>::iterator	begin();
	std::list<boost::variant<BookmarkFolder,Bookmark>>::iterator	end();

	void			InsertBookmark(const Bookmark &bm);
	void			InsertBookmarkFolder(const BookmarkFolder &bf);
	void			InsertBookmark(const Bookmark &bm,std::size_t Position);
	void			InsertBookmarkFolder(const BookmarkFolder &bf,std::size_t Position);

	void			RemoveBookmark();
	void			RemoveBookmarkFolder();

	/* Retrieves the bookmark item with the
	specified id. Item in this case may
	refer to either a bookmark or bookmark
	folder. */
	std::pair<void *,NBookmarks::BookmarkType_t>	GetBookmarkItem(UINT uID);

	void			GetIterator();

private:

	static UINT		m_IDCounter;
	UINT			m_ID;

	std::wstring	m_strName;

	/* Keeps track of the number of child
	folders that are added. Used purely as
	an optimization for the HasChildFolder()
	method above. */
	int				m_nChildFolders;

	/* These need to be able to be saved and read
	back from storage. */
	FILETIME		m_ftCreated;
	FILETIME		m_ftModified;

	/* List of child folders and bookmarks. Note that
	the ordering within this list defines the ordering
	between child items (i.e. there is no explicit
	ordering). */
	std::list<boost::variant<BookmarkFolder,Bookmark>>	m_ChildList;
};

class Bookmark
{
public:

	Bookmark(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription);
	~Bookmark();

	UINT			GetID();

	std::wstring	GetName();
	std::wstring	GetLocation();
	std::wstring	GetDescription();

	void			SetName(const std::wstring &strName);
	void			SetLocation(const std::wstring &strLocation);
	void			SetDescription(const std::wstring &strDescription);

private:

	static UINT		m_IDCounter;
	UINT			m_ID;

	std::wstring	m_strName;
	std::wstring	m_strLocation;
	std::wstring	m_strDescription;

	FILETIME		m_ftCreated;
	FILETIME		m_ftModified;
};

#endif