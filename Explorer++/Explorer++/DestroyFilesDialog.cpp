// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DestroyFilesDialog.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/StringHelper.h"
#include "../Helper/XMLSettings.h"

const TCHAR CDestroyFilesDialogPersistentSettings::SETTINGS_KEY[] = _T("DestroyFiles");

const TCHAR CDestroyFilesDialogPersistentSettings::SETTING_OVERWRITE_METHOD[] = _T("OverwriteMethod");

CDestroyFilesDialog::CDestroyFilesDialog(HINSTANCE hInstance,
	HWND hParent, std::list<std::wstring> FullFilenameList,
	BOOL bShowFriendlyDates) :
	CBaseDialog(hInstance, IDD_DESTROYFILES, hParent, true)
{
	m_FullFilenameList = FullFilenameList;
	m_bShowFriendlyDates = bShowFriendlyDates;

	m_pdfdps = &CDestroyFilesDialogPersistentSettings::GetInstance();
}

INT_PTR CDestroyFilesDialog::OnInitDialog()
{
	m_icon.reset(LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(IDI_MAIN)));
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_icon.get()));

	HWND hListView = GetDlgItem(m_hDlg,IDC_DESTROYFILES_LISTVIEW);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	SetWindowTheme(hListView,L"Explorer",NULL);

	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	LVCOLUMN lvColumn;
	TCHAR szTemp[128];

	LoadString(GetInstance(),IDS_DESTROY_FILES_COLUMN_FILE,
		szTemp,SIZEOF_ARRAY(szTemp));
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= szTemp;
	ListView_InsertColumn(hListView,0,&lvColumn);

	LoadString(GetInstance(),IDS_DESTROY_FILES_COLUMN_TYPE,
		szTemp,SIZEOF_ARRAY(szTemp));
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= szTemp;
	ListView_InsertColumn(hListView,1,&lvColumn);

	LoadString(GetInstance(),IDS_DESTROY_FILES_COLUMN_SIZE,
		szTemp,SIZEOF_ARRAY(szTemp));
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= szTemp;
	ListView_InsertColumn(hListView,2,&lvColumn);

	LoadString(GetInstance(),IDS_DESTROY_FILES_COLUMN_DATE_MODIFIED,
		szTemp,SIZEOF_ARRAY(szTemp));
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= szTemp;
	ListView_InsertColumn(hListView,3,&lvColumn);

	int iItem = 0;

	for(const auto &strFullFilename : m_FullFilenameList)
	{
		TCHAR szFullFilename[MAX_PATH];

		StringCchCopy(szFullFilename,SIZEOF_ARRAY(szFullFilename),
			strFullFilename.c_str());

		/* TODO: Perform in background thread. */
		SHFILEINFO shfi;
		SHGetFileInfo(szFullFilename,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX|
			SHGFI_TYPENAME);

		LVITEM lvItem;
		lvItem.mask		= LVIF_TEXT|LVIF_IMAGE;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= szFullFilename;
		lvItem.iImage	 = shfi.iIcon;
		ListView_InsertItem(hListView,&lvItem);

		ListView_SetItemText(hListView,iItem,1,shfi.szTypeName);

		WIN32_FILE_ATTRIBUTE_DATA wfad;
		GetFileAttributesEx(szFullFilename,GetFileExInfoStandard,&wfad);

		TCHAR szFileSize[32];
		ULARGE_INTEGER lFileSize = {wfad.nFileSizeLow,wfad.nFileSizeHigh};
		FormatSizeString(lFileSize,szFileSize,SIZEOF_ARRAY(szFileSize));
		ListView_SetItemText(hListView,iItem,2,szFileSize);

		TCHAR szDateModified[32];
		CreateFileTimeString(&wfad.ftLastWriteTime,szDateModified,
			SIZEOF_ARRAY(szDateModified),m_bShowFriendlyDates);
		ListView_SetItemText(hListView,iItem,3,szDateModified);

		iItem++;
	}

	ListView_SetColumnWidth(hListView,0,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView,1,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView,2,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView,3,LVSCW_AUTOSIZE_USEHEADER);

	switch(m_pdfdps->m_uOverwriteMethod)
	{
	case NFileOperations::OVERWRITE_ONEPASS:
		CheckDlgButton(m_hDlg,IDC_DESTROYFILES_RADIO_ONEPASS,BST_CHECKED);
		break;

	case NFileOperations::OVERWRITE_THREEPASS:
		CheckDlgButton(m_hDlg,IDC_DESTROYFILES_RADIO_THREEPASS,BST_CHECKED);
		break;
	}

	m_pdfdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

void CDestroyFilesDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_DESTROYFILES_LISTVIEW;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_GROUP;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_GROUP;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_DESTROYFILES_RADIO_ONEPASS;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_DESTROYFILES_RADIO_THREEPASS;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_DESTROYFILES_STATIC_WARNING_MESSAGE;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_DESTROYFILES_STATIC_WARNING_MESSAGE;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
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

INT_PTR CDestroyFilesDialog::OnCtlColorStatic(HWND hwnd,HDC hdc)
{
	if(hwnd == GetDlgItem(m_hDlg,IDC_DESTROYFILES_STATIC_WARNING_MESSAGE))
	{
		SetTextColor(hdc,RGB(255,0,0));
		SetBkMode(hdc,TRANSPARENT);
		return reinterpret_cast<INT_PTR>(GetStockObject(NULL_BRUSH));
	}

	return 0;
}

INT_PTR CDestroyFilesDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch(LOWORD(wParam))
	{
	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

INT_PTR CDestroyFilesDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CDestroyFilesDialog::SaveState()
{
	m_pdfdps->SaveDialogPosition(m_hDlg);

	if(IsDlgButtonChecked(m_hDlg,IDC_DESTROYFILES_RADIO_ONEPASS) == BST_CHECKED)
	{
		m_pdfdps->m_uOverwriteMethod = NFileOperations::OVERWRITE_ONEPASS;
	}
	else
	{
		m_pdfdps->m_uOverwriteMethod = NFileOperations::OVERWRITE_THREEPASS;
	}

	m_pdfdps->m_bStateSaved = TRUE;
}

void CDestroyFilesDialog::OnOk()
{
	TCHAR szConfirmation[128];
	LoadString(GetInstance(),IDS_DESTROY_FILES_CONFIRMATION,
		szConfirmation,SIZEOF_ARRAY(szConfirmation));

	/* The default button in this message box will be the second
	button (i.e. the no button). */
	int iRes = MessageBox(m_hDlg,szConfirmation,NExplorerplusplus::APP_NAME,
		MB_ICONWARNING|MB_SETFOREGROUND|MB_YESNO|MB_DEFBUTTON2);

	switch(iRes)
	{
		case IDYES:
			OnConfirmDestroy();
			break;

		default:
			EndDialog(m_hDlg,0);
			break;
	}
}

void CDestroyFilesDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CDestroyFilesDialog::OnConfirmDestroy()
{
	NFileOperations::OverwriteMethod_t OverwriteMethod;

	if(IsDlgButtonChecked(m_hDlg,IDC_DESTROYFILES_RADIO_ONEPASS) == BST_CHECKED)
	{
		OverwriteMethod = NFileOperations::OVERWRITE_ONEPASS;
	}
	else
	{
		OverwriteMethod = NFileOperations::OVERWRITE_THREEPASS;
	}

	/* TODO: Perform in background thread. */
	for(const auto &strFullFilename : m_FullFilenameList)
	{
		DeleteFileSecurely(strFullFilename,OverwriteMethod);
	}

	EndDialog(m_hDlg,1);
}

CDestroyFilesDialogPersistentSettings::CDestroyFilesDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_uOverwriteMethod = NFileOperations::OVERWRITE_ONEPASS;
}

CDestroyFilesDialogPersistentSettings& CDestroyFilesDialogPersistentSettings::GetInstance()
{
	static CDestroyFilesDialogPersistentSettings dfdps;
	return dfdps;
}

void CDestroyFilesDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_OVERWRITE_METHOD, m_uOverwriteMethod);
}

void CDestroyFilesDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_OVERWRITE_METHOD, reinterpret_cast<LPDWORD>(&m_uOverwriteMethod));
}

void CDestroyFilesDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_OVERWRITE_METHOD, NXMLSettings::EncodeIntValue(m_uOverwriteMethod));
}

void CDestroyFilesDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(lstrcmpi(bstrName, SETTING_OVERWRITE_METHOD) == 0)
	{
		m_uOverwriteMethod = static_cast<NFileOperations::OverwriteMethod_t>(NXMLSettings::DecodeIntValue(bstrValue));
	}
}