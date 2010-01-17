/******************************************************************
 *
 * Project: Explorer++
 * File: MassRenameDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides support for the mass renaming of files.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

/*
Special characters:
$N	- Counter
$F	- Filename
$B	- Basename (filename without extension)
$E	- Extension
*/

#include "stdafx.h"
#include <list>
#include "Misc.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"

using namespace std;

typedef struct
{
	TCHAR szFullFileName[MAX_PATH];
	TCHAR szFileName[MAX_PATH];
} RenameFile_t;

TCHAR *ProcessFileName(TCHAR *szTargetName,TCHAR *szFileName,int iFileIndex);

HICON				g_hMassRenameIcon;
list<RenameFile_t>	FileNameList;

INT_PTR CALLBACK MassRenameProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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

	return pContainer->MassRenameProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::MassRenameProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				HWND hListView;
				LVCOLUMN lvcol;
				list<RenameFile_t>::iterator itr;
				DWORD dwStyleEx;
				int iItem = 0;

				HIMAGELIST himl;
				HBITMAP hBitmap;

				himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);

				/* Contains all images used on the menus. */
				hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));

				ImageList_Add(himl,hBitmap,NULL);

				g_hMassRenameIcon = ImageList_GetIcon(himl,SHELLIMAGES_RIGHTARROW,ILD_NORMAL);
				SendDlgItemMessage(hDlg,IDC_MASSRENAME_MORE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)g_hMassRenameIcon);

				DeleteObject(hBitmap);
				ImageList_Destroy(himl);

				hListView = GetDlgItem(hDlg,IDC_MASSRENAME_FILELISTVIEW);

				ListView_SetExtendedListViewStyleEx(hListView,
				LVS_EX_GRIDLINES|LVS_EX_SUBITEMIMAGES,
				LVS_EX_GRIDLINES|LVS_EX_SUBITEMIMAGES);

				HIMAGELIST SmallIcons;
				LONG Style;

				Shell_GetImageLists(NULL,&SmallIcons);

				Style = GetWindowLong(hListView,GWL_STYLE);
				SetWindowLongPtr(hListView,GWL_STYLE,Style|LVS_SHAREIMAGELISTS);
				
				ListView_SetImageList(hListView,SmallIcons,LVSIL_SMALL);

				lvcol.mask		= LVCF_TEXT|LVCF_WIDTH;
				lvcol.cx		= 100;
				lvcol.pszText	= _T("Current Name");
				ListView_InsertColumn(hListView,1,&lvcol);

				lvcol.mask		= LVCF_TEXT|LVCF_WIDTH;
				lvcol.cx		= 300;
				lvcol.pszText	= _T("Preview Name");
				ListView_InsertColumn(hListView,2,&lvcol);

				LVITEM item;
				SHFILEINFO shfi;
				int nSelected;
				int iIndex = -1;
				int i = 0;
				RenameFile_t RenameFile;

				FileNameList.clear();

				nSelected = ListView_GetSelectedCount(m_hActiveListView);

				for(i = 0;i < nSelected;i++)
				{
					iIndex = ListView_GetNextItem(m_hActiveListView,
					iIndex,LVNI_SELECTED);

					if(iIndex != -1)
					{
						m_pActiveShellBrowser->QueryFullItemName(iIndex,RenameFile.szFullFileName);
						m_pActiveShellBrowser->QueryName(iIndex,RenameFile.szFileName);

						FileNameList.push_back(RenameFile);
					}
				}

				for(itr = FileNameList.begin();itr != FileNameList.end();itr++)
				{
					SHGetFileInfo(itr->szFullFileName,0,&shfi,
					sizeof(SHFILEINFO),SHGFI_SYSICONINDEX);

					item.mask		= LVIF_TEXT|LVIF_IMAGE;
					item.iItem		= iItem;
					item.iSubItem	= 0;
					item.iImage		= shfi.iIcon;
					item.pszText	= itr->szFileName;
					ListView_InsertItem(hListView,&item);

					item.mask		= LVIF_TEXT;
					item.iItem		= iItem;
					item.iSubItem	= 1;
					item.pszText	= itr->szFileName;
					ListView_SetItem(hListView,&item);

					iItem++;
				}

				RECT rc;

				GetClientRect(hListView,&rc);

				SendMessage(hListView,LVM_SETCOLUMNWIDTH,0,GetRectWidth(&rc) / 2);
				SendMessage(hListView,LVM_SETCOLUMNWIDTH,1,GetRectWidth(&rc) / 2);

				dwStyleEx = ListView_GetExtendedListViewStyle(hListView);
				ListView_SetExtendedListViewStyle(hListView,dwStyleEx|LVS_EX_FULLROWSELECT);

				SetFocus(GetDlgItem(hDlg,IDC_MASSRENAME_EDIT));

				if(m_bMassRenameDlgStateSaved)
				{
					SetWindowPos(hDlg,NULL,m_ptMassRename.x,
						m_ptMassRename.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
				}
				else
				{
					CenterWindow(m_hContainer,hDlg);
				}
			}
			break;

		case WM_COMMAND:
			if(HIWORD(wParam) != 0)
			{
				switch(HIWORD(wParam))
				{
					case EN_CHANGE:
					{
						HWND hListView;
						list<RenameFile_t>::iterator itr;
						TCHAR EditText[100];
						TCHAR *szTargetName;
						LVITEM item;
						int i = 0;

						GetDlgItemText(hDlg,IDC_MASSRENAME_EDIT,
						EditText,SIZEOF_ARRAY(EditText));

						hListView = GetDlgItem(hDlg,IDC_MASSRENAME_FILELISTVIEW);
						
						for(itr = FileNameList.begin();itr != FileNameList.end();itr++)
						{
							szTargetName = ProcessFileName(EditText,itr->szFileName,i);

							item.mask		= LVIF_TEXT;
							item.iItem		= i;
							item.iSubItem	= 1;
							item.pszText	= szTargetName;

							ListView_SetItem(hListView,&item);

							i++;
						}
					}
					break;
				}
				break;
			}
			else
			{
				switch(LOWORD(wParam))
				{
				case IDC_MASSRENAME_MORE:
					{
						HMENU hMenu;
						UINT uId;
						RECT rc;

						hMenu = GetSubMenu(LoadMenu(GetModuleHandle(0),MAKEINTRESOURCE(IDR_MASSRENAME_MENU)),0);

						GetWindowRect(GetDlgItem(hDlg,IDC_MASSRENAME_MORE),&rc);

						uId = TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_VERTICAL|TPM_RETURNCMD,
							rc.right,rc.top,0,hDlg,NULL);

						switch(uId)
						{
						case IDM_MASSRENAME_FILENAME:
							SendDlgItemMessage(hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,(LPARAM)_T("$F"));
							break;

						case IDM_MASSRENAME_BASENAME:
							SendDlgItemMessage(hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,(LPARAM)_T("$B"));
							break;

						case IDM_MASSRENAME_EXTENSION:
							SendDlgItemMessage(hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,(LPARAM)_T("$E"));
							break;

						case IDM_MASSRENAME_COUNTER:
							SendDlgItemMessage(hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,(LPARAM)_T("$N"));
							break;
						}
					}
					break;

				case IDOK:
					OnMassRenameOk(hDlg);
					break;

				case IDCANCEL:
					MassRenameSaveState(hDlg);
					EndDialog(hDlg,0);
					break;
				}
				break;
			}
			break;

		case WM_CLOSE:
			MassRenameSaveState(hDlg);
			EndDialog(hDlg,0);
			break;

		case WM_DESTROY:
			DestroyIcon(g_hMassRenameIcon);
			break;
	}

	return 0;
}

TCHAR *ProcessFileName(TCHAR *szTargetName,TCHAR *szFileName,int iFileIndex)
{
	static TCHAR szOutputText[MAX_PATH];
	TCHAR szBaseName[MAX_PATH];
	TCHAR *pExt;
	int i = 0;
	int j = 0;

	StringCchCopy(szBaseName,MAX_PATH,szFileName);
	PathRemoveExtension(szBaseName);

	pExt = PathFindExtension(szFileName);

	for(i = 0;i < lstrlen(szTargetName);i++)
	{
		if(szTargetName[i] == '$')
		{
			switch(szTargetName[i + 1])
			{
				case 'N':
					TCHAR szFileIndex[20];
					szOutputText[j] = '\0';
					_itot_s(iFileIndex,szFileIndex,20,10);
					StringCchCatN(szOutputText,MAX_PATH,szFileIndex,lstrlen(szFileIndex));
					j += lstrlen(szFileIndex);
					i += 1;
					break;

				case 'F':
					szOutputText[j] = '\0';
					StringCchCatN(szOutputText,MAX_PATH,szFileName,lstrlen(szFileName));
					j += lstrlen(szFileName);
					i += 1;
					break;

				case 'B':
					szOutputText[j] = '\0';
					StringCchCatN(szOutputText,MAX_PATH,szBaseName,lstrlen(szBaseName));
					j += lstrlen(szBaseName);
					i += 1;
					break;

				case 'E':
					szOutputText[j] = '\0';
					StringCchCatN(szOutputText,MAX_PATH,pExt,lstrlen(pExt));
					j += lstrlen(pExt);
					i += 1;
					break;

				default:
					szOutputText[j++] = szTargetName[i];
					break;
			}
		}
		else
		{
			szOutputText[j++] = szTargetName[i];
		}
	}

	szOutputText[j] = '\0';

	return szOutputText;
}

void CContainer::OnMassRenameOk(HWND hDlg)
{
	list<RenameFile_t>::iterator itr;
	HWND hListView;
	TCHAR EditText[100];
	TCHAR *szTargetName;
	int i = 0;
	TCHAR szNewFileName[MAX_PATH];

	GetDlgItemText(hDlg,IDC_MASSRENAME_EDIT,
	EditText,SIZEOF_ARRAY(EditText));

	hListView = GetDlgItem(hDlg,IDC_MASSRENAME_FILELISTVIEW);

	for(itr = FileNameList.begin();itr != FileNameList.end();itr++)
	{
		szTargetName = ProcessFileName(EditText,itr->szFileName,i);

		StringCchPrintf(szNewFileName,MAX_PATH,_T("%s\\%s"),m_CurrentDirectory,szTargetName);

		szNewFileName[lstrlen(szNewFileName) + 1] = '\0';
		itr->szFullFileName[lstrlen(itr->szFullFileName) + 1] = '\0';

		RenameFileWithUndo(szNewFileName,itr->szFullFileName);

		i++;
	}

	MassRenameSaveState(hDlg);

	EndDialog(hDlg,1);
}

void CContainer::MassRenameSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptMassRename.x = rcTemp.left;
	m_ptMassRename.y = rcTemp.top;

	m_bMassRenameDlgStateSaved = TRUE;
}