// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SearchDialog.h"
#include "DialogHelper.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/ComboBox.h"
#include "../Helper/Controls.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/XMLSettings.h"
#include <regex>

namespace NSearchDialog
{
	const int		WM_APP_SEARCHITEMFOUND = WM_APP + 1;
	const int		WM_APP_SEARCHFINISHED = WM_APP + 2;
	const int		WM_APP_SEARCHCHANGEDDIRECTORY = WM_APP + 3;
	const int		WM_APP_REGULAREXPRESSIONINVALID = WM_APP + 4;

	int CALLBACK	SortResultsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);

	DWORD WINAPI	SearchThread(LPVOID pParam);
	int CALLBACK	BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);
}

const TCHAR CSearchDialogPersistentSettings::SETTINGS_KEY[] = _T("Search");

const TCHAR CSearchDialogPersistentSettings::SETTING_COLUMN_WIDTH_1[] = _T("ColumnWidth1");
const TCHAR CSearchDialogPersistentSettings::SETTING_COLUMN_WIDTH_2[] = _T("ColumnWidth2");
const TCHAR CSearchDialogPersistentSettings::SETTING_SEARCH_DIRECTORY_TEXT[] = _T("SearchDirectoryText");
const TCHAR CSearchDialogPersistentSettings::SETTING_SEARCH_SUB_FOLDERS[] = _T("SearchSubFolders");
const TCHAR CSearchDialogPersistentSettings::SETTING_USE_REGULAR_EXPRESSIONS[] = _T("UseRegularExpressions");
const TCHAR CSearchDialogPersistentSettings::SETTING_CASE_INSENSITIVE[] = _T("CaseInsensitive");
const TCHAR CSearchDialogPersistentSettings::SETTING_ARCHIVE[] = _T("Archive");
const TCHAR CSearchDialogPersistentSettings::SETTING_HIDDEN[] = _T("Hidden");
const TCHAR CSearchDialogPersistentSettings::SETTING_READ_ONLY[] = _T("ReadOnly");
const TCHAR CSearchDialogPersistentSettings::SETTING_SYSTEM[] = _T("System");
const TCHAR CSearchDialogPersistentSettings::SETTING_SORT_MODE[] = _T("SortMode");
const TCHAR CSearchDialogPersistentSettings::SETTING_SORT_ASCENDING[] = _T("SortAscending");
const TCHAR CSearchDialogPersistentSettings::SETTING_DIRECTORY_LIST[] = _T("Directory");
const TCHAR CSearchDialogPersistentSettings::SETTING_PATTERN_LIST[] = _T("Pattern");

CSearchDialog::CSearchDialog(HINSTANCE hInstance,int iResource,
	HWND hParent,TCHAR *szSearchDirectory,IExplorerplusplus *pexpp,
	TabContainer *tabContainer) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	StringCchCopy(m_szSearchDirectory,SIZEOF_ARRAY(m_szSearchDirectory),
		szSearchDirectory);

	m_pexpp = pexpp;
	m_tabContainer = tabContainer;

	m_bSearching		= FALSE;
	m_bStopSearching	= FALSE;
	m_bSetSearchTimer	= TRUE;
	m_iInternalIndex	= 0;
	m_iPreviousSelectedColumn	= -1;
	m_pSearch			= NULL;

	m_sdps = &CSearchDialogPersistentSettings::GetInstance();
}

CSearchDialog::~CSearchDialog()
{
	if(m_pSearch != NULL)
	{
		m_pSearch->StopSearching();
		m_pSearch->Release();
	}
}

INT_PTR CSearchDialog::OnInitDialog()
{
	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hDlg);
	m_icon = IconResourceLoader::LoadIconFromPNGForDpi(Icon::Search, DIALOG_ICON_SIZE_96DPI, dpi);
	SetClassLongPtr(m_hDlg, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_icon.get()));

	m_directoryIcon = IconResourceLoader::LoadIconFromPNGForDpi(Icon::Folder, 16, dpi);
	SendMessage(GetDlgItem(m_hDlg,IDC_BUTTON_DIRECTORY),BM_SETIMAGE,
		IMAGE_ICON,reinterpret_cast<LPARAM>(m_directoryIcon.get()));

	HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);

	ListView_SetExtendedListViewStyleEx(hListView,LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER,
		LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	SetWindowTheme(hListView,L"Explorer",NULL);

	int i = 0;

	for(const auto &ci : m_sdps->m_Columns)
	{
		TCHAR szTemp[128];
		LoadString(GetInstance(),ci.uStringID,szTemp,SIZEOF_ARRAY(szTemp));

		LVCOLUMN lvColumn;
		lvColumn.mask		= LVCF_TEXT;
		lvColumn.pszText	= szTemp;
		ListView_InsertColumn(hListView,i,&lvColumn);

		i++;
	}

	RECT rc;
	GetClientRect(hListView,&rc);

	ListView_SetColumnWidth(hListView,0,(1.0/3.0) * GetRectWidth(&rc));
	ListView_SetColumnWidth(hListView,1,(1.80/3.0) * GetRectWidth(&rc));

	UpdateListViewHeader();

	lCheckDlgButton(m_hDlg,IDC_CHECK_ARCHIVE,m_sdps->m_bArchive);
	lCheckDlgButton(m_hDlg,IDC_CHECK_HIDDEN,m_sdps->m_bHidden);
	lCheckDlgButton(m_hDlg,IDC_CHECK_READONLY,m_sdps->m_bReadOnly);
	lCheckDlgButton(m_hDlg,IDC_CHECK_SYSTEM,m_sdps->m_bSystem);
	lCheckDlgButton(m_hDlg,IDC_CHECK_SEARCHSUBFOLDERS,m_sdps->m_bSearchSubFolders);
	lCheckDlgButton(m_hDlg,IDC_CHECK_CASEINSENSITIVE,m_sdps->m_bCaseInsensitive);
	lCheckDlgButton(m_hDlg,IDC_CHECK_USEREGULAREXPRESSIONS,m_sdps->m_bUseRegularExpressions);

	for(const auto &strDirectory : *m_sdps->m_pSearchDirectories)
	{
		SendDlgItemMessage(m_hDlg,IDC_COMBO_DIRECTORY,CB_INSERTSTRING,static_cast<WPARAM>(-1),
			reinterpret_cast<LPARAM>(strDirectory.c_str()));
	}

	for(const auto strPattern : *m_sdps->m_pSearchPatterns)
	{
		SendDlgItemMessage(m_hDlg,IDC_COMBO_NAME,CB_INSERTSTRING,static_cast<WPARAM>(-1),
			reinterpret_cast<LPARAM>(strPattern.c_str()));
	}

	SetDlgItemText(m_hDlg,IDC_COMBO_NAME,m_sdps->m_szSearchPattern);
	SetDlgItemText(m_hDlg,IDC_COMBO_DIRECTORY,m_szSearchDirectory);

	CComboBox::CreateNew(GetDlgItem(m_hDlg,IDC_COMBO_NAME));
	CComboBox::CreateNew(GetDlgItem(m_hDlg,IDC_COMBO_DIRECTORY));

	if(m_sdps->m_bStateSaved)
	{
		/* These dummy values will be in use if these values
		have not previously been saved. */
		if(m_sdps->m_iColumnWidth1 != -1 && m_sdps->m_iColumnWidth2 != -1)
		{
			ListView_SetColumnWidth(hListView,0,m_sdps->m_iColumnWidth1);
			ListView_SetColumnWidth(hListView,1,m_sdps->m_iColumnWidth2);
		}
	}

	m_sdps->RestoreDialogPosition(m_hDlg,true);

	SetFocus(GetDlgItem(m_hDlg,IDC_COMBO_NAME));

	return FALSE;
}

void CSearchDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_COMBO_NAME;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_COMBO_DIRECTORY;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_BUTTON_DIRECTORY;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_LISTVIEW_SEARCHRESULTS;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_STATUSLABEL;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_STATUS;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_STATUS;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_LINK_STATUS;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_LINK_STATUS;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_ETCHEDHORZ;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_ETCHEDHORZ;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDSEARCH;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDEXIT;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_GRIPPER;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);
}

INT_PTR CSearchDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch(LOWORD(wParam))
	{
	case IDSEARCH:
		OnSearch();
		break;

	case IDC_BUTTON_DIRECTORY:
		{
			BROWSEINFO bi;
			TCHAR szDirectory[MAX_PATH];
			TCHAR szDisplayName[MAX_PATH];
			TCHAR szParsingPath[MAX_PATH];
			TCHAR szTitle[256];

			LoadString(GetInstance(),IDS_SEARCHDIALOG_TITLE,szTitle,SIZEOF_ARRAY(szTitle));

			GetDlgItemText(m_hDlg,IDC_COMBO_DIRECTORY,szDirectory,SIZEOF_ARRAY(szDirectory));

			bi.hwndOwner		= m_hDlg;
			bi.pidlRoot			= NULL;
			bi.pszDisplayName	= szDisplayName;
			bi.lpszTitle		= szTitle;
			bi.ulFlags			= BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
			bi.lpfn				= NSearchDialog::BrowseCallbackProc;
			bi.lParam			= reinterpret_cast<LPARAM>(szDirectory);
			PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);

			if(pidl != NULL)
			{
				GetDisplayName(pidl,szParsingPath,SIZEOF_ARRAY(szParsingPath),SHGDN_FORPARSING);
				SetDlgItemText(m_hDlg,IDC_COMBO_DIRECTORY,szParsingPath);

				CoTaskMemFree(pidl);
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

int CALLBACK NSearchDialog::BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	UNREFERENCED_PARAMETER(lParam);

	assert(lpData != NULL);

	TCHAR *szSearchPattern = reinterpret_cast<TCHAR *>(lpData);

	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hwnd,BFFM_SETSELECTION,TRUE,reinterpret_cast<LPARAM>(szSearchPattern));
		break;
	}

	return 0;
}

void CSearchDialog::OnSearch()
{
	if(!m_bSearching)
	{
		StartSearching();
	}
	else
	{
		StopSearching();
	}
}

void CSearchDialog::StartSearching()
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
	/* TODO: Verify fields. */
	GetDlgItemText(m_hDlg, IDC_COMBO_DIRECTORY, szBaseDirectory,
		SIZEOF_ARRAY(szBaseDirectory));
	PathRemoveBlanks(szBaseDirectory);
	GetDlgItemText(m_hDlg, IDC_COMBO_NAME, szSearchPattern,
		SIZEOF_ARRAY(szSearchPattern));
	PathRemoveBlanks(szSearchPattern);

	BOOL bSearchSubFolders = IsDlgButtonChecked(m_hDlg, IDC_CHECK_SEARCHSUBFOLDERS) ==
		BST_CHECKED;

	BOOL bUseRegularExpressions = IsDlgButtonChecked(m_hDlg, IDC_CHECK_USEREGULAREXPRESSIONS) ==
		BST_CHECKED;

	BOOL bCaseInsensitive = IsDlgButtonChecked(m_hDlg, IDC_CHECK_CASEINSENSITIVE) ==
		BST_CHECKED;

	/* Turn search patterns of the form '???' into '*???*', and
	use this modified string to search. */
	if(!bUseRegularExpressions && lstrlen(szSearchPattern) > 0)
	{
		if(szSearchPattern[0] != '*' &&
			szSearchPattern[lstrlen(szSearchPattern) - 1] != '*')
		{
			TCHAR szTemp[MAX_PATH];

			StringCchPrintf(szTemp, SIZEOF_ARRAY(szTemp), _T("*%s*"),
				szSearchPattern);
			StringCchCopy(szSearchPattern, SIZEOF_ARRAY(szSearchPattern),
				szTemp);
		}
	}

	DWORD dwAttributes = 0;

	if(IsDlgButtonChecked(m_hDlg, IDC_CHECK_ARCHIVE) == BST_CHECKED)
		dwAttributes |= FILE_ATTRIBUTE_ARCHIVE;

	if(IsDlgButtonChecked(m_hDlg, IDC_CHECK_HIDDEN) == BST_CHECKED)
		dwAttributes |= FILE_ATTRIBUTE_HIDDEN;

	if(IsDlgButtonChecked(m_hDlg, IDC_CHECK_READONLY) == BST_CHECKED)
		dwAttributes |= FILE_ATTRIBUTE_READONLY;

	if(IsDlgButtonChecked(m_hDlg, IDC_CHECK_SYSTEM) == BST_CHECKED)
		dwAttributes |= FILE_ATTRIBUTE_SYSTEM;

	m_pSearch = new CSearch(m_hDlg, szBaseDirectory, szSearchPattern,
		dwAttributes, bUseRegularExpressions, bCaseInsensitive, bSearchSubFolders);
	m_pSearch->AddRef();

	/* Save the search directory and search pattern (only if they are not
	the same as the most recent entry). */
	BOOL bSaveEntry = FALSE;

	if(m_sdps->m_pSearchDirectories->empty() ||
		lstrcmp(szBaseDirectory, m_sdps->m_pSearchDirectories->begin()->c_str()) != 0)
	{
		bSaveEntry = TRUE;
	}

	if(bSaveEntry)
	{
		SaveEntry(IDC_COMBO_DIRECTORY, *m_sdps->m_pSearchDirectories);
	}

	bSaveEntry = FALSE;

	if(m_sdps->m_pSearchPatterns->empty() ||
		lstrcmp(szSearchPattern, m_sdps->m_pSearchPatterns->begin()->c_str()) != 0)
	{
		bSaveEntry = TRUE;
	}

	if(bSaveEntry)
	{
		SaveEntry(IDC_COMBO_NAME, *m_sdps->m_pSearchPatterns);
	}

	GetDlgItemText(m_hDlg, IDSEARCH, m_szSearchButton, SIZEOF_ARRAY(m_szSearchButton));

	TCHAR szTemp[64];

	LoadString(GetInstance(), IDS_STOP, szTemp, SIZEOF_ARRAY(szTemp));
	SetDlgItemText(m_hDlg, IDSEARCH, szTemp);

	m_bSearching = TRUE;

	/* Create a background thread, and search using it... */
	HANDLE hThread = CreateThread(NULL, 0, NSearchDialog::SearchThread,
		reinterpret_cast<LPVOID>(m_pSearch), 0, NULL);
	CloseHandle(hThread);
}

void CSearchDialog::SaveEntry(int comboBoxId, boost::circular_buffer<std::wstring> &buffer)
{
	TCHAR entry[MAX_PATH];
	GetDlgItemText(m_hDlg, comboBoxId, entry, SIZEOF_ARRAY(entry));

	std::wstring strEntry(entry);
	auto itr = std::find_if(buffer.begin(), buffer.end(),
		[strEntry] (const std::wstring Pattern)
	{
		return Pattern.compare(strEntry) == 0;
	});

	HWND hComboBox = GetDlgItem(m_hDlg, comboBoxId);
	ComboBox_SetCurSel(hComboBox, -1);

	if(itr != buffer.end())
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

	if(ComboBox_GetCount(hComboBox) > buffer.capacity())
	{
		SendMessage(hComboBox, CB_DELETESTRING, ComboBox_GetCount(hComboBox) - 1, 0);
	}
}

void CSearchDialog::StopSearching()
{
	m_bStopSearching = TRUE;

	if(m_pSearch != NULL)
	{
		/* Note that m_pSearch does not need to be
		released here. Once the search object finishes,
		it will send a WM_APP_SEARCHFINISHED message.
		The handler for this message will then release
		m_pSearch. */
		m_pSearch->StopSearching();
	}
}

void CSearchDialog::UpdateListViewHeader()
{
	HWND hHeader = ListView_GetHeader(GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS));

	HDITEM hdItem;

	/* Remove the sort arrow from the column that was
	previously selected. */
	if(m_iPreviousSelectedColumn != -1)
	{
		hdItem.mask	= HDI_FORMAT;
		Header_GetItem(hHeader,m_iPreviousSelectedColumn,&hdItem);

		if(hdItem.fmt & HDF_SORTUP)
		{
			hdItem.fmt &= ~HDF_SORTUP;
		}
		else if(hdItem.fmt & HDF_SORTDOWN)
		{
			hdItem.fmt &= ~HDF_SORTDOWN;
		}

		Header_SetItem(hHeader,m_iPreviousSelectedColumn,&hdItem);
	}

	int iColumn = 0;

	for(const auto &ci : m_sdps->m_Columns)
	{
		if(ci.SortMode == m_sdps->m_SortMode)
		{
			break;
		}

		iColumn++;
	}

	hdItem.mask	= HDI_FORMAT;
	Header_GetItem(hHeader,iColumn,&hdItem);

	if(m_sdps->m_bSortAscending)
	{
		hdItem.fmt |= HDF_SORTUP;
	}
	else
	{
		hdItem.fmt |= HDF_SORTDOWN;
	}

	Header_SetItem(hHeader,iColumn,&hdItem);

	m_iPreviousSelectedColumn = iColumn;
}

int CALLBACK NSearchDialog::SortResultsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	assert(lParamSort != NULL);

	CSearchDialog *psd = reinterpret_cast<CSearchDialog *>(lParamSort);

	return psd->SortResults(lParam1,lParam2);
}

int CALLBACK CSearchDialog::SortResults(LPARAM lParam1,LPARAM lParam2)
{
	int iRes = 0;

	switch(m_sdps->m_SortMode)
	{
	case CSearchDialogPersistentSettings::SORT_NAME:
		iRes = SortResultsByName(lParam1,lParam2);
		break;

	case CSearchDialogPersistentSettings::SORT_PATH:
		iRes = SortResultsByPath(lParam1,lParam2);
		break;
	}

	if(!m_sdps->m_bSortAscending)
	{
		iRes = -iRes;
	}

	return iRes;
}

int CALLBACK CSearchDialog::SortResultsByName(LPARAM lParam1,LPARAM lParam2)
{
	auto itr1 = m_SearchItemsMapInternal.find(static_cast<int>(lParam1));
	auto itr2 = m_SearchItemsMapInternal.find(static_cast<int>(lParam2));

	TCHAR szFilename1[MAX_PATH];
	TCHAR szFilename2[MAX_PATH];

	StringCchCopy(szFilename1,SIZEOF_ARRAY(szFilename1),itr1->second.c_str());
	StringCchCopy(szFilename2,SIZEOF_ARRAY(szFilename2),itr2->second.c_str());

	PathStripPath(szFilename1);
	PathStripPath(szFilename2);

	return StrCmpLogicalW(szFilename1,szFilename2);
}

int CALLBACK CSearchDialog::SortResultsByPath(LPARAM lParam1,LPARAM lParam2)
{
	auto itr1 = m_SearchItemsMapInternal.find(static_cast<int>(lParam1));
	auto itr2 = m_SearchItemsMapInternal.find(static_cast<int>(lParam2));

	TCHAR szPath1[MAX_PATH];
	TCHAR szPath2[MAX_PATH];

	StringCchCopy(szPath1,SIZEOF_ARRAY(szPath1),itr1->second.c_str());
	StringCchCopy(szPath2,SIZEOF_ARRAY(szPath2),itr2->second.c_str());

	PathRemoveFileSpec(szPath1);
	PathRemoveFileSpec(szPath2);

	return StrCmpLogicalW(szPath1,szPath2);
}

void CSearchDialog::AddMenuEntries(LPCITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu)
{
	UNREFERENCED_PARAMETER(dwData);

	LPITEMIDLIST pidlComplete = ILCombine(pidlParent,pidlItemList.front());
	SFGAOF ItemAttributes = SFGAO_FOLDER;
	GetItemAttributes(pidlComplete,&ItemAttributes);
	CoTaskMemFree(pidlComplete);

	TCHAR szTemp[64];

	if((ItemAttributes & SFGAO_FOLDER) == SFGAO_FOLDER)
	{
		LoadString(GetInstance(),IDS_SEARCH_OPEN_FOLDER_LOCATION,
			szTemp,SIZEOF_ARRAY(szTemp));
	}
	else
	{
		LoadString(GetInstance(),IDS_SEARCH_OPEN_FILE_LOCATION,
			szTemp,SIZEOF_ARRAY(szTemp));
	}

	MENUITEMINFO mii;
	mii.cbSize		= sizeof(MENUITEMINFO);
	mii.fMask		= MIIM_STRING|MIIM_ID;
	mii.wID			= MENU_ID_OPEN_FILE_LOCATION;
	mii.dwTypeData	= szTemp;
	InsertMenuItem(hMenu,1,TRUE,&mii);
}

BOOL CSearchDialog::HandleShellMenuItem(LPCITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,const TCHAR *szCmd)
{
	UNREFERENCED_PARAMETER(dwData);

	if(StrCmpI(szCmd,_T("open")) == 0)
	{
		for(auto pidlItem : pidlItemList)
		{
			LPITEMIDLIST pidlComplete = ILCombine(pidlParent,pidlItem);
			m_pexpp->OpenItem(pidlComplete,FALSE,FALSE);
			CoTaskMemFree(pidlComplete);
		}

		return TRUE;
	}

	return FALSE;
}

void CSearchDialog::HandleCustomMenuItem(LPCITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,int iCmd)
{
	switch(iCmd)
	{
	case MENU_ID_OPEN_FILE_LOCATION:
		{
			m_tabContainer->CreateNewTab(pidlParent, TabSettings(_selected = true));

			TCHAR szFilename[MAX_PATH];
			LPITEMIDLIST pidlComplete = ILCombine(pidlParent,pidlItemList.front());
			GetDisplayName(pidlComplete,szFilename,SIZEOF_ARRAY(szFilename),SHGDN_INFOLDER|SHGDN_FORPARSING);
			CoTaskMemFree(pidlComplete);

			m_pexpp->GetActiveShellBrowser()->SelectFiles(szFilename);
		}
		break;
	}
}

INT_PTR CSearchDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_DBLCLK:
		if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS))
		{
			HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);
			int iSelected = ListView_GetNextItem(hListView,-1,LVNI_ALL|LVNI_SELECTED);

			if(iSelected != -1)
			{
				LVITEM lvItem;

				lvItem.mask		= LVIF_PARAM;
				lvItem.iItem	= iSelected;
				lvItem.iSubItem	= 0;

				BOOL bRet = ListView_GetItem(hListView,&lvItem);

				if(bRet)
				{
					auto itr = m_SearchItemsMapInternal.find(static_cast<int>(lvItem.lParam));

					/* Item should always exist. */
					assert(itr != m_SearchItemsMapInternal.end());

					LPITEMIDLIST pidlFull = NULL;

					HRESULT hr = GetIdlFromParsingName(itr->second.c_str(),&pidlFull);

					if(hr == S_OK)
					{
						m_pexpp->OpenItem(pidlFull,FALSE,FALSE);

						CoTaskMemFree(pidlFull);
					}
				}
			}
		}
		break;

	case NM_CLICK:
		{
			if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_LINK_STATUS))
			{
				PNMLINK pnmlink = reinterpret_cast<PNMLINK>(pnmhdr);

				ShellExecute(NULL,L"open",pnmlink->item.szUrl,
					NULL,NULL,SW_SHOW);
			}
		}
		break;

	case NM_RCLICK:
		{
			if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS))
			{
				HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);
				int iSelected = ListView_GetNextItem(hListView,-1,LVNI_ALL|LVNI_SELECTED);

				if(iSelected != -1)
				{
					LVITEM lvItem;
					lvItem.mask		= LVIF_PARAM;
					lvItem.iItem	= iSelected;
					lvItem.iSubItem	= 0;
					BOOL bRet = ListView_GetItem(hListView,&lvItem);

					if(bRet)
					{
						auto itr = m_SearchItemsMapInternal.find(static_cast<int>(lvItem.lParam));
						assert(itr != m_SearchItemsMapInternal.end());

						LPITEMIDLIST pidlFull = NULL;

						HRESULT hr = GetIdlFromParsingName(itr->second.c_str(),&pidlFull);

						if(hr == S_OK)
						{
							std::list<LPITEMIDLIST> pidlList;
							pidlList.push_back(ILFindLastID(pidlFull));

							LPITEMIDLIST pidlDirectory = ILClone(pidlFull);
							ILRemoveLastID(pidlDirectory);

							CFileContextMenuManager fcmm(m_hDlg,pidlDirectory,
								pidlList);

							DWORD dwCursorPos = GetMessagePos();

							POINT ptCursor;
							ptCursor.x = GET_X_LPARAM(dwCursorPos);
							ptCursor.y = GET_Y_LPARAM(dwCursorPos);

							fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,&ptCursor,m_pexpp->GetStatusBar(),
								NULL,FALSE,IsKeyDown(VK_SHIFT));

							CoTaskMemFree(pidlDirectory);
							CoTaskMemFree(pidlFull);
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
			NMLISTVIEW *pnmlv = reinterpret_cast<NMLISTVIEW *>(pnmhdr);

			/* If the column clicked matches the current sort mode,
			flip the sort direction, else switch to that sort mode. */
			if(m_sdps->m_Columns[pnmlv->iSubItem].SortMode == m_sdps->m_SortMode)
			{
				m_sdps->m_bSortAscending = !m_sdps->m_bSortAscending;
			}
			else
			{
				m_sdps->m_SortMode = m_sdps->m_Columns[pnmlv->iSubItem].SortMode;
				m_sdps->m_bSortAscending = m_sdps->m_Columns[pnmlv->iSubItem].bSortAscending;
			}

			ListView_SortItems(GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS),
				NSearchDialog::SortResultsStub,reinterpret_cast<LPARAM>(this));

			UpdateListViewHeader();
		}
		break;
	}

	return 0;
}

INT_PTR CSearchDialog::OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		/* We won't actually process the item here. Instead, we'll
		add it onto the list of current items, which will be processed
		in batch. This is done to stop this message from blocking the
		main GUI (also see http://www.flounder.com/iocompletion.htm). */
		case NSearchDialog::WM_APP_SEARCHITEMFOUND:
			{
				m_AwaitingSearchItems.push_back(reinterpret_cast<LPITEMIDLIST>(wParam));

				if(m_bSetSearchTimer)
				{
					SetTimer(m_hDlg,SEARCH_PROCESSITEMS_TIMER_ID,
						SEARCH_PROCESSITEMS_TIMER_ELAPSED,NULL);

					m_bSetSearchTimer = FALSE;
				}
			}
			break;

		case NSearchDialog::WM_APP_SEARCHFINISHED:
			{
				TCHAR szStatus[512];

				if(!m_bStopSearching)
				{
					int iFoldersFound = LOWORD(lParam);
					int iFilesFound = HIWORD(lParam);

					TCHAR szTemp[128];
					LoadString(GetInstance(),IDS_SEARCH_FINISHED_MESSAGE,
						szTemp,SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szStatus,SIZEOF_ARRAY(szStatus),szTemp,
						iFoldersFound,iFilesFound);
					SetDlgItemText(m_hDlg,IDC_STATIC_STATUS,szStatus);
				}
				else
				{
					TCHAR szTemp[128];
					LoadString(GetInstance(),IDS_SEARCH_CANCELLED_MESSAGE,
						szTemp,SIZEOF_ARRAY(szTemp));
					SetDlgItemText(m_hDlg,IDC_STATIC_STATUS,szTemp);
				}

				assert(m_pSearch != NULL);

				m_pSearch->Release();
				m_pSearch = NULL;

				m_bSearching = FALSE;
				m_bStopSearching = FALSE;
				SetDlgItemText(m_hDlg,IDSEARCH,m_szSearchButton);
			}
			break;

		case NSearchDialog::WM_APP_SEARCHCHANGEDDIRECTORY:
			{
				TCHAR szStatus[512];
				TCHAR *pszDirectory = NULL;

				pszDirectory = reinterpret_cast<TCHAR *>(wParam);

				TCHAR szTemp[64];
				LoadString(GetInstance(),IDS_SEARCHING,
					szTemp,SIZEOF_ARRAY(szTemp));
				StringCchPrintf(szStatus,SIZEOF_ARRAY(szStatus),szTemp,
					pszDirectory);
				SetDlgItemText(m_hDlg,IDC_STATIC_STATUS,szStatus);
			}
			break;

		case NSearchDialog::WM_APP_REGULAREXPRESSIONINVALID:
			{
				/* The link/status controls are in the same position, and
				have the same size. If one of the controls is showing text,
				the other should not be visible. */
				ShowWindow(GetDlgItem(m_hDlg,IDC_LINK_STATUS),SW_SHOW);
				ShowWindow(GetDlgItem(m_hDlg,IDC_STATIC_STATUS),SW_HIDE);

				/* The regular expression passed to the search
				thread was invalid. Show the user an error message. */
				TCHAR szTemp[128];
				LoadString(GetInstance(),IDS_SEARCH_REGULAR_EXPRESSION_INVALID,
					szTemp,SIZEOF_ARRAY(szTemp));
				SetDlgItemText(m_hDlg,IDC_LINK_STATUS,szTemp);

				assert(m_pSearch != NULL);

				m_pSearch->Release();
				m_pSearch = NULL;

				m_bSearching = FALSE;
				m_bStopSearching = FALSE;
				SetDlgItemText(m_hDlg,IDSEARCH,m_szSearchButton);
			}
			break;
	}

	return 0;
}

INT_PTR CSearchDialog::OnTimer(int iTimerID)
{
	if(iTimerID != SEARCH_PROCESSITEMS_TIMER_ID)
	{
		return 1;
	}

	HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);
	int nListViewItems = ListView_GetItemCount(hListView);

	int nItems = min(static_cast<int>(m_AwaitingSearchItems.size()),
		SEARCH_MAX_ITEMS_BATCH_PROCESS);
	int i = 0;

	auto itr = m_AwaitingSearchItems.begin();

	while(i < nItems)
	{
		TCHAR szFullFileName[MAX_PATH];
		TCHAR szDirectory[MAX_PATH];
		TCHAR szFileName[MAX_PATH];
		LVITEM lvItem;
		SHFILEINFO shfi;
		int iIndex;

		LPITEMIDLIST pidl = *itr;

		GetDisplayName(pidl,szDirectory,SIZEOF_ARRAY(szDirectory),SHGDN_FORPARSING);
		PathRemoveFileSpec(szDirectory);

		GetDisplayName(pidl,szFullFileName,SIZEOF_ARRAY(szFullFileName),SHGDN_FORPARSING);
		GetDisplayName(pidl,szFileName,SIZEOF_ARRAY(szFileName),SHGDN_INFOLDER|SHGDN_FORPARSING);

		SHGetFileInfo((LPCWSTR)pidl,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);

		m_SearchItemsMapInternal.insert(std::unordered_map<int,std::wstring>::value_type(m_iInternalIndex,
			szFullFileName));

		lvItem.mask		= LVIF_IMAGE|LVIF_TEXT|LVIF_PARAM;
		lvItem.pszText	= szFileName;
		lvItem.iItem	= nListViewItems + i;
		lvItem.iSubItem	= 0;
		lvItem.iImage	= shfi.iIcon;
		lvItem.lParam	= m_iInternalIndex++;
		iIndex = ListView_InsertItem(hListView,&lvItem);

		ListView_SetItemText(hListView,iIndex,1,szDirectory);

		CoTaskMemFree(pidl);

		itr = m_AwaitingSearchItems.erase(itr);

		i++;
	}

	m_bSetSearchTimer = TRUE;

	return 0;
}

INT_PTR CSearchDialog::OnClose()
{
	DestroyWindow(m_hDlg);

	return 0;
}

INT_PTR CSearchDialog::OnNcDestroy()
{
	delete this;

	return 0;
}

DWORD WINAPI NSearchDialog::SearchThread(LPVOID pParam)
{
	assert(pParam != NULL);

	CSearch *pSearch = reinterpret_cast<CSearch *>(pParam);
	pSearch->StartSearching();

	return 0;
}

CSearch::CSearch(HWND hDlg,TCHAR *szBaseDirectory,
	TCHAR *szPattern,DWORD dwAttributes,BOOL bUseRegularExpressions,
	BOOL bCaseInsensitive,BOOL bSearchSubFolders)
{
	m_hDlg = hDlg;
	m_dwAttributes = dwAttributes;
	m_bUseRegularExpressions = bUseRegularExpressions;
	m_bCaseInsensitive = bCaseInsensitive;
	m_bSearchSubFolders = bSearchSubFolders;

	StringCchCopy(m_szBaseDirectory,SIZEOF_ARRAY(m_szBaseDirectory),
		szBaseDirectory);
	StringCchCopy(m_szSearchPattern,SIZEOF_ARRAY(m_szSearchPattern),
		szPattern);

	InitializeCriticalSection(&m_csStop);
	m_bStopSearching = FALSE;
}

CSearch::~CSearch()
{
	DeleteCriticalSection(&m_csStop);
}

void CSearch::StartSearching()
{
	m_iFoldersFound = 0;
	m_iFilesFound = 0;

	if(lstrcmp(m_szSearchPattern,EMPTY_STRING) != 0 &&
		m_bUseRegularExpressions)
	{
		try
		{
			if(m_bCaseInsensitive)
			{
				m_rxPattern.assign(m_szSearchPattern,std::regex_constants::icase);
			}
			else
			{
				m_rxPattern.assign(m_szSearchPattern);
			}
		}
		catch(std::exception)
		{
			SendMessage(m_hDlg,NSearchDialog::WM_APP_REGULAREXPRESSIONINVALID,
				0,0);

			return;
		}
	}

	SearchDirectory(m_szBaseDirectory);

	SendMessage(m_hDlg,NSearchDialog::WM_APP_SEARCHFINISHED,0,
		MAKELPARAM(m_iFoldersFound,m_iFilesFound));

	Release();
}

void CSearch::SearchDirectory(const TCHAR *szDirectory)
{
	SendMessage(m_hDlg,NSearchDialog::WM_APP_SEARCHCHANGEDDIRECTORY,
		reinterpret_cast<WPARAM>(szDirectory),0);

	std::list<std::wstring> SubFolderList;

	SearchDirectoryInternal(szDirectory,&SubFolderList);

	bool bStop = false;

	EnterCriticalSection(&m_csStop);
	if(m_bStopSearching)
	{
		bStop = true;
	}
	LeaveCriticalSection(&m_csStop);

	if(bStop)
	{
		return;
	}

	if(m_bSearchSubFolders)
	{
		for(const auto &strSubFolder : SubFolderList)
		{
			SearchDirectory(strSubFolder.c_str());
		}
	}
}

/* Can't recurse, as it would overflow the stack. */
void CSearch::SearchDirectoryInternal(const TCHAR *szSearchDirectory,
	std::list<std::wstring> *pSubFolderList)
{
	assert(szSearchDirectory != NULL);
	assert(pSubFolderList != NULL);

	WIN32_FIND_DATA wfd;
	TCHAR szSearchTerm[MAX_PATH];

	PathCombine(szSearchTerm,szSearchDirectory,_T("*"));

	HANDLE hFindFile = FindFirstFile(szSearchTerm,&wfd);

	if(hFindFile != INVALID_HANDLE_VALUE)
	{
		bool bStop = false;

		while(FindNextFile(hFindFile,&wfd) != 0 && !bStop)
		{
			if(lstrcmpi(wfd.cFileName,_T(".")) != 0 &&
				lstrcmpi(wfd.cFileName,_T("..")) != 0)
			{
				BOOL bMatchFileName = FALSE;
				BOOL bMatchAttributes = FALSE;

				/* Only match against the filename if it's not empty. */
				if(lstrcmp(m_szSearchPattern,EMPTY_STRING) != 0)
				{
					if(m_bUseRegularExpressions)
					{
						if(std::regex_match(wfd.cFileName,m_rxPattern))
						{
							bMatchFileName = TRUE;
						}
					}
					else
					{
						if(CheckWildcardMatch(m_szSearchPattern,wfd.cFileName,!m_bCaseInsensitive))
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

				if(m_dwAttributes != 0)
				{
					if((wfd.dwFileAttributes & m_dwAttributes) == m_dwAttributes)
					{
						bMatchAttributes = TRUE;
					}
				}
				else
				{
					bMatchAttributes = TRUE;
				}

				BOOL bItemMatch = bMatchFileName && bMatchAttributes;

				if(bItemMatch)
				{
					if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
						FILE_ATTRIBUTE_DIRECTORY)
						m_iFoldersFound++;
					else
						m_iFilesFound++;

					LPITEMIDLIST pidl = NULL;
					TCHAR szFullFileName[MAX_PATH];

					PathCombine(szFullFileName,szSearchDirectory,wfd.cFileName);
					GetIdlFromParsingName(szFullFileName,&pidl);

					PostMessage(m_hDlg,NSearchDialog::WM_APP_SEARCHITEMFOUND,
						reinterpret_cast<WPARAM>(ILClone(pidl)),0);

					CoTaskMemFree(pidl);
				}

				if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
					FILE_ATTRIBUTE_DIRECTORY)
				{
					TCHAR szSubFolder[MAX_PATH];

					PathCombine(szSubFolder,szSearchDirectory,
						wfd.cFileName);

					pSubFolderList->push_back(szSubFolder);
				}
			}

			EnterCriticalSection(&m_csStop);
			if(m_bStopSearching)
			{
				bStop = true;
			}
			LeaveCriticalSection(&m_csStop);
		}

		FindClose(hFindFile);
	}
}

void CSearch::StopSearching()
{
	EnterCriticalSection(&m_csStop);
	m_bStopSearching = TRUE;
	LeaveCriticalSection(&m_csStop);
}

void CSearchDialog::SaveState()
{
	HWND hListView;

	m_sdps->SaveDialogPosition(m_hDlg);

	m_sdps->m_bCaseInsensitive = IsDlgButtonChecked(m_hDlg,
		IDC_CHECK_CASEINSENSITIVE) == BST_CHECKED;

	m_sdps->m_bUseRegularExpressions = IsDlgButtonChecked(m_hDlg,
		IDC_CHECK_USEREGULAREXPRESSIONS) == BST_CHECKED;

	m_sdps->m_bSearchSubFolders = IsDlgButtonChecked(m_hDlg,
		IDC_CHECK_SEARCHSUBFOLDERS) == BST_CHECKED;

	m_sdps->m_bArchive = IsDlgButtonChecked(m_hDlg,
		IDC_CHECK_ARCHIVE) == BST_CHECKED;

	m_sdps->m_bHidden = IsDlgButtonChecked(m_hDlg,
		IDC_CHECK_HIDDEN) == BST_CHECKED;

	m_sdps->m_bReadOnly = IsDlgButtonChecked(m_hDlg,
		IDC_CHECK_READONLY) == BST_CHECKED;

	m_sdps->m_bSystem = IsDlgButtonChecked(m_hDlg,
		IDC_CHECK_SYSTEM) == BST_CHECKED;

	hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);

	m_sdps->m_iColumnWidth1 = ListView_GetColumnWidth(hListView,0);
	m_sdps->m_iColumnWidth2 = ListView_GetColumnWidth(hListView,1);

	GetDlgItemText(m_hDlg,IDC_COMBO_NAME,m_sdps->m_szSearchPattern,
		SIZEOF_ARRAY(m_sdps->m_szSearchPattern));

	m_sdps->m_bStateSaved = TRUE;
}

CSearchDialogPersistentSettings::CSearchDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_bSearchSubFolders			= TRUE;
	m_bUseRegularExpressions	= FALSE;
	m_bCaseInsensitive			= FALSE;
	m_bArchive					= FALSE;
	m_bHidden					= FALSE;
	m_bReadOnly					= FALSE;
	m_bSystem					= FALSE;
	m_iColumnWidth1				= -1;
	m_iColumnWidth2				= -1;

	StringCchCopy(m_szSearchPattern,SIZEOF_ARRAY(m_szSearchPattern),
		EMPTY_STRING);

	m_pSearchPatterns	= new boost::circular_buffer<std::wstring>(NDialogHelper::DEFAULT_HISTORY_SIZE);
	m_pSearchDirectories = new boost::circular_buffer<std::wstring>(NDialogHelper::DEFAULT_HISTORY_SIZE);

	ColumnInfo_t ci;
	ci.SortMode			= SORT_NAME;
	ci.uStringID		= IDS_SEARCH_COLUMN_NAME;
	ci.bSortAscending	= true;
	m_Columns.push_back(ci);

	ci.SortMode			= SORT_PATH;
	ci.uStringID		= IDS_SEARCH_COLUMN_PATH;
	ci.bSortAscending	= true;
	m_Columns.push_back(ci);

	m_SortMode			= m_Columns.front().SortMode;
	m_bSortAscending	= m_Columns.front().bSortAscending;
}

CSearchDialogPersistentSettings::~CSearchDialogPersistentSettings()
{
	delete m_pSearchPatterns;
}

CSearchDialogPersistentSettings& CSearchDialogPersistentSettings::GetInstance()
{
	static CSearchDialogPersistentSettings sdps;
	return sdps;
}

void CSearchDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_COLUMN_WIDTH_1, m_iColumnWidth1);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_COLUMN_WIDTH_2, m_iColumnWidth2);
	NRegistrySettings::SaveStringToRegistry(hKey, SETTING_SEARCH_DIRECTORY_TEXT, m_szSearchPattern);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_SEARCH_SUB_FOLDERS, m_bSearchSubFolders);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_USE_REGULAR_EXPRESSIONS, m_bUseRegularExpressions);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_CASE_INSENSITIVE, m_bCaseInsensitive);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_ARCHIVE, m_bArchive);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_HIDDEN, m_bHidden);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_READ_ONLY, m_bReadOnly);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_SYSTEM, m_bSystem);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_SORT_MODE, m_SortMode);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_SORT_ASCENDING, m_bSortAscending);

	std::list<std::wstring> SearchDirectoriesList;
	CircularBufferToList(*m_pSearchDirectories, SearchDirectoriesList);
	NRegistrySettings::SaveStringListToRegistry(hKey, SETTING_DIRECTORY_LIST, SearchDirectoriesList);

	std::list<std::wstring> SearchPatternList;
	CircularBufferToList(*m_pSearchPatterns,SearchPatternList);
	NRegistrySettings::SaveStringListToRegistry(hKey, SETTING_PATTERN_LIST, SearchPatternList);
}

void CSearchDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_COLUMN_WIDTH_1, reinterpret_cast<LPDWORD>(&m_iColumnWidth1));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_COLUMN_WIDTH_2, reinterpret_cast<LPDWORD>(&m_iColumnWidth2));
	NRegistrySettings::ReadStringFromRegistry(hKey, SETTING_SEARCH_DIRECTORY_TEXT, m_szSearchPattern, SIZEOF_ARRAY(m_szSearchPattern));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_SEARCH_SUB_FOLDERS, reinterpret_cast<LPDWORD>(&m_bSearchSubFolders));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_USE_REGULAR_EXPRESSIONS, reinterpret_cast<LPDWORD>(&m_bUseRegularExpressions));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_CASE_INSENSITIVE, reinterpret_cast<LPDWORD>(&m_bCaseInsensitive));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_ARCHIVE, reinterpret_cast<LPDWORD>(&m_bArchive));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_HIDDEN, reinterpret_cast<LPDWORD>(&m_bHidden));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_READ_ONLY, reinterpret_cast<LPDWORD>(&m_bReadOnly));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_SYSTEM, reinterpret_cast<LPDWORD>(&m_bSystem));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_SORT_MODE, reinterpret_cast<LPDWORD>(&m_SortMode));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_SORT_ASCENDING, reinterpret_cast<LPDWORD>(&m_bSortAscending));

	std::list<std::wstring> SearchDirectoriesList;
	NRegistrySettings::ReadStringListFromRegistry(hKey, SETTING_DIRECTORY_LIST, SearchDirectoriesList);
	ListToCircularBuffer(SearchDirectoriesList, *m_pSearchDirectories);

	std::list<std::wstring> SearchPatternList;
	NRegistrySettings::ReadStringListFromRegistry(hKey, SETTING_PATTERN_LIST, SearchPatternList);
	ListToCircularBuffer(SearchPatternList,*m_pSearchPatterns);
}

void CSearchDialogPersistentSettings::SaveExtraXMLSettings(
	IXMLDOMDocument *pXMLDom,IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_COLUMN_WIDTH_1, NXMLSettings::EncodeIntValue(m_iColumnWidth1));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_COLUMN_WIDTH_2, NXMLSettings::EncodeIntValue(m_iColumnWidth2));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SEARCH_DIRECTORY_TEXT, m_szSearchPattern);
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SEARCH_SUB_FOLDERS, NXMLSettings::EncodeBoolValue(m_bSearchSubFolders));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_USE_REGULAR_EXPRESSIONS, NXMLSettings::EncodeBoolValue(m_bUseRegularExpressions));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_CASE_INSENSITIVE, NXMLSettings::EncodeBoolValue(m_bCaseInsensitive));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_ARCHIVE, NXMLSettings::EncodeBoolValue(m_bArchive));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_HIDDEN, NXMLSettings::EncodeBoolValue(m_bHidden));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_READ_ONLY, NXMLSettings::EncodeBoolValue(m_bReadOnly));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SYSTEM, NXMLSettings::EncodeBoolValue(m_bSystem));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SORT_MODE, NXMLSettings::EncodeIntValue(m_SortMode));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SORT_ASCENDING, NXMLSettings::EncodeBoolValue(m_bSortAscending));

	std::list<std::wstring> SearchDirectoriesList;
	CircularBufferToList(*m_pSearchDirectories, SearchDirectoriesList);
	NXMLSettings::AddStringListToNode(pXMLDom, pParentNode, SETTING_DIRECTORY_LIST, SearchDirectoriesList);

	std::list<std::wstring> SearchPatternList;
	CircularBufferToList(*m_pSearchPatterns, SearchPatternList);
	NXMLSettings::AddStringListToNode(pXMLDom, pParentNode, SETTING_PATTERN_LIST, SearchPatternList);
}

void CSearchDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(lstrcmpi(bstrName, SETTING_COLUMN_WIDTH_1) == 0)
	{
		m_iColumnWidth1 = NXMLSettings::DecodeIntValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_COLUMN_WIDTH_2) == 0)
	{
		m_iColumnWidth2 = NXMLSettings::DecodeIntValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_SEARCH_DIRECTORY_TEXT) == 0)
	{
		StringCchCopy(m_szSearchPattern,SIZEOF_ARRAY(m_szSearchPattern),bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_SEARCH_SUB_FOLDERS) == 0)
	{
		m_bSearchSubFolders = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_USE_REGULAR_EXPRESSIONS) == 0)
	{
		m_bUseRegularExpressions = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_CASE_INSENSITIVE) == 0)
	{
		m_bCaseInsensitive = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_ARCHIVE) == 0)
	{
		m_bArchive = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_HIDDEN) == 0)
	{
		m_bHidden = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_READ_ONLY) == 0)
	{
		m_bReadOnly = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_SYSTEM) == 0)
	{
		m_bSystem = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_SORT_MODE) == 0)
	{
		int SortMode = NXMLSettings::DecodeIntValue(bstrValue);

		switch(SortMode)
		{
		case SORT_NAME:
			m_SortMode = SORT_NAME;
			break;

		case SORT_PATH:
			m_SortMode = SORT_PATH;
			break;
		}
	}
	else if(lstrcmpi(bstrName, SETTING_SORT_ASCENDING) == 0)
	{
		m_bSortAscending = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, bstrName, lstrlen(SETTING_DIRECTORY_LIST),
		SETTING_DIRECTORY_LIST, lstrlen(SETTING_DIRECTORY_LIST)) == CSTR_EQUAL)
	{
		m_pSearchDirectories->push_back(bstrValue);
	}
	else if(CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, bstrName, lstrlen(SETTING_PATTERN_LIST),
		SETTING_PATTERN_LIST, lstrlen(SETTING_PATTERN_LIST)) == CSTR_EQUAL)
	{
		m_pSearchPatterns->push_back(bstrValue);
	}
}

template <typename T>
void CSearchDialogPersistentSettings::CircularBufferToList(const boost::circular_buffer<T> &cb,
	std::list<T> &list)
{
	for(auto Item : cb)
	{
		list.push_back(Item);
	}
}

template <typename T>
void CSearchDialogPersistentSettings::ListToCircularBuffer(const std::list<T> &list,
	boost::circular_buffer<T> &cb)
{
	for(auto Item : list)
	{
		cb.push_back(Item);
	}
}