#ifndef ADDBOOKMARKDIALOG_INCLUDED
#define ADDBOOKMARKDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/Bookmark.h"

class CAddBookmarkDialog;

class CAddBookmarkDialogPersistentSettings : public CDialogSettings
{
public:

	~CAddBookmarkDialogPersistentSettings();

	static CAddBookmarkDialogPersistentSettings &GetInstance();

private:

	friend CAddBookmarkDialog;

	static const TCHAR SETTINGS_KEY[];

	CAddBookmarkDialogPersistentSettings();

	CAddBookmarkDialogPersistentSettings(const CAddBookmarkDialogPersistentSettings &);
	CAddBookmarkDialogPersistentSettings & operator=(const CAddBookmarkDialogPersistentSettings &);
};

class CAddBookmarkDialog : public CBaseDialog
{
public:

	CAddBookmarkDialog(HINSTANCE hInstance,int iResource,HWND hParent,BookmarkFolder *pAllBookmarks,Bookmark *pBookmark);
	~CAddBookmarkDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();
	BOOL	OnDestroy();

private:

	void		InsertFoldersIntoTreeView();
	HTREEITEM	InsertFolderIntoTreeView(HWND hTreeView,HTREEITEM hParent,BookmarkFolder *pBookmarkFolder);
	void		OnNewFolder();

	BookmarkFolder	*CAddBookmarkDialog::GetBookmarkFolderFromTreeView(HTREEITEM hItem);

	void		OnOk();
	void		OnCancel();

	BookmarkFolder	*m_pAllBookmarks;
	Bookmark		*m_pBookmark;
	HIMAGELIST		m_himlTreeView;

	CAddBookmarkDialogPersistentSettings	*m_pabdps;
};

#endif