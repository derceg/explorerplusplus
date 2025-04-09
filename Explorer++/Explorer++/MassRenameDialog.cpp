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
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include <boost/locale.hpp>
#include <iomanip>
#include <list>
#include <regex>

const TCHAR MassRenameDialogPersistentSettings::SETTINGS_KEY[] = _T("MassRename");

const TCHAR MassRenameDialogPersistentSettings::SETTING_COLUMN_WIDTH_1[] = _T("ColumnWidth1");
const TCHAR MassRenameDialogPersistentSettings::SETTING_COLUMN_WIDTH_2[] = _T("ColumnWidth2");

MassRenameDialog::MassRenameDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance,
	HWND hParent, ThemeManager *themeManager, const std::list<std::wstring> &FullFilenameList,
	IconResourceLoader *iconResourceLoader, FileActionHandler *pFileActionHandler) :
	ThemedDialog(resourceLoader, resourceInstance, IDD_MASSRENAME, hParent, DialogSizingType::Both,
		themeManager),
	m_FullFilenameList(FullFilenameList),
	m_iconResourceLoader(iconResourceLoader),
	m_pFileActionHandler(pFileActionHandler)
{
	m_persistentSettings = &MassRenameDialogPersistentSettings::GetInstance();
}

INT_PTR MassRenameDialog::OnInitDialog()
{
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hDlg);
	m_moreIcon = m_iconResourceLoader->LoadIconFromPNGForDpi(Icon::ArrowRight, 16, 16, dpi);
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

	SetDlgItemText(m_hDlg, IDC_MASSRENAME_EDIT, _T("/F"));
	SendMessage(GetDlgItem(m_hDlg, IDC_MASSRENAME_EDIT), EM_SETSEL, 0, -1);
	SetFocus(GetDlgItem(m_hDlg, IDC_MASSRENAME_EDIT));

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

wil::unique_hicon MassRenameDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_iconResourceLoader->LoadIconFromPNGAndScale(Icon::MassRename, iconWidth, iconHeight);
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

			LVITEM lvItem;
			std::wstring strNewFilename;
			TCHAR szFilename[MAX_PATH];
			TCHAR szNewFilename[MAX_PATH];
			int iItem = 0;

			for (const auto &strFilename : m_FullFilenameList)
			{
				StringCchCopy(szFilename, std::size(szFilename), strFilename.c_str());
				PathStripPath(szFilename);

				ProcessFileName(szNamePattern, szFilename, iItem, strNewFilename);

				StringCchCopy(szNewFilename, std::size(szNewFilename), strNewFilename.c_str());

				lvItem.mask = LVIF_TEXT;
				lvItem.iItem = iItem;
				lvItem.iSubItem = 1;
				lvItem.pszText = szNewFilename;
				ListView_SetItem(hListView, &lvItem);

				iItem++;
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
		{
			HMENU hMenu = GetSubMenu(
				LoadMenu(GetResourceInstance(), MAKEINTRESOURCE(IDR_MASSRENAME_MENU)), 0);

			RECT rc;
			GetWindowRect(GetDlgItem(m_hDlg, IDC_MASSRENAME_MORE), &rc);

			UINT uCmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_VERTICAL | TPM_RETURNCMD,
				rc.right, rc.top, 0, m_hDlg, nullptr);

			switch (uCmd)
			{
			case IDM_MASSRENAME_FILENAME:
				SendDlgItemMessage(m_hDlg, IDC_MASSRENAME_EDIT, EM_REPLACESEL, TRUE,
					reinterpret_cast<LPARAM>(_T("/F")));
				break;

			case IDM_MASSRENAME_BASENAME:
				SendDlgItemMessage(m_hDlg, IDC_MASSRENAME_EDIT, EM_REPLACESEL, TRUE,
					reinterpret_cast<LPARAM>(_T("/B")));
				break;

			case IDM_MASSRENAME_EXTENSION:
				SendDlgItemMessage(m_hDlg, IDC_MASSRENAME_EDIT, EM_REPLACESEL, TRUE,
					reinterpret_cast<LPARAM>(_T("/E")));
				break;

			case IDM_MASSRENAME_COUNTER:
				SendDlgItemMessage(m_hDlg, IDC_MASSRENAME_EDIT, EM_REPLACESEL, TRUE,
					reinterpret_cast<LPARAM>(_T("/N")));
				break;

			case IDM_MASSRENAME_LCASE:
				SendDlgItemMessage(m_hDlg, IDC_MASSRENAME_EDIT, EM_REPLACESEL, TRUE,
					reinterpret_cast<LPARAM>(_T("/L")));
				break;

			case IDM_MASSRENAME_UCASE:
				SendDlgItemMessage(m_hDlg, IDC_MASSRENAME_EDIT, EM_REPLACESEL, TRUE,
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

INT_PTR MassRenameDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
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
	int iItem = 0;

	for (const auto &strOldFilename : m_FullFilenameList)
	{
		TCHAR szFilename[MAX_PATH];
		StringCchCopy(szFilename, std::size(szFilename), strOldFilename.c_str());
		PathStripPath(szFilename);

		std::wstring strNewFilename;
		ProcessFileName(szNamePattern, szFilename, iItem, strNewFilename);

		StringCchCopy(szFilename, std::size(szFilename), strOldFilename.c_str());
		PathRemoveFileSpec(szFilename);
		strNewFilename = szFilename + std::wstring(_T("\\")) + strNewFilename;

		FileActionHandler::RenamedItem_t renamedItem;
		renamedItem.strOldFilename = strOldFilename;
		renamedItem.strNewFilename = strNewFilename;
		renamedItemList.push_back(renamedItem);

		iItem++;
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

void MassRenameDialog::ProcessFileName(const std::wstring &strTarget,
	const std::wstring &strFilename, int iFileIndex, std::wstring &strOutput)
{
	TCHAR szBaseName[MAX_PATH];
	StringCchCopy(szBaseName, std::size(szBaseName), strFilename.c_str());
	PathRemoveExtension(szBaseName);

	TCHAR *pExt = PathFindExtension(strFilename.c_str());

	size_t iPos;

	strOutput = strTarget;

	std::wregex rxPattern;
	rxPattern.assign(_T("/[0]*N"));

	bool bStop = false;

	while (!bStop)
	{
		std::match_results<std::wstring::const_iterator> mr;
		std::wstring::const_iterator itrStart = strOutput.begin();
		std::wstring::const_iterator itrEnd = strOutput.end();

		if (std::regex_search(itrStart, itrEnd, mr, rxPattern))
		{
			std::wstringstream ss;

			/* The minimum length is the number of zeros present plus one. */
			ss << std::setfill(_T('0')) << std::setw((mr.length() - 2) + 1) << iFileIndex;

			strOutput.replace(mr.position(), mr.length(), ss.str());
		}
		else
		{
			bStop = true;
		}
	}

	while ((iPos = strOutput.find(_T("/F"))) != std::wstring::npos)
	{
		strOutput.replace(iPos, 2, strFilename);
	}

	while ((iPos = strOutput.find(_T("/B"))) != std::wstring::npos)
	{
		strOutput.replace(iPos, 2, szBaseName);
	}

	while ((iPos = strOutput.find(_T("/E"))) != std::wstring::npos)
	{
		strOutput.replace(iPos, 2, pExt);
	}

	while ((iPos = strOutput.find(_T("/L"))) != std::wstring::npos)
	{
		strOutput.replace(iPos, 2, strFilename);
		strOutput = boost::locale::to_lower(strOutput);
	}

	while ((iPos = strOutput.find(_T("/U"))) != std::wstring::npos)
	{
		strOutput.replace(iPos, 2, strFilename);
		strOutput = boost::locale::to_upper(strOutput);
	}
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
