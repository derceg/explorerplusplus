// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SearchDialog.h"
#include "BrowserWindow.h"
#include "CoreInterface.h"
#include "DialogConstants.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/ComboBox.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellContextMenu.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/XMLSettings.h"
#include <algorithm>
#include <regex>

namespace NSearchDialog
{
const int WM_APP_SEARCHITEMFOUND = WM_APP + 1;
const int WM_APP_SEARCHFINISHED = WM_APP + 2;
const int WM_APP_SEARCHCHANGEDDIRECTORY = WM_APP + 3;
const int WM_APP_REGULAREXPRESSIONINVALID = WM_APP + 4;

int CALLBACK SortResultsStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

DWORD WINAPI SearchThread(LPVOID pParam);
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
}

const TCHAR SearchDialogPersistentSettings::SETTINGS_KEY[] = _T("Search");

const TCHAR SearchDialogPersistentSettings::SETTING_COLUMN_WIDTH_1[] = _T("ColumnWidth1");
const TCHAR SearchDialogPersistentSettings::SETTING_COLUMN_WIDTH_2[] = _T("ColumnWidth2");
const TCHAR SearchDialogPersistentSettings::SETTING_SEARCH_DIRECTORY_TEXT[] =
	_T("SearchDirectoryText");
const TCHAR SearchDialogPersistentSettings::SETTING_SEARCH_SUB_FOLDERS[] = _T("SearchSubFolders");
const TCHAR SearchDialogPersistentSettings::SETTING_USE_REGULAR_EXPRESSIONS[] =
	_T("UseRegularExpressions");
const TCHAR SearchDialogPersistentSettings::SETTING_CASE_INSENSITIVE[] = _T("CaseInsensitive");
const TCHAR SearchDialogPersistentSettings::SETTING_ARCHIVE[] = _T("Archive");
const TCHAR SearchDialogPersistentSettings::SETTING_HIDDEN[] = _T("Hidden");
const TCHAR SearchDialogPersistentSettings::SETTING_READ_ONLY[] = _T("ReadOnly");
const TCHAR SearchDialogPersistentSettings::SETTING_SYSTEM[] = _T("System");
const TCHAR SearchDialogPersistentSettings::SETTING_SORT_MODE[] = _T("SortMode");
const TCHAR SearchDialogPersistentSettings::SETTING_SORT_ASCENDING[] = _T("SortAscending");
const TCHAR SearchDialogPersistentSettings::SETTING_DIRECTORY_LIST[] = _T("Directory");
const TCHAR SearchDialogPersistentSettings::SETTING_PATTERN_LIST[] = _T("Pattern");

SearchDialog::SearchDialog(HINSTANCE resourceInstance, HWND hParent, ThemeManager *themeManager,
	std::wstring_view searchDirectory, BrowserWindow *browserWindow, CoreInterface *coreInterface,
	TabContainer *tabContainer, const IconResourceLoader *iconResourceLoader) :
	ThemedDialog(resourceInstance, IDD_SEARCH, hParent, DialogSizingType::Both, themeManager),
	m_searchDirectory(searchDirectory),
	m_browserWindow(browserWindow),
	m_coreInterface(coreInterface),
	m_tabContainer(tabContainer),
	m_iconResourceLoader(iconResourceLoader),
	m_bSearching(FALSE),
	m_bStopSearching(FALSE),
	m_pSearch(nullptr),
	m_iInternalIndex(0),
	m_iPreviousSelectedColumn(-1),
	m_bSetSearchTimer(TRUE)
{
	m_persistentSettings = &SearchDialogPersistentSettings::GetInstance();
}

SearchDialog::~SearchDialog()
{
	if (m_pSearch != nullptr)
	{
		m_pSearch->StopSearching();
		m_pSearch->Release();
	}
}

INT_PTR SearchDialog::OnInitDialog()
{
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hDlg);
	m_directoryIcon = m_iconResourceLoader->LoadIconFromPNGForDpi(Icon::Folder, 16, 16, dpi);
	SendMessage(GetDlgItem(m_hDlg, IDC_BUTTON_DIRECTORY), BM_SETIMAGE, IMAGE_ICON,
		reinterpret_cast<LPARAM>(m_directoryIcon.get()));

	HWND hListView = GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS);

	ListView_SetExtendedListViewStyleEx(hListView, LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER,
		LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(nullptr, &himlSmall);
	ListView_SetImageList(hListView, himlSmall, LVSIL_SMALL);

	int i = 0;

	for (const auto &ci : m_persistentSettings->m_Columns)
	{
		auto columnName = ResourceHelper::LoadString(GetResourceInstance(), ci.uStringID);

		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_TEXT;
		lvColumn.pszText = columnName.data();
		ListView_InsertColumn(hListView, i, &lvColumn);

		i++;
	}

	RECT rc;
	GetClientRect(hListView, &rc);

	ListView_SetColumnWidth(hListView, 0, (1.0 / 3.0) * GetRectWidth(&rc));
	ListView_SetColumnWidth(hListView, 1, (1.80 / 3.0) * GetRectWidth(&rc));

	UpdateListViewHeader();

	lCheckDlgButton(m_hDlg, IDC_CHECK_ARCHIVE, m_persistentSettings->m_bArchive);
	lCheckDlgButton(m_hDlg, IDC_CHECK_HIDDEN, m_persistentSettings->m_bHidden);
	lCheckDlgButton(m_hDlg, IDC_CHECK_READONLY, m_persistentSettings->m_bReadOnly);
	lCheckDlgButton(m_hDlg, IDC_CHECK_SYSTEM, m_persistentSettings->m_bSystem);
	lCheckDlgButton(m_hDlg, IDC_CHECK_SEARCHSUBFOLDERS, m_persistentSettings->m_bSearchSubFolders);
	lCheckDlgButton(m_hDlg, IDC_CHECK_CASEINSENSITIVE, m_persistentSettings->m_bCaseInsensitive);
	lCheckDlgButton(m_hDlg, IDC_CHECK_USEREGULAREXPRESSIONS,
		m_persistentSettings->m_bUseRegularExpressions);

	for (const auto &strDirectory : m_persistentSettings->m_searchDirectories)
	{
		SendDlgItemMessage(m_hDlg, IDC_COMBO_DIRECTORY, CB_INSERTSTRING, static_cast<WPARAM>(-1),
			reinterpret_cast<LPARAM>(strDirectory.c_str()));
	}

	for (const auto &strPattern : m_persistentSettings->m_searchPatterns)
	{
		SendDlgItemMessage(m_hDlg, IDC_COMBO_NAME, CB_INSERTSTRING, static_cast<WPARAM>(-1),
			reinterpret_cast<LPARAM>(strPattern.c_str()));
	}

	SetDlgItemText(m_hDlg, IDC_COMBO_NAME, m_persistentSettings->m_searchPattern.c_str());
	SetDlgItemText(m_hDlg, IDC_COMBO_DIRECTORY, m_searchDirectory.c_str());

	ComboBox::CreateNew(GetDlgItem(m_hDlg, IDC_COMBO_NAME));
	ComboBox::CreateNew(GetDlgItem(m_hDlg, IDC_COMBO_DIRECTORY));

	if (m_persistentSettings->m_bStateSaved)
	{
		/* These dummy values will be in use if these values
		have not previously been saved. */
		if (m_persistentSettings->m_iColumnWidth1 != -1
			&& m_persistentSettings->m_iColumnWidth2 != -1)
		{
			ListView_SetColumnWidth(hListView, 0, m_persistentSettings->m_iColumnWidth1);
			ListView_SetColumnWidth(hListView, 1, m_persistentSettings->m_iColumnWidth2);
		}
	}

	SetFocus(GetDlgItem(m_hDlg, IDC_COMBO_NAME));

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return FALSE;
}

wil::unique_hicon SearchDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_iconResourceLoader->LoadIconFromPNGAndScale(Icon::Search, iconWidth, iconHeight);
}

std::vector<ResizableDialogControl> SearchDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_COMBO_NAME), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_COMBO_DIRECTORY), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BUTTON_DIRECTORY), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_STATIC_STATUSLABEL), MovingType::Vertical,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_STATIC_STATUS), MovingType::Vertical,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_LINK_STATUS), MovingType::Vertical,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_STATIC_ETCHEDHORZ), MovingType::Vertical,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDSEARCH), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDEXIT), MovingType::Both, SizingType::None);
	return controls;
}

INT_PTR SearchDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDSEARCH:
		OnSearch();
		break;

	case IDC_BUTTON_DIRECTORY:
	{
		BROWSEINFO bi;
		TCHAR szDirectory[MAX_PATH];
		TCHAR szDisplayName[MAX_PATH];

		auto title = ResourceHelper::LoadString(GetResourceInstance(), IDS_SEARCHDIALOG_TITLE);

		GetDlgItemText(m_hDlg, IDC_COMBO_DIRECTORY, szDirectory, SIZEOF_ARRAY(szDirectory));

		bi.hwndOwner = m_hDlg;
		bi.pidlRoot = nullptr;
		bi.pszDisplayName = szDisplayName;
		bi.lpszTitle = title.c_str();
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		bi.lpfn = NSearchDialog::BrowseCallbackProc;
		bi.lParam = reinterpret_cast<LPARAM>(szDirectory);
		unique_pidl_absolute pidl(SHBrowseForFolder(&bi));

		if (pidl != nullptr)
		{
			std::wstring parsingPath;
			GetDisplayName(pidl.get(), SHGDN_FORPARSING, parsingPath);
			SetDlgItemText(m_hDlg, IDC_COMBO_DIRECTORY, parsingPath.c_str());
		}
	}
	break;

	case IDEXIT:
		DestroyWindow(m_hDlg);
		break;

	case IDCANCEL:
		DestroyWindow(m_hDlg);
		break;
	}

	return 0;
}

int CALLBACK NSearchDialog::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	UNREFERENCED_PARAMETER(lParam);

	assert(lpData != NULL);

	auto *szSearchPattern = reinterpret_cast<TCHAR *>(lpData);

	switch (uMsg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(szSearchPattern));
		break;
	}

	return 0;
}

void SearchDialog::OnSearch()
{
	if (!m_bSearching)
	{
		StartSearching();
	}
	else
	{
		StopSearching();
	}
}

void SearchDialog::StartSearching()
{
	ShowWindow(GetDlgItem(m_hDlg, IDC_LINK_STATUS), SW_HIDE);
	ShowWindow(GetDlgItem(m_hDlg, IDC_STATIC_STATUS), SW_SHOW);

	m_AwaitingSearchItems.clear();
	m_SearchItemsMapInternal.clear();

	ListView_DeleteAllItems(GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS));

	TCHAR szBaseDirectory[MAX_PATH];
	TCHAR szSearchPattern[MAX_PATH];

	/* Get the directory and name, and remove leading and
	trailing whitespace. */
	GetDlgItemText(m_hDlg, IDC_COMBO_DIRECTORY, szBaseDirectory, SIZEOF_ARRAY(szBaseDirectory));
	PathRemoveBlanks(szBaseDirectory);
	GetDlgItemText(m_hDlg, IDC_COMBO_NAME, szSearchPattern, SIZEOF_ARRAY(szSearchPattern));
	PathRemoveBlanks(szSearchPattern);

	BOOL bSearchSubFolders = IsDlgButtonChecked(m_hDlg, IDC_CHECK_SEARCHSUBFOLDERS) == BST_CHECKED;

	BOOL bUseRegularExpressions =
		IsDlgButtonChecked(m_hDlg, IDC_CHECK_USEREGULAREXPRESSIONS) == BST_CHECKED;

	BOOL bCaseInsensitive = IsDlgButtonChecked(m_hDlg, IDC_CHECK_CASEINSENSITIVE) == BST_CHECKED;

	/* Turn search patterns of the form '???' into '*???*', and
	use this modified string to search. */
	if (!bUseRegularExpressions && lstrlen(szSearchPattern) > 0)
	{
		if (szSearchPattern[0] != '*' && szSearchPattern[lstrlen(szSearchPattern) - 1] != '*')
		{
			TCHAR szTemp[MAX_PATH];

			StringCchPrintf(szTemp, SIZEOF_ARRAY(szTemp), _T("*%s*"), szSearchPattern);
			StringCchCopy(szSearchPattern, std::size(szSearchPattern), szTemp);
		}
	}

	DWORD dwAttributes = 0;

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_ARCHIVE) == BST_CHECKED)
	{
		dwAttributes |= FILE_ATTRIBUTE_ARCHIVE;
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_HIDDEN) == BST_CHECKED)
	{
		dwAttributes |= FILE_ATTRIBUTE_HIDDEN;
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_READONLY) == BST_CHECKED)
	{
		dwAttributes |= FILE_ATTRIBUTE_READONLY;
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_SYSTEM) == BST_CHECKED)
	{
		dwAttributes |= FILE_ATTRIBUTE_SYSTEM;
	}

	m_pSearch = new Search(m_hDlg, szBaseDirectory, szSearchPattern, dwAttributes,
		bUseRegularExpressions, bCaseInsensitive, bSearchSubFolders);
	m_pSearch->AddRef();

	/* Save the search directory and search pattern (only if they are not
	the same as the most recent entry). */
	BOOL bSaveEntry = FALSE;

	if (m_persistentSettings->m_searchDirectories.empty()
		|| lstrcmp(szBaseDirectory, m_persistentSettings->m_searchDirectories.begin()->c_str())
			!= 0)
	{
		bSaveEntry = TRUE;
	}

	if (bSaveEntry)
	{
		SaveEntry(IDC_COMBO_DIRECTORY, m_persistentSettings->m_searchDirectories);
	}

	bSaveEntry = FALSE;

	if (m_persistentSettings->m_searchPatterns.empty()
		|| lstrcmp(szSearchPattern, m_persistentSettings->m_searchPatterns.begin()->c_str()) != 0)
	{
		bSaveEntry = TRUE;
	}

	if (bSaveEntry)
	{
		SaveEntry(IDC_COMBO_NAME, m_persistentSettings->m_searchPatterns);
	}

	GetDlgItemText(m_hDlg, IDSEARCH, m_szSearchButton, SIZEOF_ARRAY(m_szSearchButton));

	auto stopText = ResourceHelper::LoadString(GetResourceInstance(), IDS_STOP);
	SetDlgItemText(m_hDlg, IDSEARCH, stopText.c_str());

	m_bSearching = TRUE;

	/* Create a background thread, and search using it... */
	HANDLE hThread = CreateThread(nullptr, 0, NSearchDialog::SearchThread,
		reinterpret_cast<LPVOID>(m_pSearch), 0, nullptr);
	CloseHandle(hThread);
}

void SearchDialog::SaveEntry(int comboBoxId, boost::circular_buffer<std::wstring> &buffer)
{
	TCHAR entry[MAX_PATH];
	GetDlgItemText(m_hDlg, comboBoxId, entry, SIZEOF_ARRAY(entry));

	std::wstring strEntry(entry);
	auto itr = std::find_if(buffer.begin(), buffer.end(),
		[strEntry](const std::wstring &Pattern) { return Pattern == strEntry; });

	HWND hComboBox = GetDlgItem(m_hDlg, comboBoxId);
	ComboBox_SetCurSel(hComboBox, -1);

	if (itr != buffer.end())
	{
		/* Remove the current element from both the list and the
		combo box. It will be reinserted at the front of both below. */
		auto index = std::distance(buffer.begin(), itr);
		SendMessage(hComboBox, CB_DELETESTRING, index, 0);

		buffer.erase(itr);
	}

	buffer.push_front(entry);

	SendMessage(hComboBox, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(entry));
	ComboBox_SetCurSel(hComboBox, 0);
	ComboBox_SetEditSel(hComboBox, -1, -1);

	if (ComboBox_GetCount(hComboBox) > buffer.capacity())
	{
		SendMessage(hComboBox, CB_DELETESTRING, ComboBox_GetCount(hComboBox) - 1, 0);
	}
}

void SearchDialog::StopSearching()
{
	m_bStopSearching = TRUE;

	if (m_pSearch != nullptr)
	{
		/* Note that m_pSearch does not need to be
		released here. Once the search object finishes,
		it will send a WM_APP_SEARCHFINISHED message.
		The handler for this message will then release
		m_pSearch. */
		m_pSearch->StopSearching();
	}
}

void SearchDialog::UpdateListViewHeader()
{
	HWND hHeader = ListView_GetHeader(GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS));

	HDITEM hdItem;

	/* Remove the sort arrow from the column that was
	previously selected. */
	if (m_iPreviousSelectedColumn != -1)
	{
		hdItem.mask = HDI_FORMAT;
		Header_GetItem(hHeader, m_iPreviousSelectedColumn, &hdItem);

		if (hdItem.fmt & HDF_SORTUP)
		{
			hdItem.fmt &= ~HDF_SORTUP;
		}
		else if (hdItem.fmt & HDF_SORTDOWN)
		{
			hdItem.fmt &= ~HDF_SORTDOWN;
		}

		Header_SetItem(hHeader, m_iPreviousSelectedColumn, &hdItem);
	}

	int iColumn = 0;

	for (const auto &ci : m_persistentSettings->m_Columns)
	{
		if (ci.sortMode == m_persistentSettings->m_SortMode)
		{
			break;
		}

		iColumn++;
	}

	hdItem.mask = HDI_FORMAT;
	Header_GetItem(hHeader, iColumn, &hdItem);

	if (m_persistentSettings->m_bSortAscending)
	{
		hdItem.fmt |= HDF_SORTUP;
	}
	else
	{
		hdItem.fmt |= HDF_SORTDOWN;
	}

	Header_SetItem(hHeader, iColumn, &hdItem);

	m_iPreviousSelectedColumn = iColumn;
}

int CALLBACK NSearchDialog::SortResultsStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	assert(lParamSort != NULL);

	auto *psd = reinterpret_cast<SearchDialog *>(lParamSort);

	return psd->SortResults(lParam1, lParam2);
}

int CALLBACK SearchDialog::SortResults(LPARAM lParam1, LPARAM lParam2)
{
	int iRes = 0;

	switch (m_persistentSettings->m_SortMode)
	{
	case SearchDialogPersistentSettings::SortMode::Name:
		iRes = SortResultsByName(lParam1, lParam2);
		break;

	case SearchDialogPersistentSettings::SortMode::Path:
		iRes = SortResultsByPath(lParam1, lParam2);
		break;
	}

	if (!m_persistentSettings->m_bSortAscending)
	{
		iRes = -iRes;
	}

	return iRes;
}

int CALLBACK SearchDialog::SortResultsByName(LPARAM lParam1, LPARAM lParam2)
{
	auto itr1 = m_SearchItemsMapInternal.find(static_cast<int>(lParam1));
	auto itr2 = m_SearchItemsMapInternal.find(static_cast<int>(lParam2));

	TCHAR szFilename1[MAX_PATH];
	TCHAR szFilename2[MAX_PATH];

	StringCchCopy(szFilename1, std::size(szFilename1), itr1->second.c_str());
	StringCchCopy(szFilename2, std::size(szFilename2), itr2->second.c_str());

	PathStripPath(szFilename1);
	PathStripPath(szFilename2);

	return StrCmpLogicalW(szFilename1, szFilename2);
}

int CALLBACK SearchDialog::SortResultsByPath(LPARAM lParam1, LPARAM lParam2)
{
	auto itr1 = m_SearchItemsMapInternal.find(static_cast<int>(lParam1));
	auto itr2 = m_SearchItemsMapInternal.find(static_cast<int>(lParam2));

	TCHAR szPath1[MAX_PATH];
	TCHAR szPath2[MAX_PATH];

	StringCchCopy(szPath1, std::size(szPath1), itr1->second.c_str());
	StringCchCopy(szPath2, std::size(szPath2), itr2->second.c_str());

	PathRemoveFileSpec(szPath1);
	PathRemoveFileSpec(szPath2);

	return StrCmpLogicalW(szPath1, szPath2);
}

void SearchDialog::UpdateMenuEntries(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, IContextMenu *contextMenu)
{
	UNREFERENCED_PARAMETER(contextMenu);

	unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0].Raw()));
	SFGAOF itemAttributes = SFGAO_FOLDER;
	GetItemAttributes(pidlComplete.get(), &itemAttributes);

	std::wstring openLocationText;

	if ((itemAttributes & SFGAO_FOLDER) == SFGAO_FOLDER)
	{
		openLocationText =
			ResourceHelper::LoadString(GetResourceInstance(), IDS_SEARCH_OPEN_FOLDER_LOCATION);
	}
	else
	{
		openLocationText =
			ResourceHelper::LoadString(GetResourceInstance(), IDS_SEARCH_OPEN_FILE_LOCATION);
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STRING | MIIM_ID;
	mii.wID = OPEN_FILE_LOCATION_MENU_ITEM_ID;
	mii.dwTypeData = openLocationText.data();
	InsertMenuItem(menu, 1, TRUE, &mii);
}

std::wstring SearchDialog::GetHelpTextForItem(UINT menuItemId)
{
	switch (menuItemId)
	{
	case OPEN_FILE_LOCATION_MENU_ITEM_ID:
		return ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
			IDS_SEARCH_OPEN_ITEM_LOCATION_HELP_TEXT);

	default:
		DCHECK(false);
		return L"";
	}
}

bool SearchDialog::HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, const std::wstring &verb)
{
	if (verb == L"open")
	{
		for (const auto &pidlItem : pidlItems)
		{
			unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItem.Raw()));
			m_browserWindow->OpenItem(pidlComplete.get());
		}

		return true;
	}

	return false;
}

void SearchDialog::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, UINT menuItemId)
{
	switch (menuItemId)
	{
	case OPEN_FILE_LOCATION_MENU_ITEM_ID:
	{
		auto navigateParams = NavigateParams::Normal(pidlParent);
		m_tabContainer->CreateNewTab(navigateParams, TabSettings(_selected = true));

		unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0].Raw()));
		m_coreInterface->GetActiveShellBrowserImpl()->SelectItems({ pidlComplete.get() });
	}
	break;
	}
}

INT_PTR SearchDialog::OnNotify(NMHDR *pnmhdr)
{
	switch (pnmhdr->code)
	{
	case NM_DBLCLK:
		if (pnmhdr->hwndFrom == GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS))
		{
			HWND hListView = GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS);
			int iSelected = ListView_GetNextItem(hListView, -1, LVNI_ALL | LVNI_SELECTED);

			if (iSelected != -1)
			{
				LVITEM lvItem;

				lvItem.mask = LVIF_PARAM;
				lvItem.iItem = iSelected;
				lvItem.iSubItem = 0;

				BOOL bRet = ListView_GetItem(hListView, &lvItem);

				if (bRet)
				{
					auto itr = m_SearchItemsMapInternal.find(static_cast<int>(lvItem.lParam));
					CHECK(itr != m_SearchItemsMapInternal.end());

					m_browserWindow->OpenItem(itr->second.c_str());
				}
			}
		}
		break;

	case NM_CLICK:
	{
		if (pnmhdr->hwndFrom == GetDlgItem(m_hDlg, IDC_LINK_STATUS))
		{
			auto pnmlink = reinterpret_cast<PNMLINK>(pnmhdr);

			ShellExecute(nullptr, L"open", pnmlink->item.szUrl, nullptr, nullptr, SW_SHOWNORMAL);
		}
	}
	break;

	case NM_RCLICK:
	{
		if (pnmhdr->hwndFrom == GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS))
		{
			HWND hListView = GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS);
			int iSelected = ListView_GetNextItem(hListView, -1, LVNI_ALL | LVNI_SELECTED);

			if (iSelected != -1)
			{
				LVITEM lvItem;
				lvItem.mask = LVIF_PARAM;
				lvItem.iItem = iSelected;
				lvItem.iSubItem = 0;
				BOOL bRet = ListView_GetItem(hListView, &lvItem);

				if (bRet)
				{
					auto itr = m_SearchItemsMapInternal.find(static_cast<int>(lvItem.lParam));
					CHECK(itr != m_SearchItemsMapInternal.end());

					unique_pidl_absolute pidlFull;
					HRESULT hr = SHParseDisplayName(itr->second.c_str(), nullptr,
						wil::out_param(pidlFull), 0, nullptr);

					if (hr == S_OK)
					{
						// The only reason this pidl is cloned at all is that ILFindLastID returns
						// an unaligned pointer. Inserting that into the pidlItems vector then
						// triggers a warning due to the underlying types having different
						// __unaligned qualifiers. This only affects Itanium (which isn't
						// supported), but cloning the pidl here is a simple way of producing an
						// aligned version.
						unique_pidl_child pidlItem(ILCloneChild(ILFindLastID(pidlFull.get())));

						std::vector<PCITEMID_CHILD> pidlItems;
						pidlItems.push_back(pidlItem.get());

						unique_pidl_absolute pidlDirectory(ILCloneFull(pidlFull.get()));
						ILRemoveLastID(pidlDirectory.get());

						ShellContextMenu shellContextMenu(pidlDirectory.get(), pidlItems, this,
							m_coreInterface->GetStatusBar());

						DWORD dwCursorPos = GetMessagePos();

						POINT ptCursor;
						ptCursor.x = GET_X_LPARAM(dwCursorPos);
						ptCursor.y = GET_Y_LPARAM(dwCursorPos);

						ShellContextMenu::Flags flags = ShellContextMenu::Flags::Standard;

						if (IsKeyDown(VK_SHIFT))
						{
							WI_SetFlag(flags, ShellContextMenu::Flags::ExtendedVerbs);
						}

						shellContextMenu.ShowMenu(m_hDlg, &ptCursor, nullptr, flags);
					}
				}
			}
		}
	}
	break;

	case LVN_COLUMNCLICK:
	{
		/* A listview header has been clicked,
		so sort by that column. */
		auto *pnmlv = reinterpret_cast<NMLISTVIEW *>(pnmhdr);

		/* If the column clicked matches the current sort mode,
		flip the sort direction, else switch to that sort mode. */
		if (m_persistentSettings->m_Columns[pnmlv->iSubItem].sortMode
			== m_persistentSettings->m_SortMode)
		{
			m_persistentSettings->m_bSortAscending = !m_persistentSettings->m_bSortAscending;
		}
		else
		{
			m_persistentSettings->m_SortMode =
				m_persistentSettings->m_Columns[pnmlv->iSubItem].sortMode;
			m_persistentSettings->m_bSortAscending =
				m_persistentSettings->m_Columns[pnmlv->iSubItem].bSortAscending;
		}

		ListView_SortItems(GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS),
			NSearchDialog::SortResultsStub, reinterpret_cast<LPARAM>(this));

		UpdateListViewHeader();
	}
	break;
	}

	return 0;
}

INT_PTR SearchDialog::OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	/* We won't actually process the item here. Instead, we'll
	add it onto the list of current items, which will be processed
	in batch. This is done to stop this message from blocking the
	main GUI (also see http://www.flounder.com/iocompletion.htm). */
	case NSearchDialog::WM_APP_SEARCHITEMFOUND:
	{
		m_AwaitingSearchItems.push_back(reinterpret_cast<PIDLIST_ABSOLUTE>(wParam));

		if (m_bSetSearchTimer)
		{
			SetTimer(m_hDlg, SEARCH_PROCESSITEMS_TIMER_ID, SEARCH_PROCESSITEMS_TIMER_ELAPSED,
				nullptr);

			m_bSetSearchTimer = FALSE;
		}
	}
	break;

	case NSearchDialog::WM_APP_SEARCHFINISHED:
	{
		TCHAR szStatus[512];

		if (!m_bStopSearching)
		{
			int iFoldersFound = LOWORD(lParam);
			int iFilesFound = HIWORD(lParam);

			TCHAR szTemp[128];
			LoadString(GetResourceInstance(), IDS_SEARCH_FINISHED_MESSAGE, szTemp,
				SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szStatus, SIZEOF_ARRAY(szStatus), szTemp, iFoldersFound, iFilesFound);
			SetDlgItemText(m_hDlg, IDC_STATIC_STATUS, szStatus);
		}
		else
		{
			auto cancelledMessage =
				ResourceHelper::LoadString(GetResourceInstance(), IDS_SEARCH_CANCELLED_MESSAGE);
			SetDlgItemText(m_hDlg, IDC_STATIC_STATUS, cancelledMessage.c_str());
		}

		assert(m_pSearch != nullptr);

		m_pSearch->Release();
		m_pSearch = nullptr;

		m_bSearching = FALSE;
		m_bStopSearching = FALSE;
		SetDlgItemText(m_hDlg, IDSEARCH, m_szSearchButton);
	}
	break;

	case NSearchDialog::WM_APP_SEARCHCHANGEDDIRECTORY:
	{
		TCHAR szStatus[512];
		TCHAR *pszDirectory = nullptr;

		pszDirectory = reinterpret_cast<TCHAR *>(wParam);

		TCHAR szTemp[64];
		LoadString(GetResourceInstance(), IDS_SEARCHING, szTemp, SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szStatus, SIZEOF_ARRAY(szStatus), szTemp, pszDirectory);
		SetDlgItemText(m_hDlg, IDC_STATIC_STATUS, szStatus);
	}
	break;

	case NSearchDialog::WM_APP_REGULAREXPRESSIONINVALID:
	{
		/* The link/status controls are in the same position, and
		have the same size. If one of the controls is showing text,
		the other should not be visible. */
		ShowWindow(GetDlgItem(m_hDlg, IDC_LINK_STATUS), SW_SHOW);
		ShowWindow(GetDlgItem(m_hDlg, IDC_STATIC_STATUS), SW_HIDE);

		/* The regular expression passed to the search
		thread was invalid. Show the user an error message. */
		auto errorMessage = ResourceHelper::LoadString(GetResourceInstance(),
			IDS_SEARCH_REGULAR_EXPRESSION_INVALID);
		SetDlgItemText(m_hDlg, IDC_LINK_STATUS, errorMessage.c_str());

		assert(m_pSearch != nullptr);

		m_pSearch->Release();
		m_pSearch = nullptr;

		m_bSearching = FALSE;
		m_bStopSearching = FALSE;
		SetDlgItemText(m_hDlg, IDSEARCH, m_szSearchButton);
	}
	break;
	}

	return 0;
}

INT_PTR SearchDialog::OnTimer(int iTimerID)
{
	if (iTimerID != SEARCH_PROCESSITEMS_TIMER_ID)
	{
		return 1;
	}

	HWND hListView = GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS);
	int nListViewItems = ListView_GetItemCount(hListView);

	int nItems =
		std::min(static_cast<int>(m_AwaitingSearchItems.size()), SEARCH_MAX_ITEMS_BATCH_PROCESS);
	int i = 0;

	auto itr = m_AwaitingSearchItems.begin();

	while (i < nItems)
	{
		LVITEM lvItem;
		SHFILEINFO shfi;
		int iIndex;

		PIDLIST_ABSOLUTE pidl = *itr;

		std::wstring fullFileName;
		GetDisplayName(pidl, SHGDN_FORPARSING, fullFileName);

		TCHAR directory[MAX_PATH];
		StringCchCopy(directory, std::size(directory), fullFileName.c_str());
		PathRemoveFileSpec(directory);

		std::wstring fileName;
		GetDisplayName(pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, fileName);

		SHGetFileInfo((LPCWSTR) pidl, 0, &shfi, sizeof(shfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);

		m_SearchItemsMapInternal.insert(
			std::unordered_map<int, std::wstring>::value_type(m_iInternalIndex, fullFileName));

		lvItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
		lvItem.pszText = fileName.data();
		lvItem.iItem = nListViewItems + i;
		lvItem.iSubItem = 0;
		lvItem.iImage = shfi.iIcon;
		lvItem.lParam = m_iInternalIndex++;
		iIndex = ListView_InsertItem(hListView, &lvItem);

		ListView_SetItemText(hListView, iIndex, 1, directory);

		CoTaskMemFree(pidl);

		itr = m_AwaitingSearchItems.erase(itr);

		i++;
	}

	m_bSetSearchTimer = TRUE;

	return 0;
}

INT_PTR SearchDialog::OnClose()
{
	DestroyWindow(m_hDlg);

	return 0;
}

INT_PTR SearchDialog::OnNcDestroy()
{
	delete this;

	return 0;
}

DWORD WINAPI NSearchDialog::SearchThread(LPVOID pParam)
{
	assert(pParam != nullptr);

	auto *pSearch = reinterpret_cast<Search *>(pParam);
	pSearch->StartSearching();

	return 0;
}

Search::Search(HWND hDlg, TCHAR *szBaseDirectory, TCHAR *szPattern, DWORD dwAttributes,
	BOOL bUseRegularExpressions, BOOL bCaseInsensitive, BOOL bSearchSubFolders)
{
	m_hDlg = hDlg;
	m_dwAttributes = dwAttributes;
	m_bUseRegularExpressions = bUseRegularExpressions;
	m_bCaseInsensitive = bCaseInsensitive;
	m_bSearchSubFolders = bSearchSubFolders;

	StringCchCopy(m_szBaseDirectory, std::size(m_szBaseDirectory), szBaseDirectory);
	StringCchCopy(m_szSearchPattern, std::size(m_szSearchPattern), szPattern);

	InitializeCriticalSection(&m_csStop);
	m_bStopSearching = FALSE;
}

Search::~Search()
{
	DeleteCriticalSection(&m_csStop);
}

void Search::StartSearching()
{
	m_iFoldersFound = 0;
	m_iFilesFound = 0;

	if (lstrlen(m_szSearchPattern) != 0 && m_bUseRegularExpressions)
	{
		try
		{
			if (m_bCaseInsensitive)
			{
				m_rxPattern.assign(m_szSearchPattern, std::regex_constants::icase);
			}
			else
			{
				m_rxPattern.assign(m_szSearchPattern);
			}
		}
		catch (std::exception)
		{
			SendMessage(m_hDlg, NSearchDialog::WM_APP_REGULAREXPRESSIONINVALID, 0, 0);

			return;
		}
	}

	SearchDirectory(m_szBaseDirectory);

	SendMessage(m_hDlg, NSearchDialog::WM_APP_SEARCHFINISHED, 0,
		MAKELPARAM(m_iFoldersFound, m_iFilesFound));

	Release();
}

void Search::SearchDirectory(const TCHAR *szDirectory)
{
	SendMessage(m_hDlg, NSearchDialog::WM_APP_SEARCHCHANGEDDIRECTORY,
		reinterpret_cast<WPARAM>(szDirectory), 0);

	std::list<std::wstring> subFolderList;

	SearchDirectoryInternal(szDirectory, &subFolderList);

	bool bStop = false;

	EnterCriticalSection(&m_csStop);
	if (m_bStopSearching)
	{
		bStop = true;
	}
	LeaveCriticalSection(&m_csStop);

	if (bStop)
	{
		return;
	}

	if (m_bSearchSubFolders)
	{
		for (const auto &strSubFolder : subFolderList)
		{
			SearchDirectory(strSubFolder.c_str());
		}
	}
}

/* Can't recurse, as it would overflow the stack. */
void Search::SearchDirectoryInternal(const TCHAR *szSearchDirectory,
	std::list<std::wstring> *pSubFolderList)
{
	assert(szSearchDirectory != nullptr);
	assert(pSubFolderList != nullptr);

	WIN32_FIND_DATA wfd;
	TCHAR szSearchTerm[MAX_PATH];

	PathCombine(szSearchTerm, szSearchDirectory, _T("*"));

	HANDLE hFindFile = FindFirstFile(szSearchTerm, &wfd);

	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		bool bStop = false;

		while (FindNextFile(hFindFile, &wfd) != 0 && !bStop)
		{
			if (lstrcmpi(wfd.cFileName, _T(".")) != 0 && lstrcmpi(wfd.cFileName, _T("..")) != 0)
			{
				BOOL bMatchFileName = FALSE;
				BOOL bMatchAttributes = FALSE;

				/* Only match against the filename if it's not empty. */
				if (lstrlen(m_szSearchPattern) != 0)
				{
					if (m_bUseRegularExpressions)
					{
						if (std::regex_match(wfd.cFileName, m_rxPattern))
						{
							bMatchFileName = TRUE;
						}
					}
					else
					{
						if (CheckWildcardMatch(m_szSearchPattern, wfd.cFileName,
								!m_bCaseInsensitive))
						{
							bMatchFileName = TRUE;
						}
					}
				}
				else
				{
					/* No filename constraint, so all filenames match. */
					bMatchFileName = TRUE;
				}

				if (m_dwAttributes != 0)
				{
					if ((wfd.dwFileAttributes & m_dwAttributes) == m_dwAttributes)
					{
						bMatchAttributes = TRUE;
					}
				}
				else
				{
					bMatchAttributes = TRUE;
				}

				BOOL bItemMatch = bMatchFileName && bMatchAttributes;

				if (bItemMatch)
				{
					if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						== FILE_ATTRIBUTE_DIRECTORY)
					{
						m_iFoldersFound++;
					}
					else
					{
						m_iFilesFound++;
					}

					TCHAR szFullFileName[MAX_PATH];
					PathCombine(szFullFileName, szSearchDirectory, wfd.cFileName);

					unique_pidl_absolute pidl;
					SHParseDisplayName(szFullFileName, nullptr, wil::out_param(pidl), 0, nullptr);

					PostMessage(m_hDlg, NSearchDialog::WM_APP_SEARCHITEMFOUND,
						reinterpret_cast<WPARAM>(ILCloneFull(pidl.get())), 0);
				}

				if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
				{
					TCHAR szSubFolder[MAX_PATH];

					PathCombine(szSubFolder, szSearchDirectory, wfd.cFileName);

					pSubFolderList->push_back(szSubFolder);
				}
			}

			EnterCriticalSection(&m_csStop);
			if (m_bStopSearching)
			{
				bStop = true;
			}
			LeaveCriticalSection(&m_csStop);
		}

		FindClose(hFindFile);
	}
}

void Search::StopSearching()
{
	EnterCriticalSection(&m_csStop);
	m_bStopSearching = TRUE;
	LeaveCriticalSection(&m_csStop);
}

void SearchDialog::SaveState()
{
	HWND hListView;

	m_persistentSettings->SaveDialogPosition(m_hDlg);

	m_persistentSettings->m_bCaseInsensitive =
		IsDlgButtonChecked(m_hDlg, IDC_CHECK_CASEINSENSITIVE) == BST_CHECKED;

	m_persistentSettings->m_bUseRegularExpressions =
		IsDlgButtonChecked(m_hDlg, IDC_CHECK_USEREGULAREXPRESSIONS) == BST_CHECKED;

	m_persistentSettings->m_bSearchSubFolders =
		IsDlgButtonChecked(m_hDlg, IDC_CHECK_SEARCHSUBFOLDERS) == BST_CHECKED;

	m_persistentSettings->m_bArchive = IsDlgButtonChecked(m_hDlg, IDC_CHECK_ARCHIVE) == BST_CHECKED;

	m_persistentSettings->m_bHidden = IsDlgButtonChecked(m_hDlg, IDC_CHECK_HIDDEN) == BST_CHECKED;

	m_persistentSettings->m_bReadOnly =
		IsDlgButtonChecked(m_hDlg, IDC_CHECK_READONLY) == BST_CHECKED;

	m_persistentSettings->m_bSystem = IsDlgButtonChecked(m_hDlg, IDC_CHECK_SYSTEM) == BST_CHECKED;

	hListView = GetDlgItem(m_hDlg, IDC_LISTVIEW_SEARCHRESULTS);

	m_persistentSettings->m_iColumnWidth1 = ListView_GetColumnWidth(hListView, 0);
	m_persistentSettings->m_iColumnWidth2 = ListView_GetColumnWidth(hListView, 1);

	m_persistentSettings->m_searchPattern = GetDlgItemString(m_hDlg, IDC_COMBO_NAME);

	m_persistentSettings->m_bStateSaved = TRUE;
}

SearchDialogPersistentSettings::SearchDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY),
	m_searchPatterns(DialogConstants::DEFAULT_HISTORY_SIZE),
	m_searchDirectories(DialogConstants::DEFAULT_HISTORY_SIZE)
{
	m_bSearchSubFolders = TRUE;
	m_bUseRegularExpressions = FALSE;
	m_bCaseInsensitive = FALSE;
	m_bArchive = FALSE;
	m_bHidden = FALSE;
	m_bReadOnly = FALSE;
	m_bSystem = FALSE;
	m_iColumnWidth1 = -1;
	m_iColumnWidth2 = -1;

	ColumnInfo ci;
	ci.sortMode = SortMode::Name;
	ci.uStringID = IDS_SEARCH_COLUMN_NAME;
	ci.bSortAscending = true;
	m_Columns.push_back(ci);

	ci.sortMode = SortMode::Path;
	ci.uStringID = IDS_SEARCH_COLUMN_PATH;
	ci.bSortAscending = true;
	m_Columns.push_back(ci);

	m_SortMode = m_Columns.front().sortMode;
	m_bSortAscending = m_Columns.front().bSortAscending;
}

SearchDialogPersistentSettings &SearchDialogPersistentSettings::GetInstance()
{
	static SearchDialogPersistentSettings sdps;
	return sdps;
}

void SearchDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::SaveDword(hKey, SETTING_COLUMN_WIDTH_1, m_iColumnWidth1);
	RegistrySettings::SaveDword(hKey, SETTING_COLUMN_WIDTH_2, m_iColumnWidth2);
	RegistrySettings::SaveString(hKey, SETTING_SEARCH_DIRECTORY_TEXT, m_searchPattern);
	RegistrySettings::SaveDword(hKey, SETTING_SEARCH_SUB_FOLDERS, m_bSearchSubFolders);
	RegistrySettings::SaveDword(hKey, SETTING_USE_REGULAR_EXPRESSIONS, m_bUseRegularExpressions);
	RegistrySettings::SaveDword(hKey, SETTING_CASE_INSENSITIVE, m_bCaseInsensitive);
	RegistrySettings::SaveDword(hKey, SETTING_ARCHIVE, m_bArchive);
	RegistrySettings::SaveDword(hKey, SETTING_HIDDEN, m_bHidden);
	RegistrySettings::SaveDword(hKey, SETTING_READ_ONLY, m_bReadOnly);
	RegistrySettings::SaveDword(hKey, SETTING_SYSTEM, m_bSystem);
	RegistrySettings::SaveDword(hKey, SETTING_SORT_MODE, static_cast<DWORD>(m_SortMode));
	RegistrySettings::SaveDword(hKey, SETTING_SORT_ASCENDING, m_bSortAscending);

	std::list<std::wstring> searchDirectoriesList;
	CircularBufferToList(m_searchDirectories, searchDirectoriesList);
	RegistrySettings::SaveStringList(hKey, SETTING_DIRECTORY_LIST, searchDirectoriesList);

	std::list<std::wstring> searchPatternList;
	CircularBufferToList(m_searchPatterns, searchPatternList);
	RegistrySettings::SaveStringList(hKey, SETTING_PATTERN_LIST, searchPatternList);
}

void SearchDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_COLUMN_WIDTH_1, m_iColumnWidth1);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_COLUMN_WIDTH_2, m_iColumnWidth2);
	RegistrySettings::ReadString(hKey, SETTING_SEARCH_DIRECTORY_TEXT, m_searchPattern);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_SEARCH_SUB_FOLDERS,
		m_bSearchSubFolders);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_USE_REGULAR_EXPRESSIONS,
		m_bUseRegularExpressions);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_CASE_INSENSITIVE,
		m_bCaseInsensitive);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_ARCHIVE, m_bArchive);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_HIDDEN, m_bHidden);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_READ_ONLY, m_bReadOnly);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_SYSTEM, m_bSystem);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_SORT_ASCENDING, m_bSortAscending);

	DWORD value;
	RegistrySettings::ReadDword(hKey, SETTING_SORT_MODE, value);
	m_SortMode = static_cast<SortMode>(value);

	std::list<std::wstring> searchDirectoriesList;
	RegistrySettings::ReadStringList(hKey, SETTING_DIRECTORY_LIST, searchDirectoriesList);
	ListToCircularBuffer(searchDirectoriesList, m_searchDirectories);

	std::list<std::wstring> searchPatternList;
	RegistrySettings::ReadStringList(hKey, SETTING_PATTERN_LIST, searchPatternList);
	ListToCircularBuffer(searchPatternList, m_searchPatterns);
}

void SearchDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_COLUMN_WIDTH_1,
		XMLSettings::EncodeIntValue(m_iColumnWidth1));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_COLUMN_WIDTH_2,
		XMLSettings::EncodeIntValue(m_iColumnWidth2));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SEARCH_DIRECTORY_TEXT,
		m_searchPattern.c_str());
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SEARCH_SUB_FOLDERS,
		XMLSettings::EncodeBoolValue(m_bSearchSubFolders));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_USE_REGULAR_EXPRESSIONS,
		XMLSettings::EncodeBoolValue(m_bUseRegularExpressions));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_CASE_INSENSITIVE,
		XMLSettings::EncodeBoolValue(m_bCaseInsensitive));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_ARCHIVE,
		XMLSettings::EncodeBoolValue(m_bArchive));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_HIDDEN,
		XMLSettings::EncodeBoolValue(m_bHidden));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_READ_ONLY,
		XMLSettings::EncodeBoolValue(m_bReadOnly));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SYSTEM,
		XMLSettings::EncodeBoolValue(m_bSystem));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SORT_MODE,
		XMLSettings::EncodeIntValue(static_cast<int>(m_SortMode)));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SORT_ASCENDING,
		XMLSettings::EncodeBoolValue(m_bSortAscending));

	std::list<std::wstring> searchDirectoriesList;
	CircularBufferToList(m_searchDirectories, searchDirectoriesList);
	XMLSettings::AddStringListToNode(pXMLDom, pParentNode, SETTING_DIRECTORY_LIST,
		searchDirectoriesList);

	std::list<std::wstring> searchPatternList;
	CircularBufferToList(m_searchPatterns, searchPatternList);
	XMLSettings::AddStringListToNode(pXMLDom, pParentNode, SETTING_PATTERN_LIST, searchPatternList);
}

void SearchDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	if (lstrcmpi(bstrName, SETTING_COLUMN_WIDTH_1) == 0)
	{
		m_iColumnWidth1 = XMLSettings::DecodeIntValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_COLUMN_WIDTH_2) == 0)
	{
		m_iColumnWidth2 = XMLSettings::DecodeIntValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_SEARCH_DIRECTORY_TEXT) == 0)
	{
		m_searchPattern = bstrValue;
	}
	else if (lstrcmpi(bstrName, SETTING_SEARCH_SUB_FOLDERS) == 0)
	{
		m_bSearchSubFolders = XMLSettings::DecodeBoolValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_USE_REGULAR_EXPRESSIONS) == 0)
	{
		m_bUseRegularExpressions = XMLSettings::DecodeBoolValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_CASE_INSENSITIVE) == 0)
	{
		m_bCaseInsensitive = XMLSettings::DecodeBoolValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_ARCHIVE) == 0)
	{
		m_bArchive = XMLSettings::DecodeBoolValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_HIDDEN) == 0)
	{
		m_bHidden = XMLSettings::DecodeBoolValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_READ_ONLY) == 0)
	{
		m_bReadOnly = XMLSettings::DecodeBoolValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_SYSTEM) == 0)
	{
		m_bSystem = XMLSettings::DecodeBoolValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_SORT_MODE) == 0)
	{
		m_SortMode = static_cast<SortMode>(XMLSettings::DecodeIntValue(bstrValue));
	}
	else if (lstrcmpi(bstrName, SETTING_SORT_ASCENDING) == 0)
	{
		m_bSortAscending = XMLSettings::DecodeBoolValue(bstrValue);
	}
	else if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, bstrName,
				 lstrlen(SETTING_DIRECTORY_LIST), SETTING_DIRECTORY_LIST,
				 lstrlen(SETTING_DIRECTORY_LIST))
		== CSTR_EQUAL)
	{
		m_searchDirectories.push_back(bstrValue);
	}
	else if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, bstrName,
				 lstrlen(SETTING_PATTERN_LIST), SETTING_PATTERN_LIST, lstrlen(SETTING_PATTERN_LIST))
		== CSTR_EQUAL)
	{
		m_searchPatterns.push_back(bstrValue);
	}
}

template <typename T>
void SearchDialogPersistentSettings::CircularBufferToList(const boost::circular_buffer<T> &cb,
	std::list<T> &list)
{
	for (const auto &item : cb)
	{
		list.push_back(item);
	}
}

template <typename T>
void SearchDialogPersistentSettings::ListToCircularBuffer(const std::list<T> &list,
	boost::circular_buffer<T> &cb)
{
	for (const auto &item : list)
	{
		cb.push_back(item);
	}
}
