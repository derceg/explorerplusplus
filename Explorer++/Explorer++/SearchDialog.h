#ifndef SEARCHDIALOG_INCLUDED
#define SEARCHDIALOG_INCLUDED

#include <list>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include "../Helper/BaseDialog.h"
#include "../Helper/ReferenceCount.h"
#include "../Helper/FileContextMenuManager.h"

class CSearchDialog;

/* Manages settings for the search dialog. */
class CSearchDialogPersistentSettings
{
public:

	~CSearchDialogPersistentSettings();

	static CSearchDialogPersistentSettings &GetInstance();

	/* Registry save/load settings. */
	void	SaveSettings(HKEY hParentKey);
	void	LoadSettings(HKEY hParentKey);

	/* XML save/load settings. */
	void	SaveSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe);
	void	LoadSettings(MSXML2::IXMLDOMNamedNodeMap *pam,long lChildNodes);

private:

	friend CSearchDialog;

	static const TCHAR REGISTRY_SETTINGS_KEY[];

	CSearchDialogPersistentSettings();

	CSearchDialogPersistentSettings(const CSearchDialogPersistentSettings &);
	CSearchDialogPersistentSettings & operator=(const CSearchDialogPersistentSettings &);

	/* TODO: Move to a base class? */
	BOOL			m_bStateSaved;

	TCHAR			m_szSearchPattern[MAX_PATH];
	std::list<std::wstring>	m_SearchDirectories;
	std::list<std::wstring>	m_SearchPatterns;
	BOOL			m_bSearchSubFolders;
	BOOL			m_bUseRegularExpressions;
	BOOL			m_bCaseInsensitive;
	BOOL			m_bArchive;
	BOOL			m_bHidden;
	BOOL			m_bReadOnly;
	BOOL			m_bSystem;

	/* Dialog/control size properties. */
	POINT			m_ptSearch;
	int				m_iSearchWidth;
	int				m_iSearchHeight;
	int				m_iColumnWidth1;
	int				m_iColumnWidth2;
};

class CSearch : public CReferenceCount
{
public:
	
	CSearch(HWND hDlg,TCHAR *szBaseDirectory,TCHAR *szPattern,DWORD dwAttributes,BOOL bUseRegularExpressions,BOOL bCaseInsensitive,BOOL bSearchSubFolders);
	~CSearch();

	void				StartSearching();
	void				StopSearching();

private:

	void				SearchDirectory(const TCHAR *szDirectory);
	void				SearchDirectoryInternal(const TCHAR *szSearchDirectory,std::list<std::wstring> *pSubFolderList);

	HWND				m_hDlg;

	TCHAR				m_szBaseDirectory[MAX_PATH];
	TCHAR				m_szSearchPattern[MAX_PATH + 2];
	DWORD				m_dwAttributes;
	BOOL				m_bUseRegularExpressions;
	BOOL				m_bCaseInsensitive;
	BOOL				m_bSearchSubFolders;

	std::wregex			m_rxPattern;

	CRITICAL_SECTION	m_csStop;
	BOOL				m_bStopSearching;

	int					m_iFoldersFound;
	int					m_iFilesFound;
};

class CSearchDialog : public CBaseDialog, public IFileContextMenuExternal
{
public:

	CSearchDialog(HINSTANCE hInstance,int iResource,HWND hParent,TCHAR *szSearchDirectory);
	~CSearchDialog();

	/* IFileContextMenuExternal methods. */
	void			AddMenuEntries(LPITEMIDLIST pidlParent,std::list<LPITEMIDLIST> pidlItemList,DWORD_PTR dwData,HMENU hMenu);
	BOOL			HandleShellMenuItem(LPITEMIDLIST pidlParent,std::list<LPITEMIDLIST> pidlItemList,DWORD_PTR dwData,TCHAR *szCmd);
	void			HandleCustomMenuItem(LPITEMIDLIST pidlParent,std::list<LPITEMIDLIST> pidlItemList,int iCmd);

	/* Sorting methods. */
	int CALLBACK	SortResults(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK	SortResultsByName(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK	SortResultsByPath(LPARAM lParam1,LPARAM lParam2);

protected:

	BOOL	OnInitDialog();
	BOOL	OnTimer(int iTimerID);
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnGetMinMaxInfo(LPMINMAXINFO pmmi);
	BOOL	OnSize(int iType,int iWidth,int iHeight);
	BOOL	OnClose();
	BOOL	OnDestroy();
	BOOL	OnNcDestroy();

	void	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

	static const int SEARCH_PROCESSITEMS_TIMER_ID = 0;
	static const int SEARCH_PROCESSITEMS_TIMER_ELAPSED = 50;
	static const int SEARCH_MAX_ITEMS_BATCH_PROCESS = 100;

	/* Available search modes. */
	enum SortMode_t
	{
		SORT_NAME,
		SORT_PATH
	};

	struct ColumnInfo_t
	{
		SortMode_t	SearchMode;
	};

	void	OnSearch();
	void	SaveState(HWND hDlg);

	CSearchDialogPersistentSettings	*m_sdps;

	HWND						m_hGripper;
	TCHAR						m_szSearchDirectory[MAX_PATH];
	HICON						m_hDialogIcon;
	HICON						m_hDirectoryIcon;
	BOOL						m_bSearching;
	BOOL						m_bStopSearching;
	TCHAR						m_szSearchButton[32];
	BOOL						m_bExit;

	/* Search data. */
	CSearch						*m_pSearch;

	/* Listview item information. */
	/* TODO: These are AWAITING search items. */
	std::list<LPITEMIDLIST>		m_SearchItems;
	std::unordered_map<int,std::wstring>	m_SearchItemsMapInternal;
	int							m_iInternalIndex;
	std::vector<ColumnInfo_t>	m_Columns;
	SortMode_t					m_SortMode;
	BOOL						m_bSortAscending;

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

#endif