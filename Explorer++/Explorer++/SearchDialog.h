// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/ReferenceCount.h"
#include <boost/circular_buffer.hpp>
#include <MsXml2.h>
#include <objbase.h>
#include <list>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

__interface IExplorerplusplus;
class SearchDialog;
class TabContainer;

/* Manages settings for the search dialog. */
class SearchDialogPersistentSettings : public DialogSettings
{
public:
	static SearchDialogPersistentSettings &GetInstance();

private:
	friend SearchDialog;

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

	enum class SortMode
	{
		Name = 1,
		Path = 2
	};

	struct ColumnInfo
	{
		SortMode sortMode;
		UINT uStringID;

		/* Indicates whether the sort direction
		for this column should be ascending by
		default. */
		bool bSortAscending;
	};

	SearchDialogPersistentSettings();

	SearchDialogPersistentSettings(const SearchDialogPersistentSettings &);
	SearchDialogPersistentSettings &operator=(const SearchDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey) override;
	void LoadExtraRegistrySettings(HKEY hKey) override;

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode) override;
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue) override;

	template <typename T>
	void CircularBufferToList(const boost::circular_buffer<T> &cb, std::list<T> &list);
	template <typename T>
	void ListToCircularBuffer(const std::list<T> &list, boost::circular_buffer<T> &cb);

	TCHAR m_szSearchPattern[MAX_PATH];
	boost::circular_buffer<std::wstring> m_searchPatterns;
	boost::circular_buffer<std::wstring> m_searchDirectories;
	BOOL m_bSearchSubFolders;
	BOOL m_bUseRegularExpressions;
	BOOL m_bCaseInsensitive;
	BOOL m_bArchive;
	BOOL m_bHidden;
	BOOL m_bReadOnly;
	BOOL m_bSystem;

	std::vector<ColumnInfo> m_Columns;
	SortMode m_SortMode;
	BOOL m_bSortAscending;

	int m_iColumnWidth1;
	int m_iColumnWidth2;
};

class Search : public ReferenceCount
{
public:
	Search(HWND hDlg, TCHAR *szBaseDirectory, TCHAR *szPattern, DWORD dwAttributes,
		BOOL bUseRegularExpressions, BOOL bCaseInsensitive, BOOL bSearchSubFolders);
	~Search();

	void StartSearching();
	void StopSearching();

private:
	void SearchDirectory(const TCHAR *szDirectory);
	void SearchDirectoryInternal(
		const TCHAR *szSearchDirectory, std::list<std::wstring> *pSubFolderList);

	HWND m_hDlg;

	TCHAR m_szBaseDirectory[MAX_PATH];
	TCHAR m_szSearchPattern[MAX_PATH + 2];
	DWORD m_dwAttributes;
	BOOL m_bUseRegularExpressions;
	BOOL m_bCaseInsensitive;
	BOOL m_bSearchSubFolders;

	std::wregex m_rxPattern;

	CRITICAL_SECTION m_csStop;
	BOOL m_bStopSearching;

	int m_iFoldersFound;
	int m_iFilesFound;
};

class SearchDialog : public BaseDialog, public IFileContextMenuExternal
{
public:
	SearchDialog(HINSTANCE hInstance, HWND hParent, std::wstring_view searchDirectory,
		IExplorerplusplus *pexpp, TabContainer *tabContainer);
	~SearchDialog();

	/* IFileContextMenuExternal methods. */
	void AddMenuEntries(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems,
		DWORD_PTR dwData, HMENU hMenu) override;
	BOOL HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, int iCmd) override;

	/* Sorting methods. */
	int CALLBACK SortResults(LPARAM lParam1, LPARAM lParam2);
	int CALLBACK SortResultsByName(LPARAM lParam1, LPARAM lParam2);
	int CALLBACK SortResultsByPath(LPARAM lParam1, LPARAM lParam2);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnTimer(int iTimerID) override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;
	INT_PTR OnNcDestroy() override;

	INT_PTR OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	static const int SEARCH_PROCESSITEMS_TIMER_ID = 0;
	static const int SEARCH_PROCESSITEMS_TIMER_ELAPSED = 50;
	static const int SEARCH_MAX_ITEMS_BATCH_PROCESS = 100;

	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const int MENU_ID_OPEN_FILE_LOCATION = (MAX_SHELL_MENU_ID + 1);

	void GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
		std::list<ResizableDialog::Control_t> &ControlList) override;
	void SaveState() override;

	void OnSearch();
	void StartSearching();
	void StopSearching();
	void SaveEntry(int comboBoxId, boost::circular_buffer<std::wstring> &buffer);
	void UpdateListViewHeader();

	std::wstring m_searchDirectory;
	wil::unique_hicon m_directoryIcon;
	BOOL m_bSearching;
	BOOL m_bStopSearching;
	TCHAR m_szSearchButton[32];

	Search *m_pSearch;

	/* Listview item information. */
	std::list<PIDLIST_ABSOLUTE> m_AwaitingSearchItems;
	std::unordered_map<int, std::wstring> m_SearchItemsMapInternal;
	int m_iInternalIndex;
	int m_iPreviousSelectedColumn;

	BOOL m_bSetSearchTimer;

	IExplorerplusplus *m_pexpp;
	TabContainer *m_tabContainer;

	SearchDialogPersistentSettings *m_persistentSettings;
};