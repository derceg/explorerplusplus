#ifndef SEARCHDIALOG_INCLUDED
#define SEARCHDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include <unordered_map>

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

	static const int SEARCH_PROCESSITEMS_TIMER_ID = 0;
	static const int SEARCH_PROCESSITEMS_TIMER_ELAPSED = 50;
	static const int SEARCH_MAX_ITEMS_BATCH_PROCESS = 100;

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

	list<wstring>				m_SearchDirectories;
	list<wstring>				m_SearchPatterns;

	BOOL						m_bSearchSubFolders;
	POINT						m_ptSearch;
	int							m_iSearchWidth;
	int							m_iSearchHeight;
	int							m_iColumnWidth1;
	int							m_iColumnWidth2;
	TCHAR						m_SearchPatternText[MAX_PATH];
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