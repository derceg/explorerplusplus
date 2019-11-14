// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Provides support for the mass renaming of files.
 * The following special characters are supported:
 * /N	- Counter
 * /F	- Filename
 * /B	- Basename (filename without extension)
 * /E	- Extension
 * /L	- Lowercase filename
 * /U	- Uppercase filename
 */

#include "stdafx.h"
#include "MassRenameDialog.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <list>
#include <regex>

const TCHAR CMassRenameDialogPersistentSettings::SETTINGS_KEY[] = _T("MassRename");

const TCHAR CMassRenameDialogPersistentSettings::SETTING_COLUMN_WIDTH_1[] = _T("ColumnWidth1");
const TCHAR CMassRenameDialogPersistentSettings::SETTING_COLUMN_WIDTH_2[] = _T("ColumnWidth2");

CMassRenameDialog::CMassRenameDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,std::list<std::wstring> FullFilenameList,
	CFileActionHandler *pFileActionHandler) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_FullFilenameList = FullFilenameList;
	m_pFileActionHandler = pFileActionHandler;

	m_pmrdps = &CMassRenameDialogPersistentSettings::GetInstance();
}

CMassRenameDialog::~CMassRenameDialog()
{

}

INT_PTR CMassRenameDialog::OnInitDialog()
{
	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hDlg);
	m_moreIcon = IconResourceLoader::LoadIconFromPNGForDpi(Icon::ArrowRight, 16, 16, dpi);
	SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_MORE,BM_SETIMAGE,IMAGE_ICON,
		reinterpret_cast<LPARAM>(m_moreIcon.get()));

	HWND hListView = GetDlgItem(m_hDlg,IDC_MASSRENAME_FILELISTVIEW);

	SetWindowTheme(hListView,L"Explorer",NULL);
	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER|LVS_EX_SUBITEMIMAGES|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,
		LVS_EX_DOUBLEBUFFER|LVS_EX_SUBITEMIMAGES|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	LVCOLUMN lvCol;
	TCHAR szTemp[64];

	LoadString(GetInstance(),IDS_MASS_RENAME_CURRENT_NAME,
		szTemp,SIZEOF_ARRAY(szTemp));
	lvCol.mask		= LVCF_TEXT;
	lvCol.pszText	= szTemp;
	ListView_InsertColumn(hListView,1,&lvCol);

	LoadString(GetInstance(),IDS_MASS_RENAME_PREVIEW_NAME,
		szTemp,SIZEOF_ARRAY(szTemp));
	lvCol.mask		= LVCF_TEXT;
	lvCol.pszText	= szTemp;
	ListView_InsertColumn(hListView,2,&lvCol);

	SendMessage(hListView,LVM_SETCOLUMNWIDTH,0,m_pmrdps->m_iColumnWidth1);
	SendMessage(hListView,LVM_SETCOLUMNWIDTH,1,m_pmrdps->m_iColumnWidth2);

	LVITEM lvItem;
	SHFILEINFO shfi;
	TCHAR szFilename[MAX_PATH];
	int iItem = 0;

	/* Add each file to the listview, along with its icon. */
	for(const auto &strFilename : m_FullFilenameList)
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

	SetDlgItemText(m_hDlg,IDC_MASSRENAME_EDIT,_T("/F"));
	SendMessage(GetDlgItem(m_hDlg,IDC_MASSRENAME_EDIT),
		EM_SETSEL,0,-1);
	SetFocus(GetDlgItem(m_hDlg,IDC_MASSRENAME_EDIT));

	m_pmrdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

wil::unique_hicon CMassRenameDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return IconResourceLoader::LoadIconFromPNGAndScale(Icon::MassRename, iconWidth, iconHeight);
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

INT_PTR CMassRenameDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

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

				for(const auto &strFilename : m_FullFilenameList)
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
						reinterpret_cast<LPARAM>(_T("/F")));
					break;

				case IDM_MASSRENAME_BASENAME:
					SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,
						reinterpret_cast<LPARAM>(_T("/B")));
					break;

				case IDM_MASSRENAME_EXTENSION:
					SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,
						reinterpret_cast<LPARAM>(_T("/E")));
					break;

				case IDM_MASSRENAME_COUNTER:
					SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,
						reinterpret_cast<LPARAM>(_T("/N")));
					break;

				case IDM_MASSRENAME_LCASE:
					SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,
						reinterpret_cast<LPARAM>(_T("/L")));
					break;

				case IDM_MASSRENAME_UCASE:
					SendDlgItemMessage(m_hDlg,IDC_MASSRENAME_EDIT,EM_REPLACESEL,TRUE,
						reinterpret_cast<LPARAM>(_T("/U")));
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

INT_PTR CMassRenameDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CMassRenameDialog::OnOk()
{
	TCHAR szNamePattern[MAX_PATH];

	GetDlgItemText(m_hDlg,IDC_MASSRENAME_EDIT,
		szNamePattern,SIZEOF_ARRAY(szNamePattern));

	if(lstrlen(szNamePattern) == 0)
	{
		EndDialog(m_hDlg,1);
		return;
	}

	std::list<CFileActionHandler::RenamedItem_t> RenamedItemList;
	int iItem = 0;

	for(const auto &strOldFilename : m_FullFilenameList)
	{
		TCHAR szFilename[MAX_PATH];
		StringCchCopy(szFilename,SIZEOF_ARRAY(szFilename),
			strOldFilename.c_str());
		PathStripPath(szFilename);

		std::wstring strNewFilename;
		ProcessFileName(szNamePattern,szFilename,iItem,strNewFilename);

		StringCchCopy(szFilename,SIZEOF_ARRAY(szFilename),
			strOldFilename.c_str());
		PathRemoveFileSpec(szFilename);
		strNewFilename = szFilename + std::wstring(_T("\\")) + strNewFilename;

		CFileActionHandler::RenamedItem_t RenamedItem;
		RenamedItem.strOldFilename = strOldFilename;
		RenamedItem.strNewFilename = strNewFilename;
		RenamedItemList.push_back(RenamedItem);

		iItem++;
	}

	m_pFileActionHandler->RenameFiles(RenamedItemList);

	EndDialog(m_hDlg,1);
}

void CMassRenameDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CMassRenameDialog::SaveState()
{
	m_pmrdps->SaveDialogPosition(m_hDlg);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MASSRENAME_FILELISTVIEW);
	m_pmrdps->m_iColumnWidth1 = ListView_GetColumnWidth(hListView,0);
	m_pmrdps->m_iColumnWidth2 = ListView_GetColumnWidth(hListView,1);

	m_pmrdps->m_bStateSaved = TRUE;
}

void CMassRenameDialog::ProcessFileName(const std::wstring &strTarget,
	const std::wstring &strFilename,int iFileIndex,std::wstring &strOutput)
{
	TCHAR szBaseName[MAX_PATH];
	StringCchCopy(szBaseName, SIZEOF_ARRAY(szBaseName), strFilename.c_str());
	PathRemoveExtension(szBaseName);

	TCHAR *pExt = PathFindExtension(strFilename.c_str());

	size_t iPos;

	strOutput = strTarget;

	std::wregex rxPattern;
	rxPattern.assign(_T("/[0]*N"));

	bool bStop = false;

	while(!bStop)
	{
		std::match_results<std::wstring::const_iterator> mr;
		std::wstring::const_iterator itrStart = strOutput.begin();
		std::wstring::const_iterator itrEnd = strOutput.end();

		if(std::regex_search(itrStart,itrEnd,mr,rxPattern))
		{
			std::wstringstream ss;

			/* The minimum length is the number of zeros present plus one. */
			ss << std::setfill(_T('0')) << std::setw((mr.length() - 2) + 1) << iFileIndex;

			strOutput.replace(mr.position(),mr.length(),ss.str());
		}
		else
		{
			bStop = true;
		}
	}

	while((iPos = strOutput.find(_T("/F"))) != std::wstring::npos)
	{
		strOutput.replace(iPos,2,strFilename);
	}

	while((iPos = strOutput.find(_T("/B"))) != std::wstring::npos)
	{
		strOutput.replace(iPos,2,szBaseName);
	}

	while((iPos = strOutput.find(_T("/E"))) != std::wstring::npos)
	{
		strOutput.replace(iPos,2,pExt);
	}

	while((iPos = strOutput.find(_T("/L"))) != std::wstring::npos)
	{
		strOutput.replace(iPos,2,strFilename);
		boost::to_lower(strOutput);
	}

	while((iPos = strOutput.find(_T("/U"))) != std::wstring::npos)
	{
		strOutput.replace(iPos,2,strFilename);
		boost::to_upper(strOutput);
	}
}

CMassRenameDialogPersistentSettings::CMassRenameDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_iColumnWidth1 = DEFAULT_MASS_RENAME_COLUMN_WIDTH;
	m_iColumnWidth2 = DEFAULT_MASS_RENAME_COLUMN_WIDTH;
}

CMassRenameDialogPersistentSettings::~CMassRenameDialogPersistentSettings()
{

}

CMassRenameDialogPersistentSettings& CMassRenameDialogPersistentSettings::GetInstance()
{
	static CMassRenameDialogPersistentSettings sfadps;
	return sfadps;
}

void CMassRenameDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_COLUMN_WIDTH_1, m_iColumnWidth1);
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_COLUMN_WIDTH_2, m_iColumnWidth2);
}

void CMassRenameDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_COLUMN_WIDTH_1, reinterpret_cast<DWORD *>(&m_iColumnWidth1));
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_COLUMN_WIDTH_2, reinterpret_cast<DWORD *>(&m_iColumnWidth2));
}

void CMassRenameDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_COLUMN_WIDTH_1, NXMLSettings::EncodeIntValue(m_iColumnWidth1));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_COLUMN_WIDTH_2, NXMLSettings::EncodeIntValue(m_iColumnWidth2));
}

void CMassRenameDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(lstrcmpi(bstrName, SETTING_COLUMN_WIDTH_1) == 0)
	{
		m_iColumnWidth1 = NXMLSettings::DecodeIntValue(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_COLUMN_WIDTH_2) == 0)
	{
		m_iColumnWidth2 = NXMLSettings::DecodeIntValue(bstrValue);
	}
}