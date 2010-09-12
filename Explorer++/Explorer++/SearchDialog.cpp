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
#include "MainResource.h"


#define WM_USER_SEARCHITEMFOUND			(WM_APP + 1)
#define WM_USER_SEARCHFINISHED			(WM_APP + 2)
#define WM_USER_SEARCHCHANGEDDIRECTORY	(WM_APP + 3)

#define SEARCH_PROCESSITEMS_TIMER_ID		0
#define SEARCH_PROCESSITEMS_TIMER_ELAPSED	50
#define SEARCH_MAX_ITEMS_BATCH_PROCESS		100

#define DEFAULT_SEARCH_ALLOCATION	200

typedef struct
{
	TCHAR szDirectory[MAX_PATH];
	TCHAR szName[MAX_PATH + 2];
	DWORD dwAttributes;

	CContainer *pContainer;
	HWND hDlg;
	HWND hListView;

	BOOL bSearchSubFolders;
} SearchInfo_t;

typedef struct
{
	TCHAR szFullFileName[MAX_PATH];
} DirectoryInfo_t;

typedef struct
{
	TCHAR szFullFileName[MAX_PATH];
} SearchItem_t;

SearchItem_t *g_pSearchItems = NULL;
int g_nSearchItemsAllocated = 0;
int *g_pMap = NULL;

DWORD WINAPI SearchThread(LPVOID pParam);

HWND g_hGripper;

HICON g_hIcon;
HICON g_hDialogIcon;
BOOL g_bSearching = FALSE;
BOOL g_bStopSearching = FALSE;

int g_iFoldersFound = 0;
int g_iFilesFound = 0;

BOOL g_bExit = FALSE;

TCHAR g_szSearch[MAX_PATH];

TCHAR g_szSearchButton[32];

int g_iMinWidth;
int g_iMinHeight;

int g_iListViewWidthDelta;
int g_iListViewHeightDelta;
int g_iSearchDirectoryWidthDelta;
int g_iNamedWidthDelta;
int g_iButtonDirectoryLeftDelta;
int g_iEtchedHorzWidthDelta;
int g_iEtchedHorzVerticalDelta;
int g_iExitLeftDelta;
int g_iExitVerticalDelta;
int g_iSearchExitDelta;
int g_iStaticStatusWidthDelta;
int g_iStatusVerticalDelta;

int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);

INT_PTR CALLBACK SearchProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	return pContainer->SearchProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::SearchProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			{
				HWND hListView;
				HWND hComboBoxEx;
				HWND hComboBox;
				HWND hButton;
				list<SearchDirectoryInfo_t>::iterator itr;
				list<SearchPatternInfo_t>::iterator itr2;
				LVCOLUMN lvColumn;
				HIMAGELIST himlSmall;
				HIMAGELIST himl;
				HBITMAP hBitmap;
				TCHAR szCurrentDirectory[MAX_PATH];

				m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory),
					szCurrentDirectory);

				SetDlgItemText(hDlg,IDC_COMBO_DIRECTORY,szCurrentDirectory);

				himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);

				hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));

				ImageList_Add(himl,hBitmap,NULL);

				g_hIcon = ImageList_GetIcon(himl,SHELLIMAGES_NEWTAB,ILD_NORMAL);
				g_hDialogIcon = ImageList_GetIcon(himl,SHELLIMAGES_SEARCH,ILD_NORMAL);

				SetClassLongPtr(hDlg,GCLP_HICONSM,(LONG)g_hDialogIcon);

				DeleteObject(hBitmap);
				ImageList_Destroy(himl);

				hButton = GetDlgItem(hDlg,IDC_BUTTON_DIRECTORY);
				SendMessage(hButton,BM_SETIMAGE,IMAGE_ICON,(LPARAM)g_hIcon);

				hListView = GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS);

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

				GetWindowRect(hDlg,&rcMain);
				g_iMinWidth = GetRectWidth(&rcMain);
				g_iMinHeight = GetRectHeight(&rcMain);

				GetClientRect(hDlg,&rcMain);

				GetWindowRect(GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				g_iListViewWidthDelta = rcMain.right - rc.right;
				g_iListViewHeightDelta = rcMain.bottom - rc.bottom;

				GetWindowRect(GetDlgItem(hDlg,IDC_COMBO_DIRECTORY),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				g_iSearchDirectoryWidthDelta = rcMain.right - rc.right;

				GetWindowRect(GetDlgItem(hDlg,IDC_COMBO_NAME),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				g_iNamedWidthDelta = rcMain.right - rc.right;

				GetWindowRect(GetDlgItem(hDlg,IDC_BUTTON_DIRECTORY),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				g_iButtonDirectoryLeftDelta = rcMain.right - rc.left;

				GetWindowRect(GetDlgItem(hDlg,IDC_STATIC_STATUS),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				g_iStaticStatusWidthDelta = rcMain.right - rc.right;
				g_iStatusVerticalDelta = rcMain.bottom - rc.top;

				GetWindowRect(GetDlgItem(hDlg,IDC_STATIC_ETCHEDHORZ),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				g_iEtchedHorzWidthDelta = rcMain.right - rc.right;
				g_iEtchedHorzVerticalDelta = rcMain.bottom - rc.top;

				GetWindowRect(GetDlgItem(hDlg,IDEXIT),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				g_iExitLeftDelta = rcMain.right - rc.left;
				g_iExitVerticalDelta = rcMain.bottom - rc.top;

				GetWindowRect(GetDlgItem(hDlg,IDSEARCH),&rc);
				GetWindowRect(GetDlgItem(hDlg,IDEXIT),&rc2);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc2,sizeof(RECT) / sizeof(POINT));
				g_iSearchExitDelta = rc2.left - rc.left;

				g_bSearching = FALSE;
				g_iFoldersFound = 0;
				g_iFilesFound = 0;
				g_bExit = FALSE;
				m_bSetSearchTimer = TRUE;

				if(m_bSearchSubFolders)
					CheckDlgButton(hDlg,IDC_CHECK_SEARCHSUBFOLDERS,BST_CHECKED);

				if(!m_SearchDirectories.empty())
				{
					COMBOBOXEXITEM cbi;
					int iItem = 0;

					hComboBoxEx = GetDlgItem(hDlg,IDC_COMBO_DIRECTORY);
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

					hComboBoxEx = GetDlgItem(hDlg,IDC_COMBO_NAME);
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

				SetDlgItemText(hDlg,IDC_COMBO_NAME,m_SearchPatternText);

				g_hGripper = CreateWindow(_T("SCROLLBAR"),EMPTY_STRING,WS_CHILD|WS_VISIBLE|
					WS_CLIPSIBLINGS|SBS_BOTTOMALIGN|SBS_SIZEGRIP,0,0,0,0,hDlg,NULL,
					GetModuleHandle(0),NULL);

				GetClientRect(hDlg,&rcMain);
				GetWindowRect(g_hGripper,&rc);
				SetWindowPos(g_hGripper,NULL,GetRectWidth(&rcMain) - GetRectWidth(&rc),
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

					SetWindowPos(hDlg,HWND_TOP,m_ptSearch.x,m_ptSearch.y,
						m_iSearchWidth,m_iSearchHeight,SWP_SHOWWINDOW);
				}
				else
				{
					CenterWindow(m_hContainer,hDlg);
				}

				SetFocus(GetDlgItem(hDlg,IDC_COMBO_DIRECTORY));
			}
			break;

		case WM_TIMER:
			{
				if((int)wParam == SEARCH_PROCESSITEMS_TIMER_ID)
				{
					list<LPITEMIDLIST>::iterator itr;
					int nItems = min((int)m_SearchItems.size(),SEARCH_MAX_ITEMS_BATCH_PROCESS);
					int i = 0;

					itr = m_SearchItems.begin();

					while(i < nItems)
					{
						HWND hListView;
						LPITEMIDLIST pidl = NULL;
						TCHAR szFullFileName[MAX_PATH];
						TCHAR szDirectory[MAX_PATH];
						TCHAR szFileName[MAX_PATH];
						LVITEM lvItem;
						SHFILEINFO shfi;
						int iIndex;
						static int iItem = 0;
						int iInternalIndex = 0;

						pidl = *itr;

						hListView = GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS);

						GetDisplayName(pidl,szDirectory,SHGDN_FORPARSING);
						PathRemoveFileSpec(szDirectory);

						GetDisplayName(pidl,szFullFileName,SHGDN_FORPARSING);
						GetDisplayName(pidl,szFileName,SHGDN_INFOLDER|SHGDN_FORPARSING);

						SHGetFileInfo((LPCWSTR)pidl,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);

						while(g_pMap[iInternalIndex] != 0 && iInternalIndex < g_nSearchItemsAllocated)
						{
							iInternalIndex++;
						}

						if(iInternalIndex == g_nSearchItemsAllocated)
						{
							g_pSearchItems = (SearchItem_t *)realloc(g_pSearchItems,
								(g_nSearchItemsAllocated + DEFAULT_SEARCH_ALLOCATION) * sizeof(SearchItem_t));
							g_pMap = (int *)realloc(g_pMap,
								(g_nSearchItemsAllocated + DEFAULT_SEARCH_ALLOCATION) * sizeof(int));
							g_nSearchItemsAllocated += DEFAULT_SEARCH_ALLOCATION;

							iInternalIndex++;
						}

						StringCchCopy(g_pSearchItems[iInternalIndex].szFullFileName,
							SIZEOF_ARRAY(g_pSearchItems[iInternalIndex].szFullFileName),
							szFullFileName);
						g_pMap[iInternalIndex] = 1;

						lvItem.mask		= LVIF_IMAGE|LVIF_TEXT|LVIF_PARAM;
						lvItem.pszText	= szFileName;
						lvItem.iItem	= iItem++;
						lvItem.iSubItem	= 0;
						lvItem.iImage	= shfi.iIcon;
						lvItem.lParam	= iInternalIndex;
						iIndex = ListView_InsertItem(hListView,&lvItem);

						ListView_SetItemText(hListView,iIndex,1,szDirectory);

						CoTaskMemFree(pidl);

						itr = m_SearchItems.erase(itr);

						i++;
					}

					m_bSetSearchTimer = TRUE;
				}
			}
			break;

		/* We won't actually process the item here. Instead, we'll
		add it onto the list of current items, which will be processed
		in batch. This is done to stop this message from blocking the
		main GUI (also see http://www.flounder.com/iocompletion.htm). */
		case WM_USER_SEARCHITEMFOUND:
			{
				m_SearchItems.push_back((LPITEMIDLIST)wParam);

				if(m_bSetSearchTimer)
				{
					SetTimer(hDlg,SEARCH_PROCESSITEMS_TIMER_ID,
						SEARCH_PROCESSITEMS_TIMER_ELAPSED,NULL);

					m_bSetSearchTimer = FALSE;
				}
			}
			break;

		case WM_USER_SEARCHFINISHED:
			{
				TCHAR szStatus[512];

				if(!g_bStopSearching)
				{
					StringCchPrintf(szStatus,SIZEOF_ARRAY(szStatus),
						_T("Finished. %d folder(s) and %d file(s) found."),g_iFoldersFound,
						g_iFilesFound);
					SetDlgItemText(hDlg,IDC_STATIC_STATUS,szStatus);
				}
				else
				{
					SetDlgItemText(hDlg,IDC_STATIC_STATUS,_T("Cancelled."));

					if(g_bExit)
					{
						SearchSaveState(hDlg);
						DestroyWindow(hDlg);
					}
				}

				g_bSearching = FALSE;
				g_bStopSearching = FALSE;

				SetDlgItemText(hDlg,IDSEARCH,g_szSearchButton);
			}
			break;

		case WM_USER_SEARCHCHANGEDDIRECTORY:
			{
				TCHAR szStatus[512];
				TCHAR *pszDirectory = NULL;

				pszDirectory = (TCHAR *)wParam;

				TCHAR szTemp[64];

				LoadString(g_hLanguageModule,IDS_SEARCHING,
					szTemp,SIZEOF_ARRAY(szTemp));
				StringCchPrintf(szStatus,SIZEOF_ARRAY(szStatus),szTemp,
					pszDirectory);
				SetDlgItemText(hDlg,IDC_STATIC_STATUS,szStatus);
			}
			break;

		case WM_GETMINMAXINFO:
			{
				MINMAXINFO *pmmi = NULL;

				pmmi = (MINMAXINFO *)lParam;

				/* Set the minimum dialog size. */
				pmmi->ptMinTrackSize.x = g_iMinWidth;
				pmmi->ptMinTrackSize.y = g_iMinHeight;

				return 0;
			}
			break;

		case WM_SIZE:
			{
				HWND hListView;
				RECT rc;
				int iWidth;
				int iHeight;

				iWidth = LOWORD(lParam);
				iHeight = HIWORD(lParam);

				GetWindowRect(g_hGripper,&rc);
				SetWindowPos(g_hGripper,NULL,iWidth - GetRectWidth(&rc),iHeight - GetRectHeight(&rc),0,
					0,SWP_NOSIZE|SWP_NOZORDER);

				hListView = GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS);

				GetWindowRect(hListView,&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				SetWindowPos(hListView,NULL,0,0,iWidth - g_iListViewWidthDelta - rc.left,
					iHeight - g_iListViewHeightDelta - rc.top,SWP_NOMOVE|SWP_NOZORDER);

				GetWindowRect(GetDlgItem(hDlg,IDC_COMBO_DIRECTORY),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				SetWindowPos(GetDlgItem(hDlg,IDC_COMBO_DIRECTORY),NULL,0,0,
					iWidth - g_iSearchDirectoryWidthDelta - rc.left,GetRectHeight(&rc),SWP_NOMOVE|SWP_NOZORDER);

				GetWindowRect(GetDlgItem(hDlg,IDC_COMBO_NAME),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				SetWindowPos(GetDlgItem(hDlg,IDC_COMBO_NAME),NULL,0,0,
					iWidth - g_iNamedWidthDelta - rc.left,GetRectHeight(&rc),SWP_NOMOVE|SWP_NOZORDER);

				GetWindowRect(GetDlgItem(hDlg,IDC_BUTTON_DIRECTORY),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				SetWindowPos(GetDlgItem(hDlg,IDC_BUTTON_DIRECTORY),NULL,
					iWidth - g_iButtonDirectoryLeftDelta,rc.top,0,0,SWP_NOSIZE|SWP_NOZORDER);

				GetWindowRect(GetDlgItem(hDlg,IDC_STATIC_STATUSLABEL),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				SetWindowPos(GetDlgItem(hDlg,IDC_STATIC_STATUSLABEL),NULL,rc.left,iHeight - g_iStatusVerticalDelta,
					0,0,SWP_NOSIZE|SWP_NOZORDER);

				GetWindowRect(GetDlgItem(hDlg,IDC_STATIC_STATUS),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				SetWindowPos(GetDlgItem(hDlg,IDC_STATIC_STATUS),NULL,rc.left,iHeight - g_iStatusVerticalDelta,
					iWidth - g_iStaticStatusWidthDelta - rc.left,GetRectHeight(&rc),SWP_NOZORDER);

				GetWindowRect(GetDlgItem(hDlg,IDC_STATIC_ETCHEDHORZ),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				SetWindowPos(GetDlgItem(hDlg,IDC_STATIC_ETCHEDHORZ),NULL,rc.left,iHeight - g_iEtchedHorzVerticalDelta,
					iWidth - g_iEtchedHorzWidthDelta - rc.left,GetRectHeight(&rc),SWP_NOZORDER);

				GetWindowRect(GetDlgItem(hDlg,IDEXIT),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				SetWindowPos(GetDlgItem(hDlg,IDEXIT),NULL,
					iWidth - g_iExitLeftDelta,iHeight - g_iExitVerticalDelta,0,0,SWP_NOSIZE|SWP_NOZORDER);

				GetWindowRect(GetDlgItem(hDlg,IDSEARCH),&rc);
				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,sizeof(RECT) / sizeof(POINT));
				SetWindowPos(GetDlgItem(hDlg,IDSEARCH),NULL,
					iWidth - g_iExitLeftDelta - g_iSearchExitDelta,iHeight - g_iExitVerticalDelta,0,0,SWP_NOSIZE|SWP_NOZORDER);
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDSEARCH:
				OnSearch(hDlg);
				break;

			case IDC_BUTTON_DIRECTORY:
				{
					BROWSEINFO bi;
					PIDLIST_ABSOLUTE pidl = NULL;
					TCHAR szDisplayName[MAX_PATH];
					TCHAR szParsingPath[MAX_PATH];
					TCHAR szTitle[] = _T("Select a folder to search, then press OK");

					GetDlgItemText(hDlg,IDC_COMBO_DIRECTORY,g_szSearch,SIZEOF_ARRAY(g_szSearch));

					CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);

					bi.hwndOwner		= hDlg;
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

						SetDlgItemText(hDlg,IDC_COMBO_DIRECTORY,szParsingPath);

						CoTaskMemFree(pidl);
					}
				}
				break;

			case IDEXIT:
				if(g_bSearching)
				{
					g_bExit = TRUE;
					g_bStopSearching = TRUE;
				}
				else
				{
					SearchSaveState(hDlg);
					DestroyWindow(hDlg);
				}
				break;

			case IDCANCEL:
				DestroyWindow(hDlg);
				break;
			}
			break;

		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code)
			{
			case NM_DBLCLK:
				if(((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS))
				{
					HWND hListView;
					LVITEM lvItem;
					LPITEMIDLIST pidlFull = NULL;
					TCHAR szDirectory[MAX_PATH];
					BOOL bRet;
					int iSelected;

					hListView = GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS);

					iSelected = ListView_GetNextItem(hListView,-1,LVNI_ALL|LVNI_SELECTED);

					if(iSelected != -1)
					{
						lvItem.mask		= LVIF_PARAM;
						lvItem.iItem	= iSelected;
						lvItem.iSubItem	= 0;
						bRet = ListView_GetItem(hListView,&lvItem);

						if(bRet)
						{
							StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),
								g_pSearchItems[(int)lvItem.lParam].szFullFileName);
							GetIdlFromParsingName(szDirectory,&pidlFull);

							OpenItem(pidlFull,TRUE,FALSE);

							CoTaskMemFree(pidlFull);
						}
					}
				}
				break;

			case NM_RCLICK:
				{
					if(((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS))
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

						hListView = GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS);

						iSelected = ListView_GetNextItem(hListView,-1,LVNI_ALL|LVNI_SELECTED);

						if(iSelected != -1)
						{
							lvItem.mask		= LVIF_PARAM;
							lvItem.iItem	= iSelected;
							lvItem.iSubItem	= 0;
							bRet = ListView_GetItem(hListView,&lvItem);

							if(bRet)
							{
								StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),
									g_pSearchItems[(int)lvItem.lParam].szFullFileName);
								GetIdlFromParsingName(szDirectory,&pidlFull);
								PathRemoveFileSpec(szDirectory);
								GetIdlFromParsingName(szDirectory,&pidlDirectory);

								pidl = ILFindLastID(pidlFull);

								dwCursorPos = GetMessagePos();
								ptsCursor = MAKEPOINTS(dwCursorPos);

								ptCursor.x = ptsCursor.x;
								ptCursor.y = ptsCursor.y;

								CreateFileContextMenu(hDlg,pidlDirectory,
									ptCursor,FROM_SEARCH,(LPCITEMIDLIST *)&pidl,1,FALSE,FALSE);

								CoTaskMemFree(pidlDirectory);
								CoTaskMemFree(pidlFull);
							}
						}
					}
				}
				break;
			}
			break;

		case WM_CLOSE:
			if(g_bSearching)
			{
				g_bExit = TRUE;
				g_bStopSearching = TRUE;
			}
			else
			{
				SearchSaveState(hDlg);
				DestroyWindow(hDlg);
			}
			break;

		case WM_DESTROY:
			g_hwndSearch = NULL;
			DestroyIcon(g_hDialogIcon);
			DestroyIcon(g_hIcon);
			break;
	}

	return 0;
}

int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)g_szSearch);
		break;
	}

	return 0;
}

void SearchDirectory(SearchInfo_t *psi,TCHAR *szDirectory);
void SearchDirectoryInternal(HWND hDlg,TCHAR *szSearchDirectory,
TCHAR *szSearchPattern,DWORD dwAttributes,list<DirectoryInfo_t> *pSubfolderList);

DWORD WINAPI SearchThread(LPVOID pParam)
{
	SearchInfo_t *psi = NULL;

	psi = (SearchInfo_t *)pParam;

	SearchDirectory(psi,psi->szDirectory);

	SendMessage(psi->hDlg,WM_USER_SEARCHFINISHED,0,0);

	free(psi);

	return 0;
}

void SearchDirectory(SearchInfo_t *psi,TCHAR *szDirectory)
{
	list<DirectoryInfo_t> SubfolderList;
	list<DirectoryInfo_t>::iterator itr;

	SendMessage(psi->hDlg,WM_USER_SEARCHCHANGEDDIRECTORY,
		(WPARAM)szDirectory,0);

	SearchDirectoryInternal(psi->hDlg,szDirectory,
		psi->szName,psi->dwAttributes,&SubfolderList);

	if(g_bStopSearching)
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
	}
}

/* Can't recurse, as it would overflow the stack. */
void SearchDirectoryInternal(HWND hDlg,TCHAR *szSearchDirectory,
TCHAR *szSearchPattern,DWORD dwAttributes,list<DirectoryInfo_t> *pSubfolderList)
{
	WIN32_FIND_DATA wfd;
	HANDLE hFindFile;
	TCHAR szSearchTerm[MAX_PATH];
	DirectoryInfo_t di;

	PathCombine(szSearchTerm,szSearchDirectory,_T("*"));

	hFindFile = FindFirstFile(szSearchTerm,&wfd);

	if(hFindFile != INVALID_HANDLE_VALUE)
	{
		while(FindNextFile(hFindFile,&wfd) != 0)
		{
			if(g_bStopSearching)
				break;

			if(lstrcmpi(wfd.cFileName,_T(".")) != 0 &&
				lstrcmpi(wfd.cFileName,_T("..")) != 0)
			{
				BOOL bFileNameActive = FALSE;
				BOOL bAttributesActive = FALSE;
				BOOL bMatchFileName = FALSE;
				BOOL bMatchAttributes = FALSE;
				BOOL bItemMatch = FALSE;

				/* Only match against the filename if it's not empty. */
				if(lstrcmp(szSearchPattern,EMPTY_STRING) != 0)
				{
					bFileNameActive = TRUE;

					if(CheckWildcardMatch(szSearchPattern,wfd.cFileName,FALSE))
					{
						bMatchFileName = TRUE;
					}
				}

				if(dwAttributes != 0)
				{
					bAttributesActive = TRUE;

					if(wfd.dwFileAttributes & dwAttributes)
					{
						bMatchAttributes = TRUE;
					}
				}

				if(bFileNameActive && bAttributesActive)
					bItemMatch = bMatchFileName && bMatchAttributes;
				else if(bFileNameActive)
					bItemMatch = bMatchFileName;
				else if(bAttributesActive)
					bItemMatch = bMatchAttributes;

				if(bItemMatch)
				{
					if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
						FILE_ATTRIBUTE_DIRECTORY)
						g_iFoldersFound++;
					else
						g_iFilesFound++;

					LPITEMIDLIST pidl = NULL;
					TCHAR szFullFileName[MAX_PATH];

					PathCombine(szFullFileName,szSearchDirectory,wfd.cFileName);
					GetIdlFromParsingName(szFullFileName,&pidl);

					PostMessage(hDlg,WM_USER_SEARCHITEMFOUND,(WPARAM)ILClone(pidl),0);

					CoTaskMemFree(pidl);
				}

				/* If this item is a folder, follow it. */
				if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
					FILE_ATTRIBUTE_DIRECTORY)
				{
					PathCombine(di.szFullFileName,szSearchDirectory,
						wfd.cFileName);
					pSubfolderList->push_back(di);
				}
			}
		}

		FindClose(hFindFile);
	}
}

void CContainer::OnSearch(HWND hDlg)
{
	SearchInfo_t *psi = NULL;
	SearchDirectoryInfo_t sdi;
	SearchPatternInfo_t spi;
	TCHAR szPattern[MAX_PATH];

	m_SearchItems.clear();

	if(!g_bSearching)
	{
		psi = (SearchInfo_t *)malloc(sizeof(SearchInfo_t));

		if(psi != NULL)
		{
			ListView_DeleteAllItems(GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS));

			if(g_pSearchItems != NULL)
			{
				free(g_pSearchItems);
				g_pSearchItems = NULL;
			}

			if(g_pMap != NULL)
			{
				free(g_pMap);
				g_pMap = NULL;
			}

			g_pSearchItems = (SearchItem_t *)malloc(DEFAULT_SEARCH_ALLOCATION * sizeof(SearchItem_t));
			g_pMap = (int *)malloc(DEFAULT_SEARCH_ALLOCATION * sizeof(int));
			g_nSearchItemsAllocated = DEFAULT_SEARCH_ALLOCATION;

			int i = 0;

			for(i = 0;i < g_nSearchItemsAllocated;i++)
			{
				g_pMap[i] = 0;
			}

			g_iFoldersFound = 0;
			g_iFilesFound = 0;

			/* Get the directory and name, and remove leading and
			trailing whitespace. */
			GetDlgItemText(hDlg,IDC_COMBO_DIRECTORY,psi->szDirectory,
				SIZEOF_ARRAY(psi->szDirectory));
			PathRemoveBlanks(psi->szDirectory);
			GetDlgItemText(hDlg,IDC_COMBO_NAME,psi->szName,
				SIZEOF_ARRAY(psi->szName));
			PathRemoveBlanks(psi->szName);

			psi->pContainer = this;
			psi->hDlg = hDlg;
			psi->hListView = GetDlgItem(hDlg,IDC_LISTVIEW_SEARCHRESULTS);
			psi->bSearchSubFolders = IsDlgButtonChecked(hDlg,IDC_CHECK_SEARCHSUBFOLDERS) ==
				BST_CHECKED;

			psi->dwAttributes = 0;

			if(IsDlgButtonChecked(hDlg,IDC_CHECK_ARCHIVE) == BST_CHECKED)
				psi->dwAttributes |= FILE_ATTRIBUTE_ARCHIVE;

			if(IsDlgButtonChecked(hDlg,IDC_CHECK_HIDDEN) == BST_CHECKED)
				psi->dwAttributes |= FILE_ATTRIBUTE_HIDDEN;

			if(IsDlgButtonChecked(hDlg,IDC_CHECK_READONLY) == BST_CHECKED)
				psi->dwAttributes |= FILE_ATTRIBUTE_READONLY;

			if(IsDlgButtonChecked(hDlg,IDC_CHECK_SYSTEM) == BST_CHECKED)
				psi->dwAttributes |= FILE_ATTRIBUTE_SYSTEM;

			/* Save the search directory and search pattern (only if they are not
			the same as the most recent entry). */
			BOOL bSaveEntry;

			if(m_SearchDirectories.empty())
				bSaveEntry = TRUE;
			else if(lstrcmp(psi->szDirectory,m_SearchDirectories.begin()->szDirectory) != 0)
				bSaveEntry = TRUE;
			else
				bSaveEntry = FALSE;

			if(bSaveEntry)
			{
				StringCchCopy(sdi.szDirectory,SIZEOF_ARRAY(sdi.szDirectory),psi->szDirectory);
				m_SearchDirectories.push_front(sdi);

				/* Insert the entry into the combobox. */
				HWND hComboBoxEx;
				HWND hComboBox;
				COMBOBOXEXITEM cbi;

				hComboBoxEx = GetDlgItem(hDlg,IDC_COMBO_DIRECTORY);
				hComboBox = (HWND)SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0);

				cbi.mask	= CBEIF_TEXT;
				cbi.iItem	= 0;
				cbi.pszText	= psi->szDirectory;
				SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,(LPARAM)&cbi);

				ComboBox_SetCurSel(hComboBox,0);
			}

			if(m_SearchPatterns.empty())
				bSaveEntry = TRUE;
			else if(lstrcmp(psi->szName,m_SearchPatterns.begin()->szPattern) != 0)
				bSaveEntry = TRUE;
			else
				bSaveEntry = FALSE;

			if(bSaveEntry)
			{
				StringCchCopy(spi.szPattern,SIZEOF_ARRAY(spi.szPattern),psi->szName);
				m_SearchPatterns.push_front(spi);

				/* Insert the entry into the combobox. */
				HWND hComboBoxEx;
				HWND hComboBox;
				COMBOBOXEXITEM cbi;

				hComboBoxEx = GetDlgItem(hDlg,IDC_COMBO_NAME);
				hComboBox = (HWND)SendMessage(hComboBoxEx,CBEM_GETCOMBOCONTROL,0,0);

				cbi.mask	= CBEIF_TEXT;
				cbi.iItem	= 0;
				cbi.pszText	= psi->szName;
				SendMessage(hComboBoxEx,CBEM_INSERTITEM,0,(LPARAM)&cbi);

				ComboBox_SetCurSel(hComboBox,0);
			}

			/* Turn search patterns of the form '???' into '*???*', and
			use this modified string to search. */
			if(lstrlen(psi->szName) > 0)
			{
				StringCchCopy(szPattern,SIZEOF_ARRAY(szPattern),psi->szName);
				memset(psi->szName,0,SIZEOF_ARRAY(psi->szName));

				if(szPattern[0] != '*')
				{
					StringCchCat(psi->szName,SIZEOF_ARRAY(psi->szName),_T("*"));
				}

				StringCchCat(psi->szName,SIZEOF_ARRAY(psi->szName),szPattern);

				if(szPattern[lstrlen(szPattern) - 1] != '*')
				{
					StringCchCat(psi->szName,SIZEOF_ARRAY(psi->szName),_T("*"));
				}
			}

			GetDlgItemText(hDlg,IDSEARCH,g_szSearchButton,SIZEOF_ARRAY(g_szSearchButton));

			TCHAR szTemp[64];

			LoadString(g_hLanguageModule,IDS_STOP,
				szTemp,SIZEOF_ARRAY(szTemp));
			SetDlgItemText(hDlg,IDSEARCH,szTemp);

			g_bSearching = TRUE;

			/* Create a background thread, and search using it... */
			CreateThread(NULL,0,SearchThread,(LPVOID)psi,0,NULL);
		}
	}
	else
	{
		g_bStopSearching = TRUE;
	}
}

void CContainer::SearchSaveState(HWND hDlg)
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