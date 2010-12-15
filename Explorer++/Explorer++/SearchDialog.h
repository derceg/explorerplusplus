#ifndef SEARCHDIALOG_INCLUDED
#define SEARCHDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"

/* TODO: */
#define WM_APP_SEARCHITEMFOUND	 (WM_APP + 1)
#define WM_APP_SEARCHFINISHED	(WM_APP + 2)
#define WM_APP_SEARCHCHANGEDDIRECTORY	(WM_APP + 3)

class CSearchDialog : public CBaseDialog
{
public:

	CSearchDialog(HINSTANCE hInstance,int iResource,HWND hParent,TCHAR *szSearchDirectory);
	~CSearchDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnGetMinMaxInfo(LPMINMAXINFO pmmi);
	BOOL	OnSize(int iType,int iWidth,int iHeight);
	BOOL	OnClose();
	BOOL	OnNcDestroy();

	void	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

	/*static const int WM_APP_SEARCHITEMFOUND = WM_APP + 1;
	static const int WM_APP_SEARCHFINISHED = WM_APP + 2;
	static const int WM_APP_SEARCHCHANGEDDIRECTORY = WM_APP + 3;*/

	void	OnSearch(HWND hDlg);
	void	SearchSaveState(HWND hDlg);

	HWND	m_hGripper;

	TCHAR	m_szSearchDirectory[MAX_PATH];
	TCHAR	m_szSearch[MAX_PATH];

	HICON	m_hDialogIcon;
	HICON	m_hDirectoryIcon;

	BOOL	m_bSearching;
	BOOL	m_bStopSearching;

	TCHAR	m_szSearchButton[32];

	int		m_iFoldersFound;
	int		m_iFilesFound;

	BOOL	m_bExit;

	int		m_iMinWidth;
	int		m_iMinHeight;

	/* Used when resizing. */
	int m_iListViewWidthDelta;
	int m_iListViewHeightDelta;
	int m_iSearchDirectoryWidthDelta;
	int m_iNamedWidthDelta;
	int m_iButtonDirectoryLeftDelta;
	int m_iEtchedHorzWidthDelta;
	int m_iEtchedHorzVerticalDelta;
	int m_iExitLeftDelta;
	int m_iExitVerticalDelta;
	int m_iSearchExitDelta;
	int m_iStaticStatusWidthDelta;
	int m_iStatusVerticalDelta;

	/* TODO: */
	BOOL					m_bSearchDlgStateSaved;
	BOOL					m_bSearchSubFolders;
	BOOL					m_bSetSearchTimer;
	POINT					m_ptSearch;
	int						m_iSearchWidth;
	int						m_iSearchHeight;
	int						m_iColumnWidth1;
	int						m_iColumnWidth2;
	TCHAR					m_SearchPatternText[MAX_PATH];
	list<SearchDirectoryInfo_t>	m_SearchDirectories;
	list<SearchPatternInfo_t>	m_SearchPatterns;
	list<LPITEMIDLIST>		m_SearchItems;
};

typedef struct
{
	CSearchDialog	*pSearchDialog;
} SearchInfo_t;

#endif