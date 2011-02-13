/******************************************************************
 *
 * Project: Explorer++
 * File: MassRenameDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides support for the mass renaming of files.
 * The following special characters are supported:
 * $N	- Counter
 * $F	- Filename
 * $B	- Basename (filename without extension)
 * $E	- Extension
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Explorer++_internal.h"
#include "MassRenameDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"


const TCHAR CMassRenameDialogPersistentSettings::SETTINGS_KEY[] = _T("MassRename");

CMassRenameDialog::CMassRenameDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,std::list<std::wstring> FullFilenameList) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_FullFilenameList = FullFilenameList;

	m_pmrdps = &CMassRenameDialogPersistentSettings::GetInstance();
}

CMassRenameDialog::~CMassRenameDialog()
{

}

BOOL CMassRenameDialog::OnInitDialog()
{
	HIMAGELIST himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himl,hBitmap,NULL);

	m_hDialogIcon = ImageList_GetIcon(himl,SHELLIMAGES_RENAME,ILD_NORMAL);
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_hDialogIcon));

	m_hMoreIcon = ImageList_GetIcon(himl,SHELLIMAGES_RIGHTARROW,ILD_NORMAL);
	SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_MORE,BM_SETIMAGE,IMAGE_ICON,
		reinterpret_cast<LPARAM>(m_hMoreIcon));

	DeleteObject(hBitmap);
	ImageList_Destroy(himl);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MASSRENAME_FILELISTVIEW);

	LONG Style;
	Style = GetWindowLong(hListView,GWL_STYLE);
	SetWindowLongPtr(hListView,GWL_STYLE,Style|LVS_SHAREIMAGELISTS);

	SetWindowTheme(hListView,L"Explorer",NULL);

	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER|LVS_EX_SUBITEMIMAGES|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,
		LVS_EX_DOUBLEBUFFER|LVS_EX_SUBITEMIMAGES|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	LVCOLUMN lvCol;

	/* TODO: Move text to string table. */
	lvCol.mask		= LVCF_TEXT;
	lvCol.pszText	= _T("Current Name");
	ListView_InsertColumn(hListView,1,&lvCol);

	/* TODO: Move text to string table. */
	lvCol.mask		= LVCF_TEXT;
	lvCol.pszText	= _T("Preview Name");
	ListView_InsertColumn(hListView,2,&lvCol);

	RECT rc;
	GetClientRect(hListView,&rc);
	SendMessage(hListView,LVM_SETCOLUMNWIDTH,0,GetRectWidth(&rc) / 2);
	SendMessage(hListView,LVM_SETCOLUMNWIDTH,1,GetRectWidth(&rc) / 2);

	LVITEM lvItem;
	SHFILEINFO shfi;
	TCHAR szFilename[MAX_PATH];
	int iItem = 0;

	/* Add each file to the listview, along with its icon. */
	for each(auto strFilename in m_FullFilenameList)
	{
		SHGetFileInfo(strFilename.c_str(),0,&shfi,
			sizeof(SHFILEINFO),SHGFI_SYSICONINDEX);

		StringCchCopy(szFilename,SIZEOF_ARRAY(szFilename),
			strFilename.c_str());
		PathStripPath(szFilename);

		lvItem.mask		= LVIF_TEXT|LVIF_IMAGE;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.iImage	= shfi.iIcon;
		lvItem.pszText	= szFilename;
		ListView_InsertItem(hListView,&lvItem);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 1;
		lvItem.pszText	= szFilename;
		ListView_SetItem(hListView,&lvItem);

		iItem++;
	}

	SetDlgItemText(m_hDlg,IDC_MASSRENAME_EDIT,_T("$F"));
	SendMessage(GetDlgItem(m_hDlg,IDC_MASSRENAME_EDIT),
		EM_SETSEL,0,-1);
	SetFocus(GetDlgItem(m_hDlg,IDC_MASSRENAME_EDIT));

	m_pmrdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

void CMassRenameDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_MASSRENAME_EDIT;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_MASSRENAME_MORE;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_MASSRENAME_FILELISTVIEW;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDOK;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDCANCEL;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_GRIPPER;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);
}

BOOL CMassRenameDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	if(HIWORD(wParam) != 0)
	{
		switch(HIWORD(wParam))
		{
		case EN_CHANGE:
			{
				TCHAR szNamePattern[MAX_PATH];
				GetDlgItemText(m_hDlg,IDC_MASSRENAME_EDIT,
					szNamePattern,SIZEOF_ARRAY(szNamePattern));

				HWND hListView = GetDlgItem(m_hDlg,IDC_MASSRENAME_FILELISTVIEW);

				LVITEM lvItem;
				std::wstring strNewFilename;
				TCHAR szFilename[MAX_PATH];
				TCHAR szNewFilename[MAX_PATH];
				int iItem = 0;

				for each(auto strFilename in m_FullFilenameList)
				{
					StringCchCopy(szFilename,SIZEOF_ARRAY(szFilename),
						strFilename.c_str());
					PathStripPath(szFilename);

					ProcessFileName(szNamePattern,szFilename,iItem,strNewFilename);

					StringCchCopy(szNewFilename,SIZEOF_ARRAY(szNewFilename),
						strNewFilename.c_str());

					lvItem.mask		= LVIF_TEXT;
					lvItem.iItem	= iItem;
					lvItem.iSubItem	= 1;
					lvItem.pszText	= szNewFilename;
					ListView_SetItem(hListView,&lvItem);

					iItem++;
				}
			}
			break;
		}
	}
	else
	{
		switch(LOWORD(wParam))
		{
		case IDC_MASSRENAME_MORE:
			{
				HMENU hMenu = GetSubMenu(LoadMenu(GetInstance(),MAKEINTRESOURCE(IDR_MASSRENAME_MENU)),0);

				RECT rc;
				GetWindowRect(GetDlgItem(m_hDlg,IDC_MASSRENAME_MORE),&rc);

				UINT uCmd = TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_VERTICAL|TPM_RETURNCMD,
					rc.right,rc.top,0,m_hDlg,NULL);

				switch(uCmd)
				{
				case IDM_MASSRENAME_FILENAME:
					SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,
						reinterpret_cast<LPARAM>(_T("$F")));
					break;

				case IDM_MASSRENAME_BASENAME:
					SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,
						reinterpret_cast<LPARAM>(_T("$B")));
					break;

				case IDM_MASSRENAME_EXTENSION:
					SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,
						reinterpret_cast<LPARAM>(_T("$E")));
					break;

				case IDM_MASSRENAME_COUNTER:
					SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,
						reinterpret_cast<LPARAM>(_T("$N")));
					break;
				}
			}
			break;

		case IDOK:
			OnOk();
			break;

		case IDCANCEL:
			OnCancel();
			break;
		}
	}

	return 0;
}

BOOL CMassRenameDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CMassRenameDialog::OnDestroy()
{
	DestroyIcon(m_hMoreIcon);
	DestroyIcon(m_hDialogIcon);

	return 0;
}

void CMassRenameDialog::OnOk()
{
	TCHAR szNamePattern[MAX_PATH];

	GetDlgItemText(m_hDlg,IDC_MASSRENAME_EDIT,
		szNamePattern,SIZEOF_ARRAY(szNamePattern));

	if(lstrlen(szNamePattern) == 0)
	{
		/* TODO: Show error. */
	}

	std::wstring strNewFilename;
	TCHAR szFilename[MAX_PATH];
	int iItem = 0;

	for each (auto strFilename in m_FullFilenameList)
	{
		StringCchCopy(szFilename,SIZEOF_ARRAY(szFilename),
			strFilename.c_str());
		PathStripPath(szFilename);

		ProcessFileName(szNamePattern,szFilename,iItem,strNewFilename);

		/* TODO: */
		/*TCHAR szNewFileName[MAX_PATH];

		StringCchPrintf(szNewFileName,MAX_PATH,_T("%s\\%s"),m_CurrentDirectory,szTargetName);

		szNewFileName[lstrlen(szNewFileName) + 1] = '\0';
		itr->szFullFileName[lstrlen(itr->szFullFileName) + 1] = '\0';

		RenameFileWithUndo(szNewFileName,itr->szFullFileName);*/

		iItem++;
	}

	EndDialog(m_hDlg,1);
}

void CMassRenameDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CMassRenameDialog::SaveState()
{
	m_pmrdps->SaveDialogPosition(m_hDlg);

	m_pmrdps->m_bStateSaved = TRUE;
}

void CMassRenameDialog::ProcessFileName(const std::wstring strTarget,
	const std::wstring strFilename,int iFileIndex,std::wstring &strOutput)
{
	TCHAR szBaseName[MAX_PATH];
	StringCchCopy(szBaseName,MAX_PATH,strFilename.c_str());
	PathRemoveExtension(szBaseName);

	TCHAR *pExt = PathFindExtension(strFilename.c_str());

	size_t iPos;

	strOutput = strTarget;

	while((iPos = strOutput.find(_T("$N"))) != std::wstring::npos)
	{
		std::wstringstream ss;
		ss << iFileIndex;

		strOutput.replace(iPos,2,ss.str());
	}

	while((iPos = strOutput.find(_T("$F"))) != std::wstring::npos)
	{
		strOutput.replace(iPos,2,strFilename);
	}

	while((iPos = strOutput.find(_T("$B"))) != std::wstring::npos)
	{
		strOutput.replace(iPos,2,szBaseName);
	}

	while((iPos = strOutput.find(_T("$E"))) != std::wstring::npos)
	{
		strOutput.replace(iPos,2,pExt);
	}
}

CMassRenameDialogPersistentSettings::CMassRenameDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CMassRenameDialogPersistentSettings::~CMassRenameDialogPersistentSettings()
{
	
}

CMassRenameDialogPersistentSettings& CMassRenameDialogPersistentSettings::GetInstance()
{
	static CMassRenameDialogPersistentSettings sfadps;
	return sfadps;
}