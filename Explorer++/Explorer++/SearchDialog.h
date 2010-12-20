#ifndef SEARCHDIALOG_INCLUDED
#define SEARCHDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include <unordered_map>

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
	BOOL	OnTimer(int iTimerID);
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

	void	OnSearch();
	void	SearchSaveState(HWND hDlg);

	HWND						m_hGripper;
	TCHAR						m_szSearchDirectory[MAX_PATH];
	TCHAR						m_szSearchPattern[MAX_PATH];
	HICON						m_hDialogIcon;
	HICON						m_hDirectoryIcon;
	BOOL						m_bSearching;
	BOOL						m_bStopSearching;
	TCHAR						m_szSearchButton[32];
	BOOL						m_bExit;

	/* Listview item information. */
	/* TODO: These are AWAITING search items. */
	list<LPITEMIDLIST>			m_SearchItems;
	unordered_map<int,wstring>	m_SearchItemsMapInternal;
	int							m_iInternalIndex;

	int							m_iMinWidth;
	int							m_iMinHeight;

	BOOL						m_bSetSearchTimer;

	/* Used when resizing. */
	int							m_iListViewWidthDelta;
	int							m_iListViewHeightDelta;
	int							m_iSearchDirectoryWidthDelta;
	int							m_iNamedWidthDelta;
	int							m_iButtonDirectoryLeftDelta;
	int							m_iEtchedHorzWidthDelta;
	int							m_iEtchedHorzVerticalDelta;
	int							m_iExitLeftDelta;
	int							m_iExitVerticalDelta;
	int							m_iSearchExitDelta;
	int							m_iStaticStatusWidthDelta;
	int							m_iStatusVerticalDelta;
};

class CSearch
{
public:
	
	CSearch(HWND hDlg,TCHAR *szBaseDirectory,TCHAR *szPattern,DWORD dwAttributes,BOOL bUseRegularExpressions,BOOL bCaseInsensitive,BOOL bSearchSubFolders);
	~CSearch();

	void				StartSearch();
	void				SetStopSearching();

private:

	void				SearchDirectory(const TCHAR *szDirectory);
	void				SearchDirectoryInternal(const TCHAR *szSearchDirectory,list<wstring> *pSubFolderList);

	HWND				m_hDlg;

	TCHAR				m_szBaseDirectory[MAX_PATH];
	TCHAR				m_szSearchPattern[MAX_PATH + 2];
	DWORD				m_dwAttributes;
	BOOL				m_bUseRegularExpressions;
	BOOL				m_bCaseInsensitive;
	BOOL				m_bSearchSubFolders;

	CRITICAL_SECTION	m_csStop;
	BOOL				m_bStopSearching;

	int					m_iFoldersFound;
	int					m_iFilesFound;
};

#endif