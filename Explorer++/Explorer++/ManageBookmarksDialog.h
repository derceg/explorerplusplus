#ifndef MANAGEBOOKMARKSDIALOG_INCLUDED
#define MANAGEBOOKMARKSDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/Bookmark.h"

class CManageBookmarksDialog;

class CManageBookmarksDialogPersistentSettings : public CDialogSettings
{
public:

	~CManageBookmarksDialogPersistentSettings();

	static CManageBookmarksDialogPersistentSettings &GetInstance();

private:

	friend CManageBookmarksDialog;

	static const TCHAR SETTINGS_KEY[];
	static const int DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH = 180;

	CManageBookmarksDialogPersistentSettings();

	CManageBookmarksDialogPersistentSettings(const CManageBookmarksDialogPersistentSettings &);
	CManageBookmarksDialogPersistentSettings & operator=(const CManageBookmarksDialogPersistentSettings &);

	int	m_iColumnWidth1;
	int	m_iColumnWidth2;
};

class CManageBookmarksDialog : public CBaseDialog
{
public:

	CManageBookmarksDialog(HINSTANCE hInstance,int iResource,HWND hParent,BookmarkFolder *pAllBookmarks);
	~CManageBookmarksDialog();

	LRESULT CALLBACK	EditSearchProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

protected:

	BOOL	OnInitDialog();
	INT_PTR	OnCtlColorEdit(HWND hwnd,HDC hdc);
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();
	BOOL	OnDestroy();

	void	SaveState();

private:

	static const COLORREF SEARCH_TEXT_COLOR = RGB(120,120,120);

	void		SetupTreeView();
	void		SetupListView();

	void		SetSearchFieldDefaultState();
	void		RemoveSearchFieldDefaultState();

	void		InsertFoldersIntoTreeView();
	HTREEITEM	InsertFolderIntoTreeView(HWND hTreeView,HTREEITEM hParent,BookmarkFolder *pBookmarkFolder);
	BookmarkFolder	*GetBookmarkFolderFromTreeView(HTREEITEM hItem);

	void		InsertBookmarksIntoListView(BookmarkFolder *pBookmarkFolder);
	void		InsertBookmarkFolderIntoListView(HWND hListView,BookmarkFolder *pBookmarkFolder,int iPosition);
	void		InsertBookmarkIntoListView(HWND hListView,Bookmark *pBookmark);

	void		OnOk();
	void		OnCancel();

	BookmarkFolder	*m_pAllBookmarks;
	HIMAGELIST		m_himlTreeView;

	HFONT			m_hEditSearchFont;
	bool			m_bSearchFieldBlank;
	bool			m_bEditingSearchField;

	CManageBookmarksDialogPersistentSettings	*m_pmbdps;
};

#endif