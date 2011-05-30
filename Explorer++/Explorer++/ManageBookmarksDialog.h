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

	enum ColumnType_t
	{
		COLUMN_TYPE_NAME = 1,
		COLUMN_TYPE_LOCATION = 2,
		COLUMN_TYPE_VISIT_DATE = 3,
		COLUMN_TYPE_VISIT_COUNT = 4,
		COLUMN_TYPE_ADDED = 5,
		COLUMN_TYPE_LAST_MODIFIED = 6
	};

	struct ColumnInfo_t
	{
		ColumnType_t	ColumnType;
		int				iWidth;
		bool			bActive;
	};

	static const TCHAR SETTINGS_KEY[];
	static const int DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH = 180;

	CManageBookmarksDialogPersistentSettings();

	CManageBookmarksDialogPersistentSettings(const CManageBookmarksDialogPersistentSettings &);
	CManageBookmarksDialogPersistentSettings & operator=(const CManageBookmarksDialogPersistentSettings &);

	void SetupDefaultColumns();

	std::vector<ColumnInfo_t>	m_vectorColumnInfo;
};

class CManageBookmarksDialog : public CBaseDialog
{
public:

	CManageBookmarksDialog(HINSTANCE hInstance,int iResource,HWND hParent,CBookmarkFolder *pAllBookmarks);
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

	void		SetupToolbar();
	void		SetupTreeView();
	void		SetupListView();

	void		GetColumnString(CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,UINT cchBuf);

	void		SetSearchFieldDefaultState();
	void		RemoveSearchFieldDefaultState();

	void		GetBookmarkItemFromListView(int iItem);

	void		OnEnChange(HWND hEdit);
	void		OnDblClk(NMHDR *pnmhdr);
	void		OnRClick(NMHDR *pnmhdr);
	void		OnListViewHeaderRClick();
	void		OnTvnSelChanged(NMTREEVIEW *pnmtv);

	void		OnOk();
	void		OnCancel();

	HWND			m_hToolbar;
	HIMAGELIST		m_himlToolbar;

	CBookmarkFolder	*m_pAllBookmarks;
	HIMAGELIST		m_himlTreeView;

	HFONT			m_hEditSearchFont;
	bool			m_bSearchFieldBlank;
	bool			m_bEditingSearchField;

	CManageBookmarksDialogPersistentSettings	*m_pmbdps;
};

#endif