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
#include "Explorer++.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/FileContextMenuManager.h"
#include "SearchDialog.h"
#include "MainResource.h"

using namespace std;


#define SEARCH_PROCESSITEMS_TIMER_ID		0
#define SEARCH_PROCESSITEMS_TIMER_ELAPSED	50
#define SEARCH_MAX_ITEMS_BATCH_PROCESS		100

typedef struct
{
	TCHAR szFullFileName[MAX_PATH];
} DirectoryInfo_t;

typedef struct
{
	TCHAR szFullFileName[MAX_PATH];
} SearchItem_t;

//SearchItem_t *g_pSearchItems = NULL;
//
//DWORD WINAPI SearchThread(LPVOID pParam);

int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);

/* Singleton class that holds settings
by the search dialog. */
class SearchDialogSettings
{
public:

	~SearchDialogSettings();

	static SearchDialogSettings &GetInstance();

private:

	SearchDialogSettings();

	SearchDialogSettings(const SearchDialogSettings &);
	SearchDialogSettings & operator=(const SearchDialogSettings &);
};

SearchDialogSettings::SearchDialogSettings()
{
	/* Initialize the settings. Either read them
	from somewhere, or initialize them to default
	values. */
}

SearchDialogSettings::~SearchDialogSettings()
{

}

SearchDialogSettings& SearchDialogSettings::GetInstance()
{
	static SearchDialogSettings sds;
	return sds;
}

/* Provides an interface to save to various
destinations (e.g. xml file, the registry). */
interface ISaveProvidor
{
	/* Registry/XML file. */
	void SetType();

	/* Overloaded - e.g. one method for int,
	one method for strings, etc. */
	void SaveValue();
};

CSearchDialog::CSearchDialog(HINSTANCE hInstance,int iResource,
	HWND hParent,TCHAR *szSearchDirectory) :
CBaseDialog(hInstance,iResource,hParent)
{
	m_bSearching = FALSE;
	m_iFoldersFound = 0;
	m_iFilesFound = 0;
	m_bExit = FALSE;
	m_bSetSearchTimer = TRUE;
	m_bSearchSubFolders = TRUE;

	m_bSearchDlgStateSaved = FALSE;

	StringCchCopy(m_szSearchDirectory,SIZEOF_ARRAY(m_szSearchDirectory),
		szSearchDirectory);

	StringCchCopy(m_SearchPatternText,SIZEOF_ARRAY(m_SearchPatternText),
		EMPTY_STRING);
}

CSearchDialog::~CSearchDialog()
{
	DestroyIcon(m_hDialogIcon);
	DestroyIcon(m_hDirectoryIcon);
}

BOOL CSearchDialog::OnInitDialog()
{
	HWND hListView;
	HWND hComboBoxEx;
	HWND hComboBox;
	list<SearchDirectoryInfo_t>::iterator itr;
	list<SearchPatternInfo_t>::iterator itr2;
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

	if(m_bSearchSubFolders)
		CheckDlgButton(m_hDlg,IDC_CHECK_SEARCHSUBFOLDERS,BST_CHECKED);

	if(!m_SearchDirectories.empty())
	{
		COMBOBOXEXITEM cbi;
		int iItem = 0;

		hComboBoxEx = GetDlgItem(m_hDlg,IDC_COMBO_DIRECTORY);
		hComboBox = (HWND)SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0);

		for(itr = m_SearchDirectories.begin();itr != m_SearchDirectories.end();itr++)
		{
			cbi.mask	= CBEIF_TEXT;
			cbi.iItem	= iItem++;
			cbi.pszText	= itr->szDirectory;
			SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,(LPARAM)&cbi);
		}

		ComboBox_SetCurSel(hComboBox,0);
	}

	if(!m_SearchPatterns.empty())
	{
		COMBOBOXEXITEM cbi;
		int iItem = 0;

		hComboBoxEx = GetDlgItem(m_hDlg,IDC_COMBO_NAME);
		hComboBox = (HWND)SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0);

		for(itr2 = m_SearchPatterns.begin();itr2 != m_SearchPatterns.end();itr2++)
		{
			cbi.mask	= CBEIF_TEXT;
			cbi.iItem	= iItem++;
			cbi.pszText	= itr2->szPattern;
			SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,(LPARAM)&cbi);
		}

		ComboBox_SetCurSel(hComboBox,0);
	}

	SetDlgItemText(m_hDlg,IDC_COMBO_NAME,m_SearchPatternText);

	m_hGripper = CreateWindow(_T("SCROLLBAR"),EMPTY_STRING,WS_CHILD|WS_VISIBLE|
		WS_CLIPSIBLINGS|SBS_BOTTOMALIGN|SBS_SIZEGRIP,0,0,0,0,m_hDlg,NULL,
		GetModuleHandle(0),NULL);

	GetClientRect(m_hDlg,&rcMain);
	GetWindowRect(m_hGripper,&rc);
	SetWindowPos(m_hGripper,NULL,GetRectWidth(&rcMain) - GetRectWidth(&rc),
		GetRectHeight(&rcMain) - GetRectHeight(&rc),0,0,SWP_NOSIZE|SWP_NOZORDER);




	if(m_bSearchDlgStateSaved)
	{
		/* These dummy values will be in use if these values
		have not previously been saved. */
		if(m_iColumnWidth1 != -1 && m_iColumnWidth2 != -1)
		{
			ListView_SetColumnWidth(hListView,0,m_iColumnWidth1);
			ListView_SetColumnWidth(hListView,1,m_iColumnWidth2);
		}

		SetWindowPos(m_hDlg,HWND_TOP,m_ptSearch.x,m_ptSearch.y,
			m_iSearchWidth,m_iSearchHeight,SWP_SHOWWINDOW);
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
		OnSearch(m_hDlg);
		break;

	case IDC_BUTTON_DIRECTORY:
		{
			BROWSEINFO bi;
			PIDLIST_ABSOLUTE pidl = NULL;
			TCHAR szDisplayName[MAX_PATH];
			TCHAR szParsingPath[MAX_PATH];
			TCHAR szTitle[] = _T("Select a folder to search, then press OK");

			GetDlgItemText(m_hDlg,IDC_COMBO_DIRECTORY,m_szSearch,SIZEOF_ARRAY(m_szSearch));

			CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);

			bi.hwndOwner		= m_hDlg;
			bi.pidlRoot			= NULL;
			bi.pszDisplayName	= szDisplayName;
			bi.lpszTitle		= szTitle;
			bi.ulFlags			= BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
			bi.lpfn				= BrowseCallbackProc;

			pidl = SHBrowseForFolder(&bi);

			CoUninitialize();

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
			SearchSaveState(m_hDlg);
			DestroyWindow(m_hDlg);
		}
		break;

	case IDCANCEL:
		DestroyWindow(m_hDlg);
		break;
	}

	return 0;
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
					/* TODO : */
					/*LPITEMIDLIST pidlFull = NULL;
					TCHAR szDirectory[MAX_PATH];

					StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),
						g_pSearchItems[(int)lvItem.lParam].szFullFileName);
					GetIdlFromParsingName(szDirectory,&pidlFull);

					OpenItem(pidlFull,TRUE,FALSE);

					CoTaskMemFree(pidlFull);*/
				}
			}
		}
		break;

	case NM_RCLICK:
		{
			if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS))
			{
				HWND hListView;
				LVITEM lvItem;
				LPITEMIDLIST pidlDirectory = NULL;
				LPITEMIDLIST pidlFull = NULL;
				LPITEMIDLIST pidl = NULL;
				TCHAR szDirectory[MAX_PATH];
				POINTS ptsCursor;
				POINT ptCursor;
				DWORD dwCursorPos;
				BOOL bRet;
				int iSelected;

				hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_SEARCHRESULTS);

				iSelected = ListView_GetNextItem(hListView,-1,LVNI_ALL|LVNI_SELECTED);

				if(iSelected != -1)
				{
					lvItem.mask		= LVIF_PARAM;
					lvItem.iItem	= iSelected;
					lvItem.iSubItem	= 0;
					bRet = ListView_GetItem(hListView,&lvItem);

					if(bRet)
					{
						/* TODO: */
						/*StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),
							g_pSearchItems[(int)lvItem.lParam].szFullFileName);*/
						GetIdlFromParsingName(szDirectory,&pidlFull);
						PathRemoveFileSpec(szDirectory);
						GetIdlFromParsingName(szDirectory,&pidlDirectory);

						pidl = ILFindLastID(pidlFull);

						dwCursorPos = GetMessagePos();
						ptsCursor = MAKEPOINTS(dwCursorPos);

						ptCursor.x = ptsCursor.x;
						ptCursor.y = ptsCursor.y;

						list<LPITEMIDLIST> pidlList;

						pidlList.push_back(pidl);

						CFileContextMenuManager fcmm(m_hDlg,pidlDirectory,
							pidlList);

						/* TODO: IFileContextMenuExternal interface. */
						//fcmm.ShowMenu(NULL,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,&ptCursor);

						CoTaskMemFree(pidlDirectory);
						CoTaskMemFree(pidlFull);
					}
				}
			}
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
		case WM_APP_SEARCHITEMFOUND:
			{
				m_SearchItems.push_back((LPITEMIDLIST)wParam);

				if(m_bSetSearchTimer)
				{
					SetTimer(m_hDlg,SEARCH_PROCESSITEMS_TIMER_ID,
						SEARCH_PROCESSITEMS_TIMER_ELAPSED,NULL);

					m_bSetSearchTimer = FALSE;
				}
			}
			break;

		case WM_APP_SEARCHFINISHED:
			{
				TCHAR szStatus[512];

				if(!m_bStopSearching)
				{
					StringCchPrintf(szStatus,SIZEOF_ARRAY(szStatus),
						_T("Finished. %d folder(s) and %d file(s) found."),m_iFoldersFound,
						m_iFilesFound);
					SetDlgItemText(m_hDlg,IDC_STATIC_STATUS,szStatus);
				}
				else
				{
					SetDlgItemText(m_hDlg,IDC_STATIC_STATUS,_T("Cancelled."));

					if(m_bExit)
					{
						SearchSaveState(m_hDlg);
						DestroyWindow(m_hDlg);
					}
				}

				m_bSearching = FALSE;
				m_bStopSearching = FALSE;

				SetDlgItemText(m_hDlg,IDSEARCH,m_szSearchButton);
			}
			break;

		case WM_APP_SEARCHCHANGEDDIRECTORY:
			{
				TCHAR szStatus[512];
				TCHAR *pszDirectory = NULL;

				pszDirectory = (TCHAR *)wParam;

				TCHAR szTemp[64];

				LoadString(g_hLanguageModule,IDS_SEARCHING,
					szTemp,SIZEOF_ARRAY(szTemp));
				StringCchPrintf(szStatus,SIZEOF_ARRAY(szStatus),szTemp,
					pszDirectory);
				SetDlgItemText(m_hDlg,IDC_STATIC_STATUS,szStatus);
			}
			break;
	}
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
		SearchSaveState(m_hDlg);
		DestroyWindow(m_hDlg);
	}

	return 0;
}

BOOL CSearchDialog::OnNcDestroy()
{
	/* TODO: */
	//g_hwndSearch = NULL;

	delete this;

	return 0;
}

//case WM_TIMER:
//	{
//		if((int)wParam == SEARCH_PROCESSITEMS_TIMER_ID)
//		{
//			list<LPITEMIDLIST>::iterator itr;
//			int nItems = min((int)m_SearchItems.size(),SEARCH_MAX_ITEMS_BATCH_PROCESS);
//			int i = 0;
//
//			itr = m_SearchItems.begin();
//
//			while(i < nItems)
//			{
//				HWND hListView;
//				LPITEMIDLIST pidl = NULL;
//				TCHAR szFullFileName[MAX_PATH];
//				TCHAR szDirectory[MAX_PATH];
//				TCHAR szFileName[MAX_PATH];
//				LVITEM lvItem;
//				SHFILEINFO shfi;
//				int iIndex;
//				static int iItem = 0;
//				int iInternalIndex = 0;
//
//				pidl = *itr;
//
//				hListView = GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS);
//
//				GetDisplayName(pidl,szDirectory,SHGDN_FORPARSING);
//				PathRemoveFileSpec(szDirectory);
//
//				GetDisplayName(pidl,szFullFileName,SHGDN_FORPARSING);
//				GetDisplayName(pidl,szFileName,SHGDN_INFOLDER|SHGDN_FORPARSING);
//
//				SHGetFileInfo((LPCWSTR)pidl,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);
//
//				while(g_pMap[iInternalIndex] != 0 && iInternalIndex < g_nSearchItemsAllocated)
//				{
//					iInternalIndex++;
//				}
//
//				if(iInternalIndex == g_nSearchItemsAllocated)
//				{
//					g_pSearchItems = (SearchItem_t *)realloc(g_pSearchItems,
//						(g_nSearchItemsAllocated + DEFAULT_SEARCH_ALLOCATION) * sizeof(SearchItem_t));
//					g_pMap = (int *)realloc(g_pMap,
//						(g_nSearchItemsAllocated + DEFAULT_SEARCH_ALLOCATION) * sizeof(int));
//					g_nSearchItemsAllocated += DEFAULT_SEARCH_ALLOCATION;
//
//					iInternalIndex++;
//				}
//
//				StringCchCopy(g_pSearchItems[iInternalIndex].szFullFileName,
//					SIZEOF_ARRAY(g_pSearchItems[iInternalIndex].szFullFileName),
//					szFullFileName);
//				g_pMap[iInternalIndex] = 1;
//
//				lvItem.mask		= LVIF_IMAGE|LVIF_TEXT|LVIF_PARAM;
//				lvItem.pszText	= szFileName;
//				lvItem.iItem	= iItem++;
//				lvItem.iSubItem	= 0;
//				lvItem.iImage	= shfi.iIcon;
//				lvItem.lParam	= iInternalIndex;
//				iIndex = ListView_InsertItem(hListView,&lvItem);
//
//				ListView_SetItemText(hListView,iIndex,1,szDirectory);
//
//				CoTaskMemFree(pidl);
//
//				itr = m_SearchItems.erase(itr);
//
//				i++;
//			}
//
//			m_bSetSearchTimer = TRUE;
//		}
//	}
//	break;

int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		/* TODO: */
		//SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)m_szSearch);
		break;
	}

	return 0;
}

void SearchDirectory(SearchInfo_t *psi,TCHAR *szDirectory);
void SearchDirectoryInternal(HWND hDlg,TCHAR *szSearchDirectory,
TCHAR *szSearchPattern,DWORD dwAttributes,list<DirectoryInfo_t> *pSubfolderList);

DWORD WINAPI SearchThread(LPVOID pParam)
{
	/*SearchInfo_t *psi = reinterpret_cast<SearchInfo_t *>(pParam);

	SearchDirectory(psi,psi->szDirectory);

	SendMessage(psi->hDlg,WM_APP_SEARCHFINISHED,0,0);

	free(psi);*/

	return 0;
}

void SearchDirectory(SearchInfo_t *psi,TCHAR *szDirectory)
{
	/*list<DirectoryInfo_t> SubfolderList;
	list<DirectoryInfo_t>::iterator itr;

	SendMessage(psi->hDlg,WM_APP_SEARCHCHANGEDDIRECTORY,
		(WPARAM)szDirectory,0);

	SearchDirectoryInternal(psi->hDlg,szDirectory,
		psi->szName,psi->dwAttributes,&SubfolderList);

	if(m_bStopSearching)
		return;

	if(psi->bSearchSubFolders)
	{
		if(!SubfolderList.empty())
		{
			for(itr = SubfolderList.begin();itr != SubfolderList.end();itr++)
			{
				SearchDirectory(psi,itr->szFullFileName);
			}
		}
	}*/
}

/* Can't recurse, as it would overflow the stack. */
void SearchDirectoryInternal(HWND hDlg,TCHAR *szSearchDirectory,
TCHAR *szSearchPattern,DWORD dwAttributes,list<DirectoryInfo_t> *pSubfolderList)
{
	//WIN32_FIND_DATA wfd;
	//HANDLE hFindFile;
	//TCHAR szSearchTerm[MAX_PATH];
	//DirectoryInfo_t di;

	//PathCombine(szSearchTerm,szSearchDirectory,_T("*"));

	//hFindFile = FindFirstFile(szSearchTerm,&wfd);

	//if(hFindFile != INVALID_HANDLE_VALUE)
	//{
	//	while(FindNextFile(hFindFile,&wfd) != 0)
	//	{
	//		if(m_bStopSearching)
	//			break;

	//		if(lstrcmpi(wfd.cFileName,_T(".")) != 0 &&
	//			lstrcmpi(wfd.cFileName,_T("..")) != 0)
	//		{
	//			BOOL bFileNameActive = FALSE;
	//			BOOL bAttributesActive = FALSE;
	//			BOOL bMatchFileName = FALSE;
	//			BOOL bMatchAttributes = FALSE;
	//			BOOL bItemMatch = FALSE;

	//			/* Only match against the filename if it's not empty. */
	//			if(lstrcmp(szSearchPattern,EMPTY_STRING) != 0)
	//			{
	//				bFileNameActive = TRUE;

	//				if(CheckWildcardMatch(szSearchPattern,wfd.cFileName,FALSE))
	//				{
	//					bMatchFileName = TRUE;
	//				}
	//			}

	//			if(dwAttributes != 0)
	//			{
	//				bAttributesActive = TRUE;

	//				if(wfd.dwFileAttributes & dwAttributes)
	//				{
	//					bMatchAttributes = TRUE;
	//				}
	//			}

	//			if(bFileNameActive && bAttributesActive)
	//				bItemMatch = bMatchFileName && bMatchAttributes;
	//			else if(bFileNameActive)
	//				bItemMatch = bMatchFileName;
	//			else if(bAttributesActive)
	//				bItemMatch = bMatchAttributes;

	//			if(bItemMatch)
	//			{
	//				if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
	//					FILE_ATTRIBUTE_DIRECTORY)
	//					m_iFoldersFound++;
	//				else
	//					m_iFilesFound++;

	//				LPITEMIDLIST pidl = NULL;
	//				TCHAR szFullFileName[MAX_PATH];

	//				PathCombine(szFullFileName,szSearchDirectory,wfd.cFileName);
	//				GetIdlFromParsingName(szFullFileName,&pidl);

	//				PostMessage(hDlg,WM_APP_SEARCHITEMFOUND,(WPARAM)ILClone(pidl),0);

	//				CoTaskMemFree(pidl);
	//			}

	//			/* If this item is a folder, follow it. */
	//			if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
	//				FILE_ATTRIBUTE_DIRECTORY)
	//			{
	//				PathCombine(di.szFullFileName,szSearchDirectory,
	//					wfd.cFileName);
	//				pSubfolderList->push_back(di);
	//			}
	//		}
	//	}

	//	FindClose(hFindFile);
	//}
}

void CSearchDialog::OnSearch(HWND hDlg)
{
	//SearchInfo_t *psi = NULL;
	//SearchDirectoryInfo_t sdi;
	//SearchPatternInfo_t spi;
	//TCHAR szPattern[MAX_PATH];

	//m_SearchItems.clear();

	//if(!m_bSearching)
	//{
	//	psi = (SearchInfo_t *)malloc(sizeof(SearchInfo_t));

	//	if(psi != NULL)
	//	{
	//		ListView_DeleteAllItems(GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS));

	//		m_iFoldersFound = 0;
	//		m_iFilesFound = 0;

	//		/* Get the directory and name, and remove leading and
	//		trailing whitespace. */
	//		GetDlgItemText(hDlg,IDC_COMBO_DIRECTORY,psi->szDirectory,
	//			SIZEOF_ARRAY(psi->szDirectory));
	//		PathRemoveBlanks(psi->szDirectory);
	//		GetDlgItemText(hDlg,IDC_COMBO_NAME,psi->szName,
	//			SIZEOF_ARRAY(psi->szName));
	//		PathRemoveBlanks(psi->szName);

	//		//psi->pContainer = this;
	//		psi->hDlg = hDlg;
	//		psi->hListView = GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS);
	//		psi->bSearchSubFolders = IsDlgButtonChecked(hDlg,IDC_CHECK_SEARCHSUBFOLDERS) ==
	//			BST_CHECKED;

	//		psi->dwAttributes = 0;

	//		if(IsDlgButtonChecked(hDlg,IDC_CHECK_ARCHIVE) == BST_CHECKED)
	//			psi->dwAttributes |= FILE_ATTRIBUTE_ARCHIVE;

	//		if(IsDlgButtonChecked(hDlg,IDC_CHECK_HIDDEN) == BST_CHECKED)
	//			psi->dwAttributes |= FILE_ATTRIBUTE_HIDDEN;

	//		if(IsDlgButtonChecked(hDlg,IDC_CHECK_READONLY) == BST_CHECKED)
	//			psi->dwAttributes |= FILE_ATTRIBUTE_READONLY;

	//		if(IsDlgButtonChecked(hDlg,IDC_CHECK_SYSTEM) == BST_CHECKED)
	//			psi->dwAttributes |= FILE_ATTRIBUTE_SYSTEM;

	//		/* Save the search directory and search pattern (only if they are not
	//		the same as the most recent entry). */
	//		BOOL bSaveEntry;

	//		if(m_SearchDirectories.empty())
	//			bSaveEntry = TRUE;
	//		else if(lstrcmp(psi->szDirectory,m_SearchDirectories.begin()->szDirectory) != 0)
	//			bSaveEntry = TRUE;
	//		else
	//			bSaveEntry = FALSE;

	//		if(bSaveEntry)
	//		{
	//			StringCchCopy(sdi.szDirectory,SIZEOF_ARRAY(sdi.szDirectory),psi->szDirectory);
	//			m_SearchDirectories.push_front(sdi);

	//			/* Insert the entry into the combobox. */
	//			HWND hComboBoxEx;
	//			HWND hComboBox;
	//			COMBOBOXEXITEM cbi;

	//			hComboBoxEx = GetDlgItem(hDlg,IDC_COMBO_DIRECTORY);
	//			hComboBox = (HWND)SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0);

	//			cbi.mask	= CBEIF_TEXT;
	//			cbi.iItem	= 0;
	//			cbi.pszText	= psi->szDirectory;
	//			SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,(LPARAM)&cbi);

	//			ComboBox_SetCurSel(hComboBox,0);
	//		}

	//		if(m_SearchPatterns.empty())
	//			bSaveEntry = TRUE;
	//		else if(lstrcmp(psi->szName,m_SearchPatterns.begin()->szPattern) != 0)
	//			bSaveEntry = TRUE;
	//		else
	//			bSaveEntry = FALSE;

	//		if(bSaveEntry)
	//		{
	//			StringCchCopy(spi.szPattern,SIZEOF_ARRAY(spi.szPattern),psi->szName);
	//			m_SearchPatterns.push_front(spi);

	//			/* Insert the entry into the combobox. */
	//			HWND hComboBoxEx;
	//			HWND hComboBox;
	//			COMBOBOXEXITEM cbi;

	//			hComboBoxEx = GetDlgItem(hDlg,IDC_COMBO_NAME);
	//			hComboBox = (HWND)SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0);

	//			cbi.mask	= CBEIF_TEXT;
	//			cbi.iItem	= 0;
	//			cbi.pszText	= psi->szName;
	//			SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,(LPARAM)&cbi);

	//			ComboBox_SetCurSel(hComboBox,0);
	//		}

	//		/* Turn search patterns of the form '???' into '*???*', and
	//		use this modified string to search. */
	//		if(lstrlen(psi->szName) > 0)
	//		{
	//			StringCchCopy(szPattern,SIZEOF_ARRAY(szPattern),psi->szName);
	//			memset(psi->szName,0,SIZEOF_ARRAY(psi->szName));

	//			if(szPattern[0] != '*')
	//			{
	//				StringCchCat(psi->szName,SIZEOF_ARRAY(psi->szName),_T("*"));
	//			}

	//			StringCchCat(psi->szName,SIZEOF_ARRAY(psi->szName),szPattern);

	//			if(szPattern[lstrlen(szPattern) - 1] != '*')
	//			{
	//				StringCchCat(psi->szName,SIZEOF_ARRAY(psi->szName),_T("*"));
	//			}
	//		}

	//		GetDlgItemText(hDlg,IDSEARCH,m_szSearchButton,SIZEOF_ARRAY(m_szSearchButton));

	//		TCHAR szTemp[64];

	//		LoadString(g_hLanguageModule,IDS_STOP,
	//			szTemp,SIZEOF_ARRAY(szTemp));
	//		SetDlgItemText(hDlg,IDSEARCH,szTemp);

	//		m_bSearching = TRUE;

	//		/* Create a background thread, and search using it... */
	//		CreateThread(NULL,0,SearchThread,(LPVOID)psi,0,NULL);
	//	}
	//}
	//else
	//{
	//	m_bStopSearching = TRUE;
	//}
}

/* TODO: Singleton class to save dialog details? */
void CSearchDialog::SearchSaveState(HWND hDlg)
{
	HWND hListView;
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptSearch.x = rcTemp.left;
	m_ptSearch.y = rcTemp.top;
	m_iSearchWidth = GetRectWidth(&rcTemp);
	m_iSearchHeight = GetRectHeight(&rcTemp);

	hListView = GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS);

	m_iColumnWidth1 = ListView_GetColumnWidth(hListView,0);
	m_iColumnWidth2 = ListView_GetColumnWidth(hListView,1);

	GetDlgItemText(hDlg,IDC_COMBO_NAME,m_SearchPatternText,
		SIZEOF_ARRAY(m_SearchPatternText));

	m_bSearchSubFolders = IsDlgButtonChecked(hDlg,
		IDC_CHECK_SEARCHSUBFOLDERS) == BST_CHECKED;

	m_bSearchDlgStateSaved = TRUE;
}