// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "DpiCompatibility.h"
#include "TabContainer.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/ReferenceCount.h"
#include <boost/circular_buffer.hpp>
#include <wil/resource.h>
#include <MsXml2.h>
#include <objbase.h>
#include <list>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

class CSearchDialog;

/* Manages settings for the search dialog. */
class CSearchDialogPersistentSettings : public CDialogSettings
{
public:

	~CSearchDialogPersistentSettings();

	static CSearchDialogPersistentSettings &GetInstance();

private:

	friend CSearchDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_COLUMN_WIDTH_1[];
	static const TCHAR SETTING_COLUMN_WIDTH_2[];
	static const TCHAR SETTING_SEARCH_DIRECTORY_TEXT[];
	static const TCHAR SETTING_SEARCH_SUB_FOLDERS[];
	static const TCHAR SETTING_USE_REGULAR_EXPRESSIONS[];
	static const TCHAR SETTING_CASE_INSENSITIVE[];
	static const TCHAR SETTING_ARCHIVE[];
	static const TCHAR SETTING_HIDDEN[];
	static const TCHAR SETTING_READ_ONLY[];
	static const TCHAR SETTING_SYSTEM[];
	static const TCHAR SETTING_DIRECTORY_LIST[];
	static const TCHAR SETTING_PATTERN_LIST[];
	static const TCHAR SETTING_SORT_MODE[];
	static const TCHAR SETTING_SORT_ASCENDING[];

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

	void						SaveExtraRegistrySettings(HKEY hKey);
	void						LoadExtraRegistrySettings(HKEY hKey);

	void						SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode);
	void						LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	template <typename T> void	CircularBufferToList(const boost::circular_buffer<T> &cb,std::list<T> &list);
	template <typename T> void	ListToCircularBuffer(const std::list<T> &list,boost::circular_buffer<T> &cb);

	TCHAR						m_szSearchPattern[MAX_PATH];
	boost::circular_buffer<std::wstring>	*m_pSearchPatterns;
	boost::circular_buffer<std::wstring>	*m_pSearchDirectories;
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

	CSearchDialog(HINSTANCE hInstance, int iResource, HWND hParent, TCHAR *szSearchDirectory,
		IExplorerplusplus *pexpp, TabContainer *tabContainer);
	~CSearchDialog();

	/* IFileContextMenuExternal methods. */
	void			AddMenuEntries(LPCITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu);
	BOOL			HandleShellMenuItem(LPCITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,const TCHAR *szCmd);
	void			HandleCustomMenuItem(LPCITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,int iCmd);

	/* Sorting methods. */
	int CALLBACK	SortResults(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK	SortResultsByName(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK	SortResultsByPath(LPARAM lParam1,LPARAM lParam2);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnTimer(int iTimerID);
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();
	INT_PTR	OnNcDestroy();

	INT_PTR	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

	static const int SEARCH_PROCESSITEMS_TIMER_ID = 0;
	static const int SEARCH_PROCESSITEMS_TIMER_ELAPSED = 50;
	static const int SEARCH_MAX_ITEMS_BATCH_PROCESS = 100;

	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const int MENU_ID_OPEN_FILE_LOCATION = (MAX_SHELL_MENU_ID + 1);

	void						GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void						SaveState();

	void						OnSearch();
	void						StartSearching();
	void						StopSearching();
	void						SaveEntry(int comboBoxId, boost::circular_buffer<std::wstring> &buffer);
	void						UpdateListViewHeader();

	TCHAR m_szSearchDirectory[MAX_PATH];
	DpiCompatibility m_dpiCompat;
	wil::unique_hicon m_icon;
	wil::unique_hicon m_directoryIcon;
	BOOL m_bSearching;
	BOOL m_bStopSearching;
	TCHAR m_szSearchButton[32];

	CSearch *m_pSearch;

	/* Listview item information. */
	std::list<LPITEMIDLIST> m_AwaitingSearchItems;
	std::unordered_map<int,std::wstring> m_SearchItemsMapInternal;
	int m_iInternalIndex;
	int m_iPreviousSelectedColumn;

	BOOL m_bSetSearchTimer;

	IExplorerplusplus *m_pexpp;
	TabContainer *m_tabContainer;

	CSearchDialogPersistentSettings *m_sdps;
};