#ifndef BOOKMARKSTOOLBAR_INCLUDED
#define BOOKMARKSTOOLBAR_INCLUDED

#include "../Helper/Bookmark.h"

class CBookmarksToolbar : public NBookmark::IBookmarkItemNotification
{
	friend LRESULT CALLBACK BookmarksToolbarProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	CBookmarksToolbar(HWND hToolbar,CBookmarkFolder &AllBookmarks,const GUID &guidBookmarksToolbar);
	~CBookmarksToolbar();

	void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark);
	void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder);
	void	OnBookmarkModified(const GUID &guid);
	void	OnBookmarkFolderModified(const GUID &guid);
	void	OnBookmarkRemoved(const GUID &guid);
	void	OnBookmarkFolderRemoved(const GUID &guid);

private:

	CBookmarksToolbar & operator = (const CBookmarksToolbar &bt);

	LRESULT CALLBACK	BookmarksToolbarProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void	InitializeToolbar();

	void	InsertBookmarkItems();
	void	InsertBookmark(const CBookmark &Bookmark);
	void	InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder);
	void	InsertBookmarkItem(const std::wstring &strName,const GUID &guid,bool bFolder);

	HWND							m_hToolbar;
	CBookmarkFolder					&m_AllBookmarks;
	GUID							m_guidBookmarksToolbar;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDCounter;
};

#endif