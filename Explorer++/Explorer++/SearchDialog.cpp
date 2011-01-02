/******************************************************************
 *
 * Project: Explorer++
 * File: SearchDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles all messages associated with the 'Search' dialog box.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <regex>
#include "Explorer++_internal.h"
#include "XMLSettings.h"
#include "SearchDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/Registry.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/FileContextMenuManager.h"


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

const TCHAR CSearchDialogPersistentSettings::REGISTRY_SETTINGS_KEY[] = _T("Search");

CSearchDialog::CSearchDialog(HINSTANCE hInstance,int iResource,
	HWND hParent,TCHAR *szSearchDirectory) :
CBaseDialog(hInstance,iResource,hParent)
{
	m_bSearching = FALSE;
	m_bStopSearching = FALSE;
	m_bExit = FALSE;
	m_bSetSearchTimer = TRUE;
	m_iInternalIndex = 0;

	ColumnInfo_t ci;

	ci.SearchMode = SORT_NAME;
	m_Columns.push_back(ci);

	ci.SearchMode = SORT_PATH;
	m_Columns.push_back(ci);

	m_pSearch = NULL;

	StringCchCopy(m_szSearchDirectory,SIZEOF_ARRAY(m_szSearchDirectory),
		szSearchDirectory);

	m_sdps = &CSearchDialogPersistentSettings::GetInstance();
}

CSearchDialog::~CSearchDialog()
{
	DestroyIcon(m_hDialogIcon);
	DestroyIcon(m_hDirectoryIcon);

	if(m_pSearch != NULL)
	{
		m_pSearch->StopSearching();
		m_pSearch->Release();
	}
}

BOOL CSearchDialog::OnInitDialog()
{
	HWND hListView;
	LVCOLUMN lvColumn;
	HIMAGELIST himlSmall;
	HIMAGELIST himl;
	HBITMAP hBitmap;

	SetDlgItemText(m_hDlg,IDC_COMBO_DIRECTORY,m_szSearchDirectory);

	himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);

	hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));

	ImageList_Add(himl,hBitmap,NULL);

	m_hDirectoryIcon = ImageList_GetIcon(himl,SHELLIMAGES_NEWTAB,ILD_NORMAL);
	m_hDialogIcon = ImageList_GetIcon(himl,SHELLIMAGES_SEARCH,ILD_NORMAL);

	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_hDialogIcon));

	DeleteObject(hBitmap);
	ImageList_Destroy(himl);

	SendMessage(GetDlgItem(m_hDlg,IDC_BUTTON_DIRECTORY),BM_SETIMAGE,
		IMAGE_ICON,(LPARAM)m_hDirectoryIcon);

	hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);

	ListView_SetExtendedListViewStyleEx(hListView,LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER,
		LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);

	Shell_GetImageLists(NULL,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Name");
	ListView_InsertColumn(hListView,0,&lvColumn);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Path");
	ListView_InsertColumn(hListView,1,&lvColumn);

	RECT rc;

	GetClientRect(hListView,&rc);

	/* Set the name column to 1/3 of the width of the listview and the
	path column to 2/3 the width. */
	ListView_SetColumnWidth(hListView,0,(1.0/3.0) * GetRectWidth(&rc));
	ListView_SetColumnWidth(hListView,1,(1.80/3.0) * GetRectWidth(&rc));

	RECT rcMain;
	RECT rc2;

	GetWindowRect(m_hDlg,&rcMain);
	m_iMinWidth = GetRectWidth(&rcMain);
	m_iMinHeight = GetRectHeight(&rcMain);

	GetClientRect(m_hDlg,&rcMain);

	GetWindowRect(GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	m_iListViewWidthDelta = rcMain.right - rc.right;
	m_iListViewHeightDelta = rcMain.bottom - rc.bottom;

	GetWindowRect(GetDlgItem(m_hDlg,IDC_COMBO_DIRECTORY),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	m_iSearchDirectoryWidthDelta = rcMain.right - rc.right;

	GetWindowRect(GetDlgItem(m_hDlg,IDC_COMBO_NAME),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	m_iNamedWidthDelta = rcMain.right - rc.right;

	GetWindowRect(GetDlgItem(m_hDlg,IDC_BUTTON_DIRECTORY),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	m_iButtonDirectoryLeftDelta = rcMain.right - rc.left;

	GetWindowRect(GetDlgItem(m_hDlg,IDC_STATIC_STATUS),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	m_iStaticStatusWidthDelta = rcMain.right - rc.right;
	m_iStatusVerticalDelta = rcMain.bottom - rc.top;

	GetWindowRect(GetDlgItem(m_hDlg,IDC_STATIC_ETCHEDHORZ),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	m_iEtchedHorzWidthDelta = rcMain.right - rc.right;
	m_iEtchedHorzVerticalDelta = rcMain.bottom - rc.top;

	GetWindowRect(GetDlgItem(m_hDlg,IDEXIT),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	m_iExitLeftDelta = rcMain.right - rc.left;
	m_iExitVerticalDelta = rcMain.bottom - rc.top;

	GetWindowRect(GetDlgItem(m_hDlg,IDSEARCH),&rc);
	GetWindowRect(GetDlgItem(m_hDlg,IDEXIT),&rc2);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc2,sizeof(RECT) / sizeof(POINT));
	m_iSearchExitDelta = rc2.left - rc.left;

	if(m_sdps->m_bArchive)
	{
		CheckDlgButton(m_hDlg,IDC_CHECK_ARCHIVE,BST_CHECKED);
	}

	if(m_sdps->m_bHidden)
	{
		CheckDlgButton(m_hDlg,IDC_CHECK_HIDDEN,BST_CHECKED);
	}

	if(m_sdps->m_bReadOnly)
	{
		CheckDlgButton(m_hDlg,IDC_CHECK_READONLY,BST_CHECKED);
	}

	if(m_sdps->m_bSystem)
	{
		CheckDlgButton(m_hDlg,IDC_CHECK_SYSTEM,BST_CHECKED);
	}

	if(m_sdps->m_bSearchSubFolders)
	{
		CheckDlgButton(m_hDlg,IDC_CHECK_SEARCHSUBFOLDERS,BST_CHECKED);
	}

	if(m_sdps->m_bCaseInsensitive)
	{
		CheckDlgButton(m_hDlg,IDC_CHECK_CASEINSENSITIVE,BST_CHECKED);
	}

	if(m_sdps->m_bUseRegularExpressions)
	{
		CheckDlgButton(m_hDlg,IDC_CHECK_USEREGULAREXPRESSIONS,BST_CHECKED);
	}

	COMBOBOXEXITEM cbi;
	int iItem = 0;

	HWND hComboBoxEx = GetDlgItem(m_hDlg,IDC_COMBO_DIRECTORY);
	HWND hComboBox = reinterpret_cast<HWND>(SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0));

	for each(auto strDirectory in m_sdps->m_SearchDirectories)
	{
		TCHAR szDirectory[MAX_PATH];

		StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),strDirectory.c_str());

		cbi.mask	= CBEIF_TEXT;
		cbi.iItem	= iItem++;
		cbi.pszText	= szDirectory;
		SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,(LPARAM)&cbi);
	}

	ComboBox_SetCurSel(hComboBox,0);

	hComboBoxEx = GetDlgItem(m_hDlg,IDC_COMBO_NAME);
	hComboBox = reinterpret_cast<HWND>(SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0));
	iItem = 0;

	for each(auto strPattern in m_sdps->m_SearchPatterns)
	{
		TCHAR szPattern[MAX_PATH];

		StringCchCopy(szPattern,SIZEOF_ARRAY(szPattern),strPattern.c_str());

		cbi.mask	= CBEIF_TEXT;
		cbi.iItem	= iItem++;
		cbi.pszText	= szPattern;
		SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,(LPARAM)&cbi);
	}

	ComboBox_SetCurSel(hComboBox,0);

	SetDlgItemText(m_hDlg,IDC_COMBO_NAME,m_sdps->m_szSearchPattern);

	m_hGripper = CreateWindow(_T("SCROLLBAR"),EMPTY_STRING,WS_CHILD|WS_VISIBLE|
		WS_CLIPSIBLINGS|SBS_BOTTOMALIGN|SBS_SIZEGRIP,0,0,0,0,m_hDlg,NULL,
		GetModuleHandle(0),NULL);

	GetClientRect(m_hDlg,&rcMain);
	GetWindowRect(m_hGripper,&rc);
	SetWindowPos(m_hGripper,NULL,GetRectWidth(&rcMain) - GetRectWidth(&rc),
		GetRectHeight(&rcMain) - GetRectHeight(&rc),0,0,SWP_NOSIZE|SWP_NOZORDER);

	if(m_sdps->m_bStateSaved)
	{
		/* These dummy values will be in use if these values
		have not previously been saved. */
		if(m_sdps->m_iColumnWidth1 != -1 && m_sdps->m_iColumnWidth2 != -1)
		{
			ListView_SetColumnWidth(hListView,0,m_sdps->m_iColumnWidth1);
			ListView_SetColumnWidth(hListView,1,m_sdps->m_iColumnWidth2);
		}

		SetWindowPos(m_hDlg,HWND_TOP,m_sdps->m_ptSearch.x,
			m_sdps->m_ptSearch.y,m_sdps->m_iSearchWidth,
			m_sdps->m_iSearchHeight,SWP_SHOWWINDOW);
	}
	else
	{
		CenterWindow(GetParent(m_hDlg),m_hDlg);
	}

	SetFocus(GetDlgItem(m_hDlg,IDC_COMBO_NAME));

	return FALSE;
}

BOOL CSearchDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
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
				GetDisplayName(pidl,szParsingPath,SHGDN_FORPARSING);
				SetDlgItemText(m_hDlg,IDC_COMBO_DIRECTORY,szParsingPath);

				CoTaskMemFree(pidl);
			}
		}
		break;

	case IDEXIT:
		if(m_bSearching)
		{
			m_bExit = TRUE;
			m_bStopSearching = TRUE;
		}
		else
		{
			DestroyWindow(m_hDlg);
		}
		break;

	case IDCANCEL:
		DestroyWindow(m_hDlg);
		break;
	}

	return 0;
}

int CALLBACK NSearchDialog::BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
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
		ShowWindow(GetDlgItem(m_hDlg,IDC_LINK_STATUS),SW_HIDE);
		ShowWindow(GetDlgItem(m_hDlg,IDC_STATIC_STATUS),SW_SHOW);

		m_SearchItems.clear();
		m_SearchItemsMapInternal.clear();

		ListView_DeleteAllItems(GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS));

		TCHAR szBaseDirectory[MAX_PATH];
		TCHAR szSearchPattern[MAX_PATH];

		/* Get the directory and name, and remove leading and
		trailing whitespace. */
		GetDlgItemText(m_hDlg,IDC_COMBO_DIRECTORY,szBaseDirectory,
			SIZEOF_ARRAY(szBaseDirectory));
		PathRemoveBlanks(szBaseDirectory);
		GetDlgItemText(m_hDlg,IDC_COMBO_NAME,szSearchPattern,
			SIZEOF_ARRAY(szSearchPattern));
		PathRemoveBlanks(szSearchPattern);

		BOOL bSearchSubFolders = IsDlgButtonChecked(m_hDlg,IDC_CHECK_SEARCHSUBFOLDERS) ==
			BST_CHECKED;

		BOOL bUseRegularExpressions = IsDlgButtonChecked(m_hDlg,IDC_CHECK_USEREGULAREXPRESSIONS) ==
			BST_CHECKED;

		BOOL bCaseInsensitive = IsDlgButtonChecked(m_hDlg,IDC_CHECK_CASEINSENSITIVE) ==
			BST_CHECKED;

		/* Turn search patterns of the form '???' into '*???*', and
		use this modified string to search. */
		if(!bUseRegularExpressions && lstrlen(szSearchPattern) > 0)
		{
			if(szSearchPattern[0] != '*' &&
				szSearchPattern[lstrlen(szSearchPattern) - 1] != '*')
			{

				TCHAR szTemp[MAX_PATH];

				StringCchPrintf(szTemp,SIZEOF_ARRAY(szTemp),_T("*%s*"),
					szSearchPattern);
				StringCchCopy(szSearchPattern,SIZEOF_ARRAY(szSearchPattern),
					szTemp);
			}
		}

		DWORD dwAttributes = 0;

		if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_ARCHIVE) == BST_CHECKED)
			dwAttributes |= FILE_ATTRIBUTE_ARCHIVE;

		if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_HIDDEN) == BST_CHECKED)
			dwAttributes |= FILE_ATTRIBUTE_HIDDEN;

		if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_READONLY) == BST_CHECKED)
			dwAttributes |= FILE_ATTRIBUTE_READONLY;

		if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_SYSTEM) == BST_CHECKED)
			dwAttributes |= FILE_ATTRIBUTE_SYSTEM;

		m_pSearch = new CSearch(m_hDlg,szBaseDirectory,szSearchPattern,
			dwAttributes,bUseRegularExpressions,bCaseInsensitive,bSearchSubFolders);

		/* Save the search directory and search pattern (only if they are not
		the same as the most recent entry). */
		BOOL bSaveEntry = FALSE;

		if(m_sdps->m_SearchDirectories.empty() ||
			lstrcmp(szBaseDirectory,
			m_sdps->m_SearchDirectories.begin()->c_str()) != 0)
		{
			bSaveEntry = TRUE;
		}

		if(bSaveEntry)
		{
			m_sdps->m_SearchDirectories.push_front(szBaseDirectory);

			HWND hComboBoxEx = GetDlgItem(m_hDlg,IDC_COMBO_DIRECTORY);
			HWND hComboBox = reinterpret_cast<HWND>(SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0));

			COMBOBOXEXITEM cbi;
			cbi.mask	= CBEIF_TEXT;
			cbi.iItem	= 0;
			cbi.pszText	= szBaseDirectory;
			SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,reinterpret_cast<LPARAM>(&cbi));

			ComboBox_SetCurSel(hComboBox,0);
		}

		bSaveEntry = FALSE;

		if(m_sdps->m_SearchPatterns.empty() ||
			lstrcmp(szSearchPattern,m_sdps->m_SearchPatterns.begin()->c_str()) != 0)
		{
			bSaveEntry = TRUE;
		}

		if(bSaveEntry)
		{
			m_sdps->m_SearchPatterns.push_front(szSearchPattern);

			HWND hComboBoxEx = GetDlgItem(m_hDlg,IDC_COMBO_NAME);
			HWND hComboBox = (HWND)SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0);

			COMBOBOXEXITEM cbi;
			cbi.mask	= CBEIF_TEXT;
			cbi.iItem	= 0;
			cbi.pszText	= szSearchPattern;
			SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,(LPARAM)&cbi);

			ComboBox_SetCurSel(hComboBox,0);
		}

		GetDlgItemText(m_hDlg,IDSEARCH,m_szSearchButton,SIZEOF_ARRAY(m_szSearchButton));

		TCHAR szTemp[64];

		LoadString(GetInstance(),IDS_STOP,szTemp,SIZEOF_ARRAY(szTemp));
		SetDlgItemText(m_hDlg,IDSEARCH,szTemp);

		m_bSearching = TRUE;

		/* Create a background thread, and search using it... */
		CreateThread(NULL,0,NSearchDialog::SearchThread,reinterpret_cast<LPVOID>(m_pSearch),
			0,NULL);
	}
	else
	{
		m_bStopSearching = TRUE;

		if(m_pSearch != NULL)
		{
			m_pSearch->StopSearching();
		}
	}
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

	switch(m_SortMode)
	{
	case SORT_NAME:
		iRes = SortResultsByName(lParam1,lParam2);
		break;

	case SORT_PATH:
		iRes = SortResultsByPath(lParam1,lParam2);
		break;
	}

	if(!m_bSortAscending)
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

void CSearchDialog::AddMenuEntries(LPITEMIDLIST pidlParent,
	std::list<LPITEMIDLIST> pidlItemList,DWORD_PTR dwData,HMENU hMenu)
{
	/* TODO: Move string into string table. */
	MENUITEMINFO mii;
	mii.cbSize		= sizeof(MENUITEMINFO);
	mii.fMask		= MIIM_STRING|MIIM_ID;
	mii.wID			= MENU_ID_OPEN_FILE_LOCATION;
	mii.dwTypeData	= _T("Open file location");
	InsertMenuItem(hMenu,1,TRUE,&mii);
}

BOOL CSearchDialog::HandleShellMenuItem(LPITEMIDLIST pidlParent,
	std::list<LPITEMIDLIST> pidlItemList,DWORD_PTR dwData,TCHAR *szCmd)
{
	if(StrCmpI(szCmd,_T("open")) == 0)
	{
		/* TODO: Open the item. */
		return TRUE;
	}

	return FALSE;
}

void CSearchDialog::HandleCustomMenuItem(LPITEMIDLIST pidlParent,
	std::list<LPITEMIDLIST> pidlItemList,int iCmd)
{
	switch(iCmd)
	{
	case MENU_ID_OPEN_FILE_LOCATION:
		{
			/* TODO: Browse to the parent folder,
			and select the specified item. */
			/*TCHAR szFileName[MAX_PATH];

			BrowseFolder(pidlParent,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);

			vector<LPITEMIDLIST> pidlItemVector(pidlItemList.begin(),pidlItemList.end());
			GetDisplayName(pidlItemVector[0],szFileName,SHGDN_INFOLDER|SHGDN_FORPARSING);

			m_pActiveShellBrowser->SelectFiles(szFileName);*/
		}
		break;
	}
}

BOOL CSearchDialog::OnNotify(NMHDR *pnmhdr)
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
						/* TODO: */
						//OpenItem(pidlFull,TRUE,FALSE);

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
							list<LPITEMIDLIST> pidlList;
							pidlList.push_back(ILFindLastID(pidlFull));

							LPITEMIDLIST pidlDirectory = ILClone(pidlFull);
							ILRemoveLastID(pidlDirectory);

							CFileContextMenuManager fcmm(m_hDlg,pidlDirectory,
								pidlList);

							DWORD dwCursorPos = GetMessagePos();

							POINT ptCursor;
							ptCursor.x = GET_X_LPARAM(dwCursorPos);
							ptCursor.y = GET_Y_LPARAM(dwCursorPos);

							fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,&ptCursor,NULL,
								FALSE,GetKeyState(VK_SHIFT) & 0x80);

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
			if(m_Columns[pnmlv->iSubItem].SearchMode == m_SortMode)
			{
				m_bSortAscending = !m_bSortAscending;
			}
			else
			{
				m_SortMode = m_Columns[pnmlv->iSubItem].SearchMode;
			}

			ListView_SortItems(GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS),
				NSearchDialog::SortResultsStub,reinterpret_cast<LPARAM>(this));
		}
		break;
	}

	return 0;
}

void CSearchDialog::OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		/* We won't actually process the item here. Instead, we'll
		add it onto the list of current items, which will be processed
		in batch. This is done to stop this message from blocking the
		main GUI (also see http://www.flounder.com/iocompletion.htm). */
		case NSearchDialog::WM_APP_SEARCHITEMFOUND:
			{
				m_SearchItems.push_back(reinterpret_cast<LPITEMIDLIST>(wParam));

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

					StringCchPrintf(szStatus,SIZEOF_ARRAY(szStatus),
						_T("Finished. %d folder(s) and %d file(s) found."),
						iFoldersFound,iFilesFound);
					SetDlgItemText(m_hDlg,IDC_STATIC_STATUS,szStatus);
				}
				else
				{
					SetDlgItemText(m_hDlg,IDC_STATIC_STATUS,_T("Cancelled."));

					if(m_bExit)
					{
						DestroyWindow(m_hDlg);
					}
				}

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

				pszDirectory = (TCHAR *)wParam;

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
				SetDlgItemText(m_hDlg,IDC_LINK_STATUS,_T("<a href=\"http://msdn.microsoft.com/en-us/library/bb982727.aspx\">\
					The form of the regular expression is incorrect</a>"));

				m_pSearch->Release();
				m_pSearch = NULL;

				m_bSearching = FALSE;
				m_bStopSearching = FALSE;
				SetDlgItemText(m_hDlg,IDSEARCH,m_szSearchButton);
			}
			break;
	}
}

BOOL CSearchDialog::OnTimer(int iTimerID)
{
	if(iTimerID != SEARCH_PROCESSITEMS_TIMER_ID)
	{
		return 1;
	}

	HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);
	int nListViewItems = ListView_GetItemCount(hListView);

	int nItems = min(static_cast<int>(m_SearchItems.size()),
		SEARCH_MAX_ITEMS_BATCH_PROCESS);
	int i = 0;

	auto itr = m_SearchItems.begin();

	while(i < nItems)
	{
		TCHAR szFullFileName[MAX_PATH];
		TCHAR szDirectory[MAX_PATH];
		TCHAR szFileName[MAX_PATH];
		LVITEM lvItem;
		SHFILEINFO shfi;
		int iIndex;

		LPITEMIDLIST pidl = *itr;

		GetDisplayName(pidl,szDirectory,SHGDN_FORPARSING);
		PathRemoveFileSpec(szDirectory);

		GetDisplayName(pidl,szFullFileName,SHGDN_FORPARSING);
		GetDisplayName(pidl,szFileName,SHGDN_INFOLDER|SHGDN_FORPARSING);

		SHGetFileInfo((LPCWSTR)pidl,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);

		m_SearchItemsMapInternal.insert(unordered_map<int,wstring>::value_type(m_iInternalIndex,
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

		itr = m_SearchItems.erase(itr);

		i++;
	}

	m_bSetSearchTimer = TRUE;

	return 0;
}

BOOL CSearchDialog::OnGetMinMaxInfo(LPMINMAXINFO pmmi)
{
	/* Set the minimum dialog size. */
	pmmi->ptMinTrackSize.x = m_iMinWidth;
	pmmi->ptMinTrackSize.y = m_iMinHeight;

	return 0;
}

BOOL CSearchDialog::OnSize(int iType,int iWidth,int iHeight)
{
	HWND hListView;
	RECT rc;

	GetWindowRect(m_hGripper,&rc);
	SetWindowPos(m_hGripper,NULL,iWidth - GetRectWidth(&rc),iHeight - GetRectHeight(&rc),0,
		0,SWP_NOSIZE|SWP_NOZORDER);

	hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);

	GetWindowRect(hListView,&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(hListView,NULL,0,0,iWidth - m_iListViewWidthDelta - rc.left,
		iHeight - m_iListViewHeightDelta - rc.top,SWP_NOMOVE|SWP_NOZORDER);

	GetWindowRect(GetDlgItem(m_hDlg,IDC_COMBO_DIRECTORY),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(GetDlgItem(m_hDlg,IDC_COMBO_DIRECTORY),NULL,0,0,
		iWidth - m_iSearchDirectoryWidthDelta - rc.left,GetRectHeight(&rc),SWP_NOMOVE|SWP_NOZORDER);

	GetWindowRect(GetDlgItem(m_hDlg,IDC_COMBO_NAME),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(GetDlgItem(m_hDlg,IDC_COMBO_NAME),NULL,0,0,
		iWidth - m_iNamedWidthDelta - rc.left,GetRectHeight(&rc),SWP_NOMOVE|SWP_NOZORDER);

	GetWindowRect(GetDlgItem(m_hDlg,IDC_BUTTON_DIRECTORY),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(GetDlgItem(m_hDlg,IDC_BUTTON_DIRECTORY),NULL,
		iWidth - m_iButtonDirectoryLeftDelta,rc.top,0,0,SWP_NOSIZE|SWP_NOZORDER);

	GetWindowRect(GetDlgItem(m_hDlg,IDC_STATIC_STATUSLABEL),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(GetDlgItem(m_hDlg,IDC_STATIC_STATUSLABEL),NULL,rc.left,iHeight - m_iStatusVerticalDelta,
		0,0,SWP_NOSIZE|SWP_NOZORDER);

	GetWindowRect(GetDlgItem(m_hDlg,IDC_STATIC_STATUS),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(GetDlgItem(m_hDlg,IDC_STATIC_STATUS),NULL,rc.left,iHeight - m_iStatusVerticalDelta,
		iWidth - m_iStaticStatusWidthDelta - rc.left,GetRectHeight(&rc),SWP_NOZORDER);

	GetWindowRect(GetDlgItem(m_hDlg,IDC_LINK_STATUS),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(GetDlgItem(m_hDlg,IDC_LINK_STATUS),NULL,rc.left,iHeight - m_iStatusVerticalDelta,
		iWidth - m_iStaticStatusWidthDelta - rc.left,GetRectHeight(&rc),SWP_NOZORDER);

	GetWindowRect(GetDlgItem(m_hDlg,IDC_STATIC_ETCHEDHORZ),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(GetDlgItem(m_hDlg,IDC_STATIC_ETCHEDHORZ),NULL,rc.left,iHeight - m_iEtchedHorzVerticalDelta,
		iWidth - m_iEtchedHorzWidthDelta - rc.left,GetRectHeight(&rc),SWP_NOZORDER);

	GetWindowRect(GetDlgItem(m_hDlg,IDEXIT),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(GetDlgItem(m_hDlg,IDEXIT),NULL,
		iWidth - m_iExitLeftDelta,iHeight - m_iExitVerticalDelta,0,0,SWP_NOSIZE|SWP_NOZORDER);

	GetWindowRect(GetDlgItem(m_hDlg,IDSEARCH),&rc);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
	SetWindowPos(GetDlgItem(m_hDlg,IDSEARCH),NULL,
		iWidth - m_iExitLeftDelta - m_iSearchExitDelta,iHeight - m_iExitVerticalDelta,0,0,SWP_NOSIZE|SWP_NOZORDER);

	return 0;
}

BOOL CSearchDialog::OnClose()
{
	if(m_bSearching)
	{
		m_bExit = TRUE;
		m_bStopSearching = TRUE;
	}
	else
	{
		DestroyWindow(m_hDlg);
	}

	return 0;
}

BOOL CSearchDialog::OnDestroy()
{
	/* Within WM_DESTROY, all child windows
	still exist. */
	SaveState();

	return 0;
}

BOOL CSearchDialog::OnNcDestroy()
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
				m_rxPattern.assign(m_szSearchPattern,regex_constants::icase);
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
}

void CSearch::SearchDirectory(const TCHAR *szDirectory)
{
	SendMessage(m_hDlg,NSearchDialog::WM_APP_SEARCHCHANGEDDIRECTORY,
		reinterpret_cast<WPARAM>(szDirectory),0);

	list<wstring> SubFolderList;

	SearchDirectoryInternal(szDirectory,&SubFolderList);

	EnterCriticalSection(&m_csStop);
	if(m_bStopSearching)
		return;
	LeaveCriticalSection(&m_csStop);

	if(m_bSearchSubFolders)
	{
		for each(auto strSubFolder in SubFolderList)
		{
			SearchDirectory(strSubFolder.c_str());
		}
	}
}

/* Can't recurse, as it would overflow the stack. */
void CSearch::SearchDirectoryInternal(const TCHAR *szSearchDirectory,
	list<wstring> *pSubFolderList)
{
	assert(szSearchDirectory != NULL);
	assert(pSubFolderList != NULL);

	WIN32_FIND_DATA wfd;
	TCHAR szSearchTerm[MAX_PATH];

	PathCombine(szSearchTerm,szSearchDirectory,_T("*"));

	HANDLE hFindFile = FindFirstFile(szSearchTerm,&wfd);

	if(hFindFile != INVALID_HANDLE_VALUE)
	{
		while(FindNextFile(hFindFile,&wfd) != 0)
		{
			EnterCriticalSection(&m_csStop);
			if(m_bStopSearching)
				break;
			LeaveCriticalSection(&m_csStop);

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
						if(regex_match(wfd.cFileName,m_rxPattern))
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
					if(wfd.dwFileAttributes & m_dwAttributes)
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
	RECT rcTemp;

	GetWindowRect(m_hDlg,&rcTemp);
	m_sdps->m_ptSearch.x = rcTemp.left;
	m_sdps->m_ptSearch.y = rcTemp.top;
	m_sdps->m_iSearchWidth = GetRectWidth(&rcTemp);
	m_sdps->m_iSearchHeight = GetRectHeight(&rcTemp);

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

CSearchDialogPersistentSettings::CSearchDialogPersistentSettings()
{
	m_bStateSaved = FALSE;

	/* Initialize all settings using default values. */
	m_bSearchSubFolders = TRUE;
	m_bUseRegularExpressions = FALSE;
	m_bCaseInsensitive = FALSE;
	m_bArchive = FALSE;
	m_bHidden = FALSE;
	m_bReadOnly = FALSE;
	m_bSystem = FALSE;
	m_iColumnWidth1 = -1;
	m_iColumnWidth1 = -2;

	StringCchCopy(m_szSearchPattern,SIZEOF_ARRAY(m_szSearchPattern),
		EMPTY_STRING);
}

CSearchDialogPersistentSettings::~CSearchDialogPersistentSettings()
{
	
}

CSearchDialogPersistentSettings& CSearchDialogPersistentSettings::GetInstance()
{
	static CSearchDialogPersistentSettings sdps;
	return sdps;
}

void CSearchDialogPersistentSettings::SaveSettings(HKEY hParentKey)
{
	if(!m_bStateSaved)
	{
		return;
	}

	HKEY hKey;
	DWORD dwDisposition;

	LONG lRes = RegCreateKeyEx(hParentKey,REGISTRY_SETTINGS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&dwDisposition);

	if(lRes == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey,_T("Position"),0,
			REG_BINARY,reinterpret_cast<LPBYTE>(&m_ptSearch),
			sizeof(m_ptSearch));

		SaveDwordToRegistry(hKey,_T("Width"),m_iSearchWidth);
		SaveDwordToRegistry(hKey,_T("Height"),m_iSearchHeight);
		SaveDwordToRegistry(hKey,_T("ColumnWidth1"),m_iColumnWidth1);
		SaveDwordToRegistry(hKey,_T("ColumnWidth2"),m_iColumnWidth2);
		SaveStringToRegistry(hKey,_T("SearchDirectoryText"),m_szSearchPattern);
		SaveDwordToRegistry(hKey,_T("SearchSubFolders"),m_bSearchSubFolders);
		SaveDwordToRegistry(hKey,_T("UseRegularExpressions"),m_bUseRegularExpressions);
		SaveDwordToRegistry(hKey,_T("CaseInsensitive"),m_bCaseInsensitive);
		SaveDwordToRegistry(hKey,_T("Archive"),m_bArchive);
		SaveDwordToRegistry(hKey,_T("Hidden"),m_bHidden);
		SaveDwordToRegistry(hKey,_T("ReadOnly"),m_bReadOnly);
		SaveDwordToRegistry(hKey,_T("System"),m_bSystem);

		TCHAR szItemKey[32];
		int i = 0;
		
		for each(auto strDirectory in m_SearchDirectories)
		{
			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),_T("Directory%d"),i++);
			SaveStringToRegistry(hKey,szItemKey,strDirectory.c_str());
		}

		i = 0;

		for each(auto strPattern in m_SearchPatterns)
		{
			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),_T("Pattern%d"),i++);
			SaveStringToRegistry(hKey,szItemKey,strPattern.c_str());
		}

		RegCloseKey(hKey);
	}
}

void CSearchDialogPersistentSettings::LoadSettings(HKEY hParentKey)
{
	HKEY hKey;
	DWORD dwSize;

	LONG lRes = RegOpenKeyEx(hParentKey,REGISTRY_SETTINGS_KEY,0,
		KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptSearch,&dwSize);

		ReadDwordFromRegistry(hKey,_T("Width"),reinterpret_cast<LPDWORD>(&m_iSearchWidth));
		ReadDwordFromRegistry(hKey,_T("Height"),reinterpret_cast<LPDWORD>(&m_iSearchHeight));
		ReadDwordFromRegistry(hKey,_T("ColumnWidth1"),reinterpret_cast<LPDWORD>(&m_iColumnWidth1));
		ReadDwordFromRegistry(hKey,_T("ColumnWidth2"),reinterpret_cast<LPDWORD>(&m_iColumnWidth2));
		ReadStringFromRegistry(hKey,_T("SearchDirectoryText"),m_szSearchPattern,SIZEOF_ARRAY(m_szSearchPattern));
		ReadDwordFromRegistry(hKey,_T("SearchSubFolders"),reinterpret_cast<LPDWORD>(&m_bSearchSubFolders));
		ReadDwordFromRegistry(hKey,_T("UseRegularExpressions"),reinterpret_cast<LPDWORD>(&m_bUseRegularExpressions));
		ReadDwordFromRegistry(hKey,_T("CaseInsensitive"),reinterpret_cast<LPDWORD>(&m_bCaseInsensitive));
		ReadDwordFromRegistry(hKey,_T("Archive"),reinterpret_cast<LPDWORD>(&m_bArchive));
		ReadDwordFromRegistry(hKey,_T("Hidden"),reinterpret_cast<LPDWORD>(&m_bHidden));
		ReadDwordFromRegistry(hKey,_T("ReadOnly"),reinterpret_cast<LPDWORD>(&m_bReadOnly));
		ReadDwordFromRegistry(hKey,_T("System"),reinterpret_cast<LPDWORD>(&m_bSystem));

		TCHAR szItemKey[32];
		TCHAR szTemp[512];
		int i = 0;

		lRes = ERROR_SUCCESS;

		while(lRes == ERROR_SUCCESS)
		{
			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("Directory%d"),i++);

			lRes = ReadStringFromRegistry(hKey,szItemKey,
				szTemp,SIZEOF_ARRAY(szTemp));

			if(lRes == ERROR_SUCCESS)
			{
				m_SearchDirectories.push_back(szTemp);
			}
		}

		i = 0;
		lRes = ERROR_SUCCESS;

		while(lRes == ERROR_SUCCESS)
		{
			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("Pattern%d"),i++);

			lRes = ReadStringFromRegistry(hKey,szItemKey,
				szTemp,SIZEOF_ARRAY(szTemp));

			if(lRes == ERROR_SUCCESS)
			{
				m_SearchPatterns.push_back(szTemp);
			}
		}

		m_bStateSaved = TRUE;

		RegCloseKey(hKey);
	}
}

void CSearchDialogPersistentSettings::SaveSettings(MSXML2::IXMLDOMDocument *pXMLDom,
	MSXML2::IXMLDOMElement *pe)
{
	if(!m_bStateSaved)
	{
		return;
	}

	MSXML2::IXMLDOMElement *pParentNode = NULL;
	BSTR bstr_wsntt = SysAllocString(L"\n\t\t");
	TCHAR szNode[64];

	AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);

	CreateElementNode(pXMLDom,&pParentNode,pe,_T("DialogState"),_T("Search"));

	int i = 0;

	for each(auto strDirectory in m_SearchDirectories)
	{
		StringCchPrintf(szNode,SIZEOF_ARRAY(szNode),_T("Directory%d"),i++);
		AddAttributeToNode(pXMLDom,pParentNode,szNode,strDirectory.c_str());
	}

	i = 0;

	for each(auto strPattern in m_SearchPatterns)
	{
		StringCchPrintf(szNode,SIZEOF_ARRAY(szNode),_T("Pattern%d"),i++);
		AddAttributeToNode(pXMLDom,pParentNode,szNode,strPattern.c_str());
	}

	AddAttributeToNode(pXMLDom,pParentNode,_T("PosX"),EncodeIntValue(m_ptSearch.x));
	AddAttributeToNode(pXMLDom,pParentNode,_T("PosY"),EncodeIntValue(m_ptSearch.y));
	AddAttributeToNode(pXMLDom,pParentNode,_T("Width"),EncodeIntValue(m_iSearchWidth));
	AddAttributeToNode(pXMLDom,pParentNode,_T("Height"),EncodeIntValue(m_iSearchHeight));
	AddAttributeToNode(pXMLDom,pParentNode,_T("ColumnWidth1"),EncodeIntValue(m_iColumnWidth1));
	AddAttributeToNode(pXMLDom,pParentNode,_T("ColumnWidth2"),EncodeIntValue(m_iColumnWidth2));
	AddAttributeToNode(pXMLDom,pParentNode,_T("SearchDirectoryText"),m_szSearchPattern);
	AddAttributeToNode(pXMLDom,pParentNode,_T("SearchSubFolders"),EncodeBoolValue(m_bSearchSubFolders));
	AddAttributeToNode(pXMLDom,pParentNode,_T("UseRegularExpressions"),EncodeBoolValue(m_bUseRegularExpressions));
	AddAttributeToNode(pXMLDom,pParentNode,_T("CaseInsensitive"),EncodeBoolValue(m_bCaseInsensitive));
	AddAttributeToNode(pXMLDom,pParentNode,_T("Archive"),EncodeBoolValue(m_bArchive));
	AddAttributeToNode(pXMLDom,pParentNode,_T("Hidden"),EncodeBoolValue(m_bHidden));
	AddAttributeToNode(pXMLDom,pParentNode,_T("ReadOnly"),EncodeBoolValue(m_bReadOnly));
	AddAttributeToNode(pXMLDom,pParentNode,_T("System"),EncodeBoolValue(m_bSystem));
}

void CSearchDialogPersistentSettings::LoadSettings(MSXML2::IXMLDOMNamedNodeMap *pam,
	long lChildNodes)
{
	MSXML2::IXMLDOMNode *pNode = NULL;
	BSTR bstrName;
	BSTR bstrValue;

	for(int i = 1;i < lChildNodes;i++)
	{
		pam->get_item(i,&pNode);

		pNode->get_nodeName(&bstrName);
		pNode->get_text(&bstrValue);

		if(lstrcmpi(bstrName,_T("PosX")) == 0)
		{
			m_ptSearch.x = DecodeIntValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("PosY")) == 0)
		{
			m_ptSearch.y = DecodeIntValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("Width")) == 0)
		{
			m_iSearchWidth = DecodeIntValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("Height")) == 0)
		{
			m_iSearchHeight = DecodeIntValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("ColumnWidth1")) == 0)
		{
			m_iColumnWidth1 = DecodeIntValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("ColumnWidth2")) == 0)
		{
			m_iColumnWidth2 = DecodeIntValue(bstrValue);
		}
		else if(CheckWildcardMatch(_T("Directory*"),bstrName,TRUE))
		{
			m_SearchDirectories.push_back(bstrValue);
		}
		else if(CheckWildcardMatch(_T("Pattern*"),bstrName,TRUE))
		{
			m_SearchPatterns.push_back(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("SearchDirectoryText")) == 0)
		{
			StringCchCopy(m_szSearchPattern,SIZEOF_ARRAY(m_szSearchPattern),bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("SearchSubFolders")) == 0)
		{
			m_bSearchSubFolders = DecodeBoolValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("UseRegularExpressions")) == 0)
		{
			m_bUseRegularExpressions = DecodeBoolValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("CaseInsensitive")) == 0)
		{
			m_bCaseInsensitive = DecodeBoolValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("Archive")) == 0)
		{
			m_bArchive = DecodeBoolValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("Hidden")) == 0)
		{
			m_bHidden = DecodeBoolValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("ReadOnly")) == 0)
		{
			m_bReadOnly = DecodeBoolValue(bstrValue);
		}
		else if(lstrcmpi(bstrName,_T("System")) == 0)
		{
			m_bSystem = DecodeBoolValue(bstrValue);
		}
	}

	m_bStateSaved = TRUE;
}