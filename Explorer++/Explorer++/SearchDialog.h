#ifndef SEARCHDIALOG_INCLUDED
#define SEARCHDIALOG_INCLUDED

#include <list>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <boost/circular_buffer.hpp>
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ReferenceCount.h"
#include "../Helper/FileContextMenuManager.h"

#import <msxml3.dll> raw_interfaces_only

class CSearchDialog;

/* Manages settings for the search dialog. */
class CSearchDialogPersistentSettings : public CDialogSettings
{
public:

	~CSearchDialogPersistentSettings();

	static CSearchDialogPersistentSettings &GetInstance();

protected:

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue);

private:

	friend CSearchDialog;

	static const TCHAR SETTINGS_KEY[];

	enum SortMode_t
	{
		SORT_NAME = 1,
		SORT_PATH = 2
	};

	struct ColumnInfo_t
	{
		SortMode_t	SortMode;
		UINT		uStringID;

		/* Indicates whether the sort direction
		for this column should be ascending by
		default. */
		bool		bSortAscending;
	};

	CSearchDialogPersistentSettings();

	CSearchDialogPersistentSettings(const CSearchDialogPersistentSettings &);
	CSearchDialogPersistentSettings & operator=(const CSearchDialogPersistentSettings &);

	template <typename T> void	CircularBufferToList(const boost::circular_buffer<T> &cb,std::list<T> &list);
	template <typename T> void	ListToCircularBuffer(const std::list<T> &list,boost::circular_buffer<T> &cb);

	TCHAR						m_szSearchPattern[MAX_PATH];
	std::list<std::wstring>		m_SearchDirectories;
	boost::circular_buffer<std::wstring>	*m_pSearchPatterns;
	BOOL						m_bSearchSubFolders;
	BOOL						m_bUseRegularExpressions;
	BOOL						m_bCaseInsensitive;
	BOOL						m_bArchive;
	BOOL						m_bHidden;
	BOOL						m_bReadOnly;
	BOOL						m_bSystem;

	std::vector<ColumnInfo_t>	m_Columns;
	SortMode_t					m_SortMode;
	BOOL						m_bSortAscending;

	int							m_iColumnWidth1;
	int							m_iColumnWidth2;
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

	CSearchDialog(HINSTANCE hInstance,int iResource,HWND hParent,TCHAR *szSearchDirectory,IExplorerplusplus *pexpp);
	~CSearchDialog();

	/* IFileContextMenuExternal methods. */
	void			AddMenuEntries(LPITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu);
	BOOL			HandleShellMenuItem(LPITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,TCHAR *szCmd);
	void			HandleCustomMenuItem(LPITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,int iCmd);

	/* Sorting methods. */
	int CALLBACK	SortResults(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK	SortResultsByName(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK	SortResultsByPath(LPARAM lParam1,LPARAM lParam2);

protected:

	BOOL	OnInitDialog();
	BOOL	OnTimer(int iTimerID);
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();
	BOOL	OnNcDestroy();

	void	SaveState();

	void	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	static const int SEARCH_PROCESSITEMS_TIMER_ID = 0;
	static const int SEARCH_PROCESSITEMS_TIMER_ELAPSED = 50;
	static const int SEARCH_MAX_ITEMS_BATCH_PROCESS = 100;

	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const int MENU_ID_OPEN_FILE_LOCATION = (MAX_SHELL_MENU_ID + 1);

	void						OnSearch();
	void						UpdateListViewHeader();

	TCHAR						m_szSearchDirectory[MAX_PATH];
	HICON						m_hDialogIcon;
	HICON						m_hDirectoryIcon;
	BOOL						m_bSearching;
	BOOL						m_bStopSearching;
	TCHAR						m_szSearchButton[32];

	CSearch						*m_pSearch;

	/* Listview item information. */
	std::list<LPITEMIDLIST>		m_AwaitingSearchItems;
	std::unordered_map<int,std::wstring>	m_SearchItemsMapInternal;
	int							m_iInternalIndex;
	int							m_iPreviousSelectedColumn;

	BOOL						m_bSetSearchTimer;

	IExplorerplusplus			*m_pexpp;

	CSearchDialogPersistentSettings	*m_sdps;
};

#endif