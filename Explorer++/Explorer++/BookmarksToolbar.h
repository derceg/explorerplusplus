#pragma once

#include "../Helper/Bookmark.h"
#include <boost/optional.hpp>

class CBookmarksToolbarDropHandler : public IDropTarget
{
public:

	CBookmarksToolbarDropHandler(HWND hToolbar,CBookmarkFolder &AllBookmarks,const GUID &guidBookmarksToolbar);
	~CBookmarksToolbarDropHandler();

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

private:

	CBookmarksToolbarDropHandler & operator = (const CBookmarksToolbarDropHandler &btdh);

	/* IDropTarget methods. */
	HRESULT __stdcall	DragEnter(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT __stdcall	DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT __stdcall	DragLeave(void);
	HRESULT __stdcall	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);

	int					GetToolbarPositionIndex(const POINTL &pt,bool &bAfter);
	void				RemoveInsertionMark();

	ULONG				m_ulRefCount;

	HWND				m_hToolbar;
	CBookmarkFolder		&m_AllBookmarks;
	GUID				m_guidBookmarksToolbar;

	IDragSourceHelper	*m_pDragSourceHelper;
	IDropTargetHelper	*m_pDropTargetHelper;
	bool				m_bAcceptData;
};

class CBookmarksToolbar : public NBookmark::IBookmarkItemNotification
{
	friend LRESULT CALLBACK BookmarksToolbarProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
	friend LRESULT CALLBACK BookmarksToolbarParentProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	CBookmarksToolbar(HWND hToolbar, IExplorerplusplus *pexpp, CBookmarkFolder &AllBookmarks, const GUID &guidBookmarksToolbar, UINT uIDStart, UINT uIDEnd);
	~CBookmarksToolbar();

	/* IBookmarkItemNotification methods. */
	void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark,std::size_t Position);
	void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder,std::size_t Position);
	void	OnBookmarkModified(const GUID &guid);
	void	OnBookmarkFolderModified(const GUID &guid);
	void	OnBookmarkRemoved(const GUID &guid);
	void	OnBookmarkFolderRemoved(const GUID &guid);

private:

	CBookmarksToolbar & operator = (const CBookmarksToolbar &bt);

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	LRESULT CALLBACK	BookmarksToolbarProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	BookmarksToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void	InitializeToolbar();

	void	InsertBookmarkItems();
	void	InsertBookmark(const CBookmark &Bookmark);
	void	InsertBookmark(const CBookmark &Bookmark,std::size_t Position);
	void	InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder);
	void	InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder,std::size_t Position);
	void	InsertBookmarkItem(const std::wstring &strName,const GUID &guid,bool bFolder,std::size_t Position);

	void	ModifyBookmarkItem(const GUID &guid,bool bFolder);

	void	RemoveBookmarkItem(const GUID &guid);

	bool	OnCommand(WPARAM wParam, LPARAM lParam);
	bool	OnGetInfoTip(NMTBGETINFOTIP *infoTip);

	int		GetBookmarkItemIndex(const GUID &guid);

	boost::optional<NBookmarkHelper::variantBookmark_t>	GetBookmarkItemFromToolbarIndex(int index);

	HWND							m_hToolbar;
	HIMAGELIST						m_himl;

	IExplorerplusplus				*m_pexpp;

	CBookmarkFolder					&m_AllBookmarks;
	GUID							m_guidBookmarksToolbar;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDStart;
	UINT							m_uIDEnd;
	UINT							m_uIDCounter;

	CBookmarksToolbarDropHandler	*m_pbtdh;
};