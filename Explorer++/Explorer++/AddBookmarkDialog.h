#ifndef ADDBOOKMARKDIALOG_INCLUDED
#define ADDBOOKMARKDIALOG_INCLUDED

#include <unordered_set>
#include "BookmarkHelper.h"
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

	bool							m_bInitialized;
	GUID							m_guidSelected;
	NBookmarkHelper::setExpansion_t	m_setExpansion;
};

class CAddBookmarkDialog : public CBaseDialog, public NBookmark::IBookmarkItemNotification
{
public:

	CAddBookmarkDialog(HINSTANCE hInstance,int iResource,HWND hParent,CBookmarkFolder &AllBookmarks,CBookmark &Bookmark);
	~CAddBookmarkDialog();

	LRESULT CALLBACK	TreeViewEditProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark);
	void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder);
	void	OnBookmarkModified(const GUID &guid);
	void	OnBookmarkFolderModified(const GUID &guid);
	void	OnBookmarkRemoved(const GUID &guid);
	void	OnBookmarkFolderRemoved(const GUID &guid);

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();
	BOOL	OnDestroy();
	BOOL	OnNcDestroy();

	void	SaveState();

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	CAddBookmarkDialog & operator = (const CAddBookmarkDialog &mbd);

	void		SetDialogIcon();

	void		OnRClick(NMHDR *pnmhdr);
	void		OnNewFolder();

	void		OnTvnBeginLabelEdit();
	BOOL		OnTvnEndLabelEdit(NMTVDISPINFO *pnmtvdi);
	void		OnTvnKeyDown(NMTVKEYDOWN *pnmtvkd);

	void		OnTreeViewRename();

	void		OnOk();
	void		OnCancel();

	void		SaveTreeViewState();
	void		SaveTreeViewExpansionState(HWND hTreeView,HTREEITEM hItem);

	HICON			m_hDialogIcon;

	CBookmarkFolder	&m_AllBookmarks;
	CBookmark		&m_Bookmark;

	CBookmarkTreeView	*m_pBookmarkTreeView;

	bool		m_bNewFolderCreated;
	GUID		m_NewFolderGUID;

	CAddBookmarkDialogPersistentSettings	*m_pabdps;
};

#endif