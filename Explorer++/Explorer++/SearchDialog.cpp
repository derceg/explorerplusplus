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
#include "SearchDialog.h"
#include "DialogHelper.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/XMLSettings.h"
#include "../Helper/Macros.h"


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

CSearchDialog::CSearchDialog(HINSTANCE hInstance,int iResource,
	HWND hParent,TCHAR *szSearchDirectory,IExplorerplusplus *pexpp) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	StringCchCopy(m_szSearchDirectory,SIZEOF_ARRAY(m_szSearchDirectory),
		szSearchDirectory);

	m_pexpp = pexpp;

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
	SetDlgItemText(m_hDlg,IDC_COMBO_NAME,m_sdps->m_szSearchPattern);
	SetDlgItemText(m_hDlg,IDC_COMBO_DIRECTORY,m_szSearchDirectory);

	HIMAGELIST himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himl,hBitmap,NULL);

	m_hDirectoryIcon = ImageList_GetIcon(himl,SHELLIMAGES_NEWTAB,ILD_NORMAL);
	m_hDialogIcon = ImageList_GetIcon(himl,SHELLIMAGES_SEARCH,ILD_NORMAL);

	SendMessage(GetDlgItem(m_hDlg,IDC_BUTTON_DIRECTORY),BM_SETIMAGE,
		IMAGE_ICON,reinterpret_cast<LPARAM>(m_hDirectoryIcon));

	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_hDialogIcon));

	DeleteObject(hBitmap);
	ImageList_Destroy(himl);

	HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);

	ListView_SetExtendedListViewStyleEx(hListView,LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER,
		LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	SetWindowTheme(hListView,L"Explorer",NULL);

	int i = 0;

	for each(auto ci in m_sdps->m_Columns)
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

	HWND hComboBoxEx = GetDlgItem(m_hDlg,IDC_COMBO_DIRECTORY);
	HWND hComboBox = reinterpret_cast<HWND>(SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0));

	int iItem = 0;

	for each(auto strDirectory in m_sdps->m_SearchDirectories)
	{
		TCHAR szDirectory[MAX_PATH];

		StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),strDirectory.c_str());

		COMBOBOXEXITEM cbi;
		cbi.mask	= CBEIF_TEXT;
		cbi.iItem	= iItem++;
		cbi.pszText	= szDirectory;
		SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,reinterpret_cast<LPARAM>(&cbi));
	}

	ComboBox_SetCurSel(hComboBox,0);

	hComboBoxEx = GetDlgItem(m_hDlg,IDC_COMBO_NAME);
	hComboBox = reinterpret_cast<HWND>(SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0));
	iItem = 0;

	for each(auto strPattern in *m_sdps->m_pSearchPatterns)
	{
		TCHAR szPattern[MAX_PATH];

		StringCchCopy(szPattern,SIZEOF_ARRAY(szPattern),strPattern.c_str());

		COMBOBOXEXITEM cbi;
		cbi.mask	= CBEIF_TEXT;
		cbi.iItem	= iItem++;
		cbi.pszText	= szPattern;
		SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,reinterpret_cast<LPARAM>(&cbi));
	}

	ComboBox_SetCurSel(hComboBox,0);

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

		m_AwaitingSearchItems.clear();
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
		m_pSearch->AddRef();

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
			/* TODO: Switch to circular buffer. */
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

		if(m_sdps->m_pSearchPatterns->empty() ||
			lstrcmp(szSearchPattern,m_sdps->m_pSearchPatterns->begin()->c_str()) != 0)
		{
			bSaveEntry = TRUE;
		}

		if(bSaveEntry)
		{
			TCHAR szSearchPatternOriginal[MAX_PATH];
			GetDlgItemText(m_hDlg,IDC_COMBO_NAME,szSearchPatternOriginal,
				SIZEOF_ARRAY(szSearchPatternOriginal));

			std::wstring strSearchPatternOriginal(szSearchPatternOriginal);
			auto itr = std::find_if(m_sdps->m_pSearchPatterns->begin(),m_sdps->m_pSearchPatterns->end(),
				[strSearchPatternOriginal](const std::wstring Pattern){return Pattern.compare(strSearchPatternOriginal) == 0;});

			HWND hComboBoxEx = GetDlgItem(m_hDlg,IDC_COMBO_NAME);
			HWND hComboBox = reinterpret_cast<HWND>(SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0));

			ComboBox_SetCurSel(hComboBox,-1);

			/* Remove the current element from both the list and the
			combo box. It will be reinserted at the front of both below. */
			if(itr != m_sdps->m_pSearchPatterns->end())
			{
				auto index = std::distance(m_sdps->m_pSearchPatterns->begin(),itr);
				SendMessage(hComboBoxEx,CBEM_DELETEITEM,index,0);

				m_sdps->m_pSearchPatterns->erase(itr);
			}

			m_sdps->m_pSearchPatterns->push_front(szSearchPatternOriginal);

			COMBOBOXEXITEM cbi;
			cbi.mask	= CBEIF_TEXT;
			cbi.iItem	= 0;
			cbi.pszText	= szSearchPatternOriginal;
			SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,reinterpret_cast<LPARAM>(&cbi));

			if(ComboBox_GetCount(hComboBox) > m_sdps->m_pSearchPatterns->capacity())
			{
				SendMessage(hComboBoxEx,CBEM_DELETEITEM,ComboBox_GetCount(hComboBox) - 1,0);
			}
		}

		GetDlgItemText(m_hDlg,IDSEARCH,m_szSearchButton,SIZEOF_ARRAY(m_szSearchButton));

		TCHAR szTemp[64];

		LoadString(GetInstance(),IDS_STOP,szTemp,SIZEOF_ARRAY(szTemp));
		SetDlgItemText(m_hDlg,IDSEARCH,szTemp);

		m_bSearching = TRUE;

		/* Create a background thread, and search using it... */
		HANDLE hThread = CreateThread(NULL,0,NSearchDialog::SearchThread,
			reinterpret_cast<LPVOID>(m_pSearch),0,NULL);
		CloseHandle(hThread);
	}
	else
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

	for each(auto ci in m_sdps->m_Columns)
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

void CSearchDialog::AddMenuEntries(LPITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu)
{
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

BOOL CSearchDialog::HandleShellMenuItem(LPITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,TCHAR *szCmd)
{
	if(StrCmpI(szCmd,_T("open")) == 0)
	{
		for each(auto pidlItem in pidlItemList)
		{
			LPITEMIDLIST pidlComplete = ILCombine(pidlParent,pidlItem);
			m_pexpp->OpenItem(pidlComplete,FALSE,FALSE);
			CoTaskMemFree(pidlComplete);
		}

		return TRUE;
	}

	return FALSE;
}

void CSearchDialog::HandleCustomMenuItem(LPITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,int iCmd)
{
	switch(iCmd)
	{
	case MENU_ID_OPEN_FILE_LOCATION:
		{
			m_pexpp->BrowseFolder(pidlParent,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);

			TCHAR szFilename[MAX_PATH];
			LPITEMIDLIST pidlComplete = ILCombine(pidlParent,pidlItemList.front());
			GetDisplayName(pidlComplete,szFilename,SHGDN_INFOLDER|SHGDN_FORPARSING);
			CoTaskMemFree(pidlComplete);

			m_pexpp->GetActiveShellBrowser()->SelectFiles(szFilename);
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
								NULL,FALSE,GetKeyState(VK_SHIFT) & 0x80);

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
}

BOOL CSearchDialog::OnTimer(int iTimerID)
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

		GetDisplayName(pidl,szDirectory,SHGDN_FORPARSING);
		PathRemoveFileSpec(szDirectory);

		GetDisplayName(pidl,szFullFileName,SHGDN_FORPARSING);
		GetDisplayName(pidl,szFileName,SHGDN_INFOLDER|SHGDN_FORPARSING);

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

BOOL CSearchDialog::OnClose()
{
	DestroyWindow(m_hDlg);

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
		for each(auto strSubFolder in SubFolderList)
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
						if(std::tr1::regex_match(wfd.cFileName,m_rxPattern))
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
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("ColumnWidth1"),m_iColumnWidth1);
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("ColumnWidth2"),m_iColumnWidth2);
	NRegistrySettings::SaveStringToRegistry(hKey,_T("SearchDirectoryText"),m_szSearchPattern);
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("SearchSubFolders"),m_bSearchSubFolders);
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("UseRegularExpressions"),m_bUseRegularExpressions);
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("CaseInsensitive"),m_bCaseInsensitive);
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("Archive"),m_bArchive);
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("Hidden"),m_bHidden);
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("ReadOnly"),m_bReadOnly);
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("System"),m_bSystem);
	NRegistrySettings::SaveStringListToRegistry(hKey,_T("Directory"),m_SearchDirectories);

	std::list<std::wstring> SearchPatternList;
	CircularBufferToList(*m_pSearchPatterns,SearchPatternList);
	NRegistrySettings::SaveStringListToRegistry(hKey,_T("Pattern"),SearchPatternList);

	NRegistrySettings::SaveDwordToRegistry(hKey,_T("SortMode"),m_SortMode);
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("SortAscending"),m_bSortAscending);
}

void CSearchDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("ColumnWidth1"),reinterpret_cast<LPDWORD>(&m_iColumnWidth1));
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("ColumnWidth2"),reinterpret_cast<LPDWORD>(&m_iColumnWidth2));
	NRegistrySettings::ReadStringFromRegistry(hKey,_T("SearchDirectoryText"),m_szSearchPattern,SIZEOF_ARRAY(m_szSearchPattern));
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("SearchSubFolders"),reinterpret_cast<LPDWORD>(&m_bSearchSubFolders));
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("UseRegularExpressions"),reinterpret_cast<LPDWORD>(&m_bUseRegularExpressions));
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("CaseInsensitive"),reinterpret_cast<LPDWORD>(&m_bCaseInsensitive));
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("Archive"),reinterpret_cast<LPDWORD>(&m_bArchive));
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("Hidden"),reinterpret_cast<LPDWORD>(&m_bHidden));
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("ReadOnly"),reinterpret_cast<LPDWORD>(&m_bReadOnly));
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("System"),reinterpret_cast<LPDWORD>(&m_bSystem));
	NRegistrySettings::ReadStringListFromRegistry(hKey,_T("Directory"),m_SearchDirectories);

	std::list<std::wstring> SearchPatternList;
	NRegistrySettings::ReadStringListFromRegistry(hKey,_T("Pattern"),SearchPatternList);
	ListToCircularBuffer(SearchPatternList,*m_pSearchPatterns);

	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("SortMode"),reinterpret_cast<LPDWORD>(&m_SortMode));
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("SortAscending"),reinterpret_cast<LPDWORD>(&m_bSortAscending));
}

void CSearchDialogPersistentSettings::SaveExtraXMLSettings(
	MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ColumnWidth1"),NXMLSettings::EncodeIntValue(m_iColumnWidth1));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ColumnWidth2"),NXMLSettings::EncodeIntValue(m_iColumnWidth2));
	NXMLSettings::AddStringListToNode(pXMLDom,pParentNode,_T("Directory"),m_SearchDirectories);

	std::list<std::wstring> SearchPatternList;
	CircularBufferToList(*m_pSearchPatterns,SearchPatternList);
	NXMLSettings::AddStringListToNode(pXMLDom,pParentNode,_T("Pattern"),SearchPatternList);

	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("SearchDirectoryText"),m_szSearchPattern);
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("SearchSubFolders"),NXMLSettings::EncodeBoolValue(m_bSearchSubFolders));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("UseRegularExpressions"),NXMLSettings::EncodeBoolValue(m_bUseRegularExpressions));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("CaseInsensitive"),NXMLSettings::EncodeBoolValue(m_bCaseInsensitive));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Archive"),NXMLSettings::EncodeBoolValue(m_bArchive));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Hidden"),NXMLSettings::EncodeBoolValue(m_bHidden));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ReadOnly"),NXMLSettings::EncodeBoolValue(m_bReadOnly));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("System"),NXMLSettings::EncodeBoolValue(m_bSystem));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("SortMode"),NXMLSettings::EncodeIntValue(m_SortMode));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("SortAscending"),NXMLSettings::EncodeBoolValue(m_bSortAscending));
}

void CSearchDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(lstrcmpi(bstrName,_T("ColumnWidth1")) == 0)
	{
		m_iColumnWidth1 = NXMLSettings::DecodeIntValue(bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("ColumnWidth2")) == 0)
	{
		m_iColumnWidth2 = NXMLSettings::DecodeIntValue(bstrValue);
	}
	else if(CheckWildcardMatch(_T("Directory*"),bstrName,TRUE))
	{
		m_SearchDirectories.push_back(bstrValue);
	}
	else if(CheckWildcardMatch(_T("Pattern*"),bstrName,TRUE))
	{
		m_pSearchPatterns->push_back(bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("SearchDirectoryText")) == 0)
	{
		StringCchCopy(m_szSearchPattern,SIZEOF_ARRAY(m_szSearchPattern),bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("SearchSubFolders")) == 0)
	{
		m_bSearchSubFolders = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("UseRegularExpressions")) == 0)
	{
		m_bUseRegularExpressions = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("CaseInsensitive")) == 0)
	{
		m_bCaseInsensitive = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("Archive")) == 0)
	{
		m_bArchive = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("Hidden")) == 0)
	{
		m_bHidden = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("ReadOnly")) == 0)
	{
		m_bReadOnly = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("System")) == 0)
	{
		m_bSystem = NXMLSettings::DecodeBoolValue(bstrValue);
	}
	else if(lstrcmpi(bstrName,_T("SortMode")) == 0)
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
	else if(lstrcmpi(bstrName,_T("SortAscending")) == 0)
	{
		m_bSortAscending = NXMLSettings::DecodeBoolValue(bstrValue);
	}
}

template <typename T>
void CSearchDialogPersistentSettings::CircularBufferToList(const boost::circular_buffer<T> &cb,
	std::list<T> &list)
{
	for each(auto Item in cb)
	{
		list.push_back(Item);
	}
}

template <typename T>
void CSearchDialogPersistentSettings::ListToCircularBuffer(const std::list<T> &list,
	boost::circular_buffer<T> &cb)
{
	for each(auto Item in list)
	{
		cb.push_back(Item);
	}
}