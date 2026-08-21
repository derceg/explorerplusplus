// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MassRenameDialog.h"
#include "MainResource.h"
#include "MassRenameTokensMenu.h"
#include "NoOpMenuHelpTextHost.h"
#include "PopupMenuView.h"
#include "ResourceLoader.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include <Shlwapi.h>
#include <list>
#include <string>

using namespace std::string_literals;

const TCHAR MassRenameDialogPersistentSettings::SETTINGS_KEY[] = _T("MassRename");

const TCHAR MassRenameDialogPersistentSettings::SETTING_COLUMN_WIDTH_1[] = _T("ColumnWidth1");
const TCHAR MassRenameDialogPersistentSettings::SETTING_COLUMN_WIDTH_2[] = _T("ColumnWidth2");

MassRenameDialog *MassRenameDialog::Create(const ResourceLoader *resourceLoader, HWND hParent,
	const std::list<std::wstring> &FullFilenameList, FileActionHandler *pFileActionHandler,
	const AcceleratorManager *acceleratorManager)
{
	return new MassRenameDialog(resourceLoader, hParent, FullFilenameList, pFileActionHandler,
		acceleratorManager);
}

MassRenameDialog::MassRenameDialog(const ResourceLoader *resourceLoader, HWND hParent,
	const std::list<std::wstring> &FullFilenameList, FileActionHandler *pFileActionHandler,
	const AcceleratorManager *acceleratorManager) :
	BaseDialog(resourceLoader, IDD_MASSRENAME, hParent, DialogSizingType::Both),
	m_FullFilenameList(FullFilenameList),
	m_pFileActionHandler(pFileActionHandler),
	m_acceleratorManager(acceleratorManager)
{
	m_persistentSettings = &MassRenameDialogPersistentSettings::GetInstance();
}

INT_PTR MassRenameDialog::OnInitDialog()
{
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hDlg);
	m_moreIcon = m_resourceLoader->LoadIconFromPNGForDpi(Icon::ArrowRight, 16, 16, dpi);
	SendDlgItemMessage(m_hDlg, IDC_MASSRENAME_MORE, BM_SETIMAGE, IMAGE_ICON,
		reinterpret_cast<LPARAM>(m_moreIcon.get()));

	HWND hListView = GetDlgItem(m_hDlg, IDC_MASSRENAME_FILELISTVIEW);
	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES,
		LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(nullptr, &himlSmall);
	ListView_SetImageList(hListView, himlSmall, LVSIL_SMALL);

	LVCOLUMN lvCol;

	std::wstring currentNameText = m_resourceLoader->LoadString(IDS_MASS_RENAME_CURRENT_NAME);
	lvCol.mask = LVCF_TEXT;
	lvCol.pszText = currentNameText.data();
	ListView_InsertColumn(hListView, 1, &lvCol);

	std::wstring previewNameText = m_resourceLoader->LoadString(IDS_MASS_RENAME_PREVIEW_NAME);
	lvCol.mask = LVCF_TEXT;
	lvCol.pszText = previewNameText.data();
	ListView_InsertColumn(hListView, 2, &lvCol);

	SendMessage(hListView, LVM_SETCOLUMNWIDTH, 0, m_persistentSettings->m_iColumnWidth1);
	SendMessage(hListView, LVM_SETCOLUMNWIDTH, 1, m_persistentSettings->m_iColumnWidth2);

	LVITEM lvItem;
	SHFILEINFO shfi;
	TCHAR szFilename[MAX_PATH];
	int iItem = 0;

	/* Add each file to the listview, along with its icon. */
	for (const auto &strFilename : m_FullFilenameList)
	{
		SHGetFileInfo(strFilename.c_str(), 0, &shfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX);

		StringCchCopy(szFilename, std::size(szFilename), strFilename.c_str());
		PathStripPath(szFilename);

		lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 0;
		lvItem.iImage = shfi.iIcon;
		lvItem.pszText = szFilename;
		ListView_InsertItem(hListView, &lvItem);

		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 1;
		lvItem.pszText = szFilename;
		ListView_SetItem(hListView, &lvItem);

		iItem++;
	}

	SetDlgItemText(m_hDlg, IDC_MASSRENAME_EDIT,
		GetMassRenameTokenText(MassRenameToken::Filename).c_str());
	SendMessage(GetDlgItem(m_hDlg, IDC_MASSRENAME_EDIT), EM_SETSEL, 0, -1);
	SetFocus(GetDlgItem(m_hDlg, IDC_MASSRENAME_EDIT));

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

wil::unique_hicon MassRenameDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_resourceLoader->LoadIconFromPNGAndScale(Icon::MassRename, iconWidth, iconHeight);
}

std::vector<ResizableDialogControl> MassRenameDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MASSRENAME_EDIT), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MASSRENAME_MORE), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MASSRENAME_FILELISTVIEW), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDCANCEL), MovingType::Both, SizingType::None);
	return controls;
}

INT_PTR MassRenameDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
		{
			TCHAR szNamePattern[MAX_PATH];
			GetDlgItemText(m_hDlg, IDC_MASSRENAME_EDIT, szNamePattern, std::size(szNamePattern));

			HWND hListView = GetDlgItem(m_hDlg, IDC_MASSRENAME_FILELISTVIEW);
			int index = 0;

			for (const auto &path : m_FullFilenameList)
			{
				TCHAR filename[MAX_PATH];
				StringCchCopy(filename, std::size(filename), path.c_str());
				PathStripPath(filename);

				auto updatedFilename = ExpandMassRenamePattern(szNamePattern, filename, index);

				LVITEM lvItem = {};
				lvItem.mask = LVIF_TEXT;
				lvItem.iItem = index;
				lvItem.iSubItem = 1;
				lvItem.pszText = const_cast<wchar_t *>(updatedFilename.c_str());
				ListView_SetItem(hListView, &lvItem);

				index++;
			}
		}
		break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDC_MASSRENAME_MORE:
			OnShowTokensMenu();
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

INT_PTR MassRenameDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void MassRenameDialog::OnShowTokensMenu()
{
	RECT rc;
	auto res = GetWindowRect(GetDlgItem(m_hDlg, IDC_MASSRENAME_MORE), &rc);
	CHECK(res);

	auto tokenSelectedCallback = [this](MassRenameToken token)
	{
		SendDlgItemMessage(m_hDlg, IDC_MASSRENAME_EDIT, EM_REPLACESEL, true,
			reinterpret_cast<LPARAM>(GetMassRenameTokenText(token).c_str()));
	};

	PopupMenuView popupMenu(NoOpMenuHelpTextHost::GetInstance());
	MassRenameTokensMenu menu(&popupMenu, m_acceleratorManager, tokenSelectedCallback,
		m_resourceLoader);
	popupMenu.Show(m_hDlg, { rc.left, rc.top });
}

void MassRenameDialog::OnOk()
{
	TCHAR szNamePattern[MAX_PATH];

	GetDlgItemText(m_hDlg, IDC_MASSRENAME_EDIT, szNamePattern, std::size(szNamePattern));

	if (lstrlen(szNamePattern) == 0)
	{
		EndDialog(m_hDlg, 1);
		return;
	}

	std::list<FileActionHandler::RenamedItem_t> renamedItemList;
	int index = 0;

	for (const auto &strOldFilename : m_FullFilenameList)
	{
		TCHAR filename[MAX_PATH];
		StringCchCopy(filename, std::size(filename), strOldFilename.c_str());
		PathStripPath(filename);

		auto updatedFilename = ExpandMassRenamePattern(szNamePattern, filename, index);

		StringCchCopy(filename, std::size(filename), strOldFilename.c_str());
		PathRemoveFileSpec(filename);
		auto updatedPath = filename + L"\\"s + updatedFilename;

		FileActionHandler::RenamedItem_t renamedItem;
		renamedItem.strOldFilename = strOldFilename;
		renamedItem.strNewFilename = updatedPath;
		renamedItemList.push_back(renamedItem);

		index++;
	}

	m_pFileActionHandler->RenameFiles(renamedItemList);

	EndDialog(m_hDlg, 1);
}

void MassRenameDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

void MassRenameDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	HWND hListView = GetDlgItem(m_hDlg, IDC_MASSRENAME_FILELISTVIEW);
	m_persistentSettings->m_iColumnWidth1 = ListView_GetColumnWidth(hListView, 0);
	m_persistentSettings->m_iColumnWidth2 = ListView_GetColumnWidth(hListView, 1);

	m_persistentSettings->m_bStateSaved = TRUE;
}

MassRenameDialogPersistentSettings::MassRenameDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
	m_iColumnWidth1 = DEFAULT_MASS_RENAME_COLUMN_WIDTH;
	m_iColumnWidth2 = DEFAULT_MASS_RENAME_COLUMN_WIDTH;
}

MassRenameDialogPersistentSettings &MassRenameDialogPersistentSettings::GetInstance()
{
	static MassRenameDialogPersistentSettings sfadps;
	return sfadps;
}

void MassRenameDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::SaveDword(hKey, SETTING_COLUMN_WIDTH_1, m_iColumnWidth1);
	RegistrySettings::SaveDword(hKey, SETTING_COLUMN_WIDTH_2, m_iColumnWidth2);
}

void MassRenameDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_COLUMN_WIDTH_1, m_iColumnWidth1);
	RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_COLUMN_WIDTH_2, m_iColumnWidth2);
}

void MassRenameDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_COLUMN_WIDTH_1,
		XMLSettings::EncodeIntValue(m_iColumnWidth1));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_COLUMN_WIDTH_2,
		XMLSettings::EncodeIntValue(m_iColumnWidth2));
}

void MassRenameDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	if (lstrcmpi(bstrName, SETTING_COLUMN_WIDTH_1) == 0)
	{
		m_iColumnWidth1 = XMLSettings::DecodeIntValue(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_COLUMN_WIDTH_2) == 0)
	{
		m_iColumnWidth2 = XMLSettings::DecodeIntValue(bstrValue);
	}
}
