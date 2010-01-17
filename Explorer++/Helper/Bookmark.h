#ifndef BOOKMARK_INCLUDED
#define BOOKMARK_INCLUDED

#include <list>

using namespace std;

#define BOOKMARK_TYPE_FOLDER	0
#define BOOKMARK_TYPE_BOOKMARK	1

#define GROUP_NAME_MAX_CHARS	256

/* Basic structure used to export bookmark
information. */
typedef struct
{
	/* These attributes are used for both bookmark
	folders and bookmarks themselves. */
	TCHAR			szItemName[GROUP_NAME_MAX_CHARS];
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
	HRESULT	GetChild(Bookmark_t *pParent,Bookmark_t *pChild);
	HRESULT	GetChildFolder(Bookmark_t *pParent,Bookmark_t *pChildFolder);
	HRESULT	GetNextBookmarkSibling(Bookmark_t *pParent,Bookmark_t *pSibling);
	HRESULT	GetNextFolderSibling(Bookmark_t *pParent,Bookmark_t *pFolderSibling);
	void	CreateNewBookmark(void *pParentHandle,Bookmark_t *pFolder);
	void	DeleteBookmark(void *pBookmarkHandle);
	void	UpdateBookmark(void *pBookmarkHandle,Bookmark_t *pUpdatedBookmark);
	void	RetrieveBookmark(void *pBookmarkHandle,Bookmark_t *pBookmark);
	void	SwapBookmarks(Bookmark_t *pBookmark1,Bookmark_t *pBookmark2);

private:

	typedef struct
	{
		/* The name of this item. */
		TCHAR			szItemName[GROUP_NAME_MAX_CHARS];
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

#endif