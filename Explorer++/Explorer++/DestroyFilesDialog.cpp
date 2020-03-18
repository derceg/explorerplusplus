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

const TCHAR DestroyFilesDialogPersistentSettings::SETTINGS_KEY[] = _T("DestroyFiles");

const TCHAR DestroyFilesDialogPersistentSettings::SETTING_OVERWRITE_METHOD[] = _T("OverwriteMethod");

DestroyFilesDialog::DestroyFilesDialog(HINSTANCE hInstance,
	HWND hParent, const std::list<std::wstring> &FullFilenameList,
	BOOL bShowFriendlyDates) :
	BaseDialog(hInstance, IDD_DESTROYFILES, hParent, true)
{
	m_FullFilenameList = FullFilenameList;
	m_bShowFriendlyDates = bShowFriendlyDates;

	m_pdfdps = &DestroyFilesDialogPersistentSettings::GetInstance();
}

INT_PTR DestroyFilesDialog::OnInitDialog()
{
	m_icon.reset(LoadIcon(GetModuleHandle(nullptr),MAKEINTRESOURCE(IDI_MAIN)));
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_icon.get()));

	HWND hListView = GetDlgItem(m_hDlg,IDC_DESTROYFILES_LISTVIEW);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(nullptr,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	SetWindowTheme(hListView,L"Explorer", nullptr);

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

void DestroyFilesDialog::GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
	std::list<ResizableDialog::Control_t> &ControlList)
{
	dsc = BaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	ResizableDialog::Control_t control;

	control.iID = IDC_DESTROYFILES_LISTVIEW;
	control.Type = ResizableDialog::TYPE_RESIZE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(control);

	control.iID = IDC_GROUP;
	control.Type = ResizableDialog::TYPE_RESIZE;
	control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(control);

	control.iID = IDC_GROUP;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(control);

	control.iID = IDC_DESTROYFILES_RADIO_ONEPASS;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(control);

	control.iID = IDC_DESTROYFILES_RADIO_THREEPASS;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(control);

	control.iID = IDC_DESTROYFILES_STATIC_WARNING_MESSAGE;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(control);

	control.iID = IDC_DESTROYFILES_STATIC_WARNING_MESSAGE;
	control.Type = ResizableDialog::TYPE_RESIZE;
	control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(control);

	control.iID = IDOK;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(control);

	control.iID = IDCANCEL;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(control);

	control.iID = IDC_GRIPPER;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(control);
}

INT_PTR DestroyFilesDialog::OnCtlColorStatic(HWND hwnd,HDC hdc)
{
	if(hwnd == GetDlgItem(m_hDlg,IDC_DESTROYFILES_STATIC_WARNING_MESSAGE))
	{
		SetTextColor(hdc,RGB(255,0,0));
		SetBkMode(hdc,TRANSPARENT);
		return reinterpret_cast<INT_PTR>(GetStockObject(NULL_BRUSH));
	}

	return 0;
}

INT_PTR DestroyFilesDialog::OnCommand(WPARAM wParam,LPARAM lParam)
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

INT_PTR DestroyFilesDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void DestroyFilesDialog::SaveState()
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

void DestroyFilesDialog::OnOk()
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

void DestroyFilesDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void DestroyFilesDialog::OnConfirmDestroy()
{
	NFileOperations::OverwriteMethod_t overwriteMethod;

	if(IsDlgButtonChecked(m_hDlg,IDC_DESTROYFILES_RADIO_ONEPASS) == BST_CHECKED)
	{
		overwriteMethod = NFileOperations::OVERWRITE_ONEPASS;
	}
	else
	{
		overwriteMethod = NFileOperations::OVERWRITE_THREEPASS;
	}

	/* TODO: Perform in background thread. */
	for(const auto &strFullFilename : m_FullFilenameList)
	{
		DeleteFileSecurely(strFullFilename,overwriteMethod);
	}

	EndDialog(m_hDlg,1);
}

DestroyFilesDialogPersistentSettings::DestroyFilesDialogPersistentSettings() :
DialogSettings(SETTINGS_KEY)
{
	m_uOverwriteMethod = NFileOperations::OVERWRITE_ONEPASS;
}

DestroyFilesDialogPersistentSettings& DestroyFilesDialogPersistentSettings::GetInstance()
{
	static DestroyFilesDialogPersistentSettings dfdps;
	return dfdps;
}

void DestroyFilesDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_OVERWRITE_METHOD, m_uOverwriteMethod);
}

void DestroyFilesDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_OVERWRITE_METHOD, reinterpret_cast<LPDWORD>(&m_uOverwriteMethod));
}

void DestroyFilesDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_OVERWRITE_METHOD, NXMLSettings::EncodeIntValue(m_uOverwriteMethod));
}

void DestroyFilesDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(lstrcmpi(bstrName, SETTING_OVERWRITE_METHOD) == 0)
	{
		m_uOverwriteMethod = static_cast<NFileOperations::OverwriteMethod_t>(NXMLSettings::DecodeIntValue(bstrValue));
	}
}