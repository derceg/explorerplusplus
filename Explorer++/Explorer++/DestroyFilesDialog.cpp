/******************************************************************
 *
 * Project: Explorer++
 * File: DestroyFilesDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Destroy Files' dialog and associated messages.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++_internal.h"
#include "DestroyFilesDialog.h"
#include "MainResource.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"


const TCHAR CDestroyFilesDialogPersistentSettings::SETTINGS_KEY[] = _T("DestroyFiles");

CDestroyFilesDialog::CDestroyFilesDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,std::list<std::wstring> FullFilenameList) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_FullFilenameList = FullFilenameList;

	m_pdfdps = &CDestroyFilesDialogPersistentSettings::GetInstance();
}

CDestroyFilesDialog::~CDestroyFilesDialog()
{

}

BOOL CDestroyFilesDialog::OnInitDialog()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_DESTROYFILES_LISTVIEW);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	SetWindowTheme(hListView,L"Explorer",NULL);

	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	LVCOLUMN lvColumn;
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("File");
	ListView_InsertColumn(hListView,0,&lvColumn);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Type");
	ListView_InsertColumn(hListView,1,&lvColumn);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Size");
	ListView_InsertColumn(hListView,2,&lvColumn);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Date Modified");
	ListView_InsertColumn(hListView,3,&lvColumn);

	int iItem = 0;

	for each(auto strFullFilename in m_FullFilenameList)
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

		/* TODO: Friendly dates global. */
		TCHAR szDateModified[32];
		CreateFileTimeString(&wfad.ftLastWriteTime,szDateModified,
			SIZEOF_ARRAY(szDateModified),TRUE);
		ListView_SetItemText(hListView,iItem,3,szDateModified);

		iItem++;
	}

	ListView_SetColumnWidth(hListView,0,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView,1,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView,2,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView,3,LVSCW_AUTOSIZE_USEHEADER);

	switch(m_pdfdps->m_uOverwriteMethod)
	{
	case OVERWRITE_ONEPASS:
		CheckDlgButton(m_hDlg,IDC_DESTROYFILES_RADIO_ONEPASS,BST_CHECKED);
		break;

	case OVERWRITE_THREEPASS:
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

BOOL CDestroyFilesDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
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

BOOL CDestroyFilesDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CDestroyFilesDialog::SaveState()
{
	m_pdfdps->SaveDialogPosition(m_hDlg);

	if(IsDlgButtonChecked(m_hDlg,IDC_DESTROYFILES_RADIO_ONEPASS) == BST_CHECKED)
	{
		m_pdfdps->m_uOverwriteMethod = OVERWRITE_ONEPASS;
	}
	else
	{
		m_pdfdps->m_uOverwriteMethod = OVERWRITE_THREEPASS;
	}

	m_pdfdps->m_bStateSaved = TRUE;
}

void CDestroyFilesDialog::OnOk()
{
	/* The default button in this message box will be the second
	button (i.e. the no button). */
	/* TODO: Move string into string table. */
	int iRes = MessageBox(m_hDlg,
	_T("Files that are destroyed will be \
permanently deleted, and will NOT be recoverable.\n\n\
Are you sure you want to continue?"),
	NExplorerplusplus::WINDOW_NAME,MB_ICONWARNING|MB_SETFOREGROUND|
	MB_YESNO|MB_DEFBUTTON2);

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
	/* TODO: Perform in background thread. */
	//for each(auto strFullFilename in m_FullFilenameList)
	//{
	//	//DeleteFileSecurely(strFullFilename.c_str(),uOverwriteMethod);
	//}

	EndDialog(m_hDlg,1);
}

CDestroyFilesDialogPersistentSettings::CDestroyFilesDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_uOverwriteMethod = OVERWRITE_ONEPASS;
}

CDestroyFilesDialogPersistentSettings::~CDestroyFilesDialogPersistentSettings()
{
	
}

CDestroyFilesDialogPersistentSettings& CDestroyFilesDialogPersistentSettings::GetInstance()
{
	static CDestroyFilesDialogPersistentSettings dfdps;
	return dfdps;
}

void CDestroyFilesDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveDwordToRegistry(hKey,_T("OverwriteMethod"),m_uOverwriteMethod);
}

void CDestroyFilesDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadDwordFromRegistry(hKey,_T("OverwriteMethod"),reinterpret_cast<LPDWORD>(&m_uOverwriteMethod));
}

void CDestroyFilesDialogPersistentSettings::SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,
	MSXML2::IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("OverwriteMethod"),NXMLSettings::EncodeIntValue(m_uOverwriteMethod));
}

void CDestroyFilesDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(lstrcmpi(bstrName,_T("OverwriteMethod")) == 0)
	{
		m_uOverwriteMethod = NXMLSettings::DecodeIntValue(bstrValue);
	}
}