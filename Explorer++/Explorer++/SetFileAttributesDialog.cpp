// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SetFileAttributesDialog.h"
#include "MainResource.h"
#include "../Helper/TimeHelper.h"

const TCHAR SetFileAttributesDialogPersistentSettings::SETTINGS_KEY[] = _T("SetFileAttributes");

SetFileAttributesDialog::SetFileAttributesDialog(const ResourceLoader *resourceLoader, HWND hParent,
	const std::vector<SetFileAttributesItem> &items) :
	BaseDialog(resourceLoader, IDD_SETFILEATTRIBUTES, hParent, DialogSizingType::None),
	m_items(items)
{
	CHECK(!m_items.empty());

	m_psfadps = &SetFileAttributesDialogPersistentSettings::GetInstance();
}

INT_PTR SetFileAttributesDialog::OnInitDialog()
{
	InitializeAttributesStructure();
	InitializeDateFields();

	int numItems = static_cast<int>(m_items.size());

	int numArchived = 0;
	int numHidden = 0;
	int numSystem = 0;
	int numReadOnly = 0;
	int numNotIndexed = 0;

	for (const auto &item : m_items)
	{
		if (WI_IsFlagSet(item.findData.dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE))
		{
			numArchived++;
		}

		if (WI_IsFlagSet(item.findData.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
		{
			numHidden++;
		}

		if (WI_IsFlagSet(item.findData.dwFileAttributes, FILE_ATTRIBUTE_SYSTEM))
		{
			numSystem++;
		}

		if (WI_IsFlagSet(item.findData.dwFileAttributes, FILE_ATTRIBUTE_READONLY))
		{
			numReadOnly++;
		}

		if (WI_IsFlagSet(item.findData.dwFileAttributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
		{
			numNotIndexed++;
		}
	}

	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_ARCHIVE),
		numArchived == 0 || numArchived == numItems);
	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_HIDDEN), numHidden == 0 || numHidden == numItems);
	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_SYSTEM), numSystem == 0 || numSystem == numItems);
	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_READONLY),
		numReadOnly == 0 || numReadOnly == numItems);
	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_NOT_INDEXED),
		numNotIndexed == 0 || numNotIndexed == numItems);

	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_ARCHIVE), numArchived, numItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_HIDDEN), numHidden, numItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_SYSTEM), numSystem, numItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_READONLY), numReadOnly, numItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_NOT_INDEXED), numNotIndexed, numItems);

	m_bModificationDateEnabled = FALSE;
	m_bCreationDateEnabled = FALSE;
	m_bAccessDateEnabled = FALSE;

	m_psfadps->RestoreDialogPosition(m_hDlg, false);

	return 0;
}

void SetFileAttributesDialog::InitializeDateFields()
{
	const auto &firstItem = m_items[0];

	/* Use the dates of the first file... */
	FileTimeToLocalSystemTime(&firstItem.findData.ftLastWriteTime, &m_LocalWrite);
	FileTimeToLocalSystemTime(&firstItem.findData.ftCreationTime, &m_LocalCreation);
	FileTimeToLocalSystemTime(&firstItem.findData.ftLastAccessTime, &m_LocalAccess);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_MODIFICATIONDATE), GDT_VALID, &m_LocalWrite);
	DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_MODIFICATIONTIME), GDT_VALID, &m_LocalWrite);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_CREATIONDATE), GDT_VALID, &m_LocalCreation);
	DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_CREATIONTIME), GDT_VALID, &m_LocalCreation);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_ACCESSDATE), GDT_VALID, &m_LocalAccess);
	DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_ACCESSTIME), GDT_VALID, &m_LocalAccess);

	/* All date/time fields are disabled initially. */
	DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_MODIFICATIONDATE), GDT_NONE, NULL);
	EnableWindow(GetDlgItem(m_hDlg, IDC_MODIFICATIONTIME), FALSE);
	EnableWindow(GetDlgItem(m_hDlg, IDC_MODIFICATION_RESET), FALSE);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_CREATIONDATE), GDT_NONE, NULL);
	EnableWindow(GetDlgItem(m_hDlg, IDC_CREATIONTIME), FALSE);
	EnableWindow(GetDlgItem(m_hDlg, IDC_CREATION_RESET), FALSE);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_ACCESSDATE), GDT_NONE, NULL);
	EnableWindow(GetDlgItem(m_hDlg, IDC_ACCESSTIME), FALSE);
	EnableWindow(GetDlgItem(m_hDlg, IDC_ACCESS_RESET), FALSE);
}

void SetFileAttributesDialog::InitializeAttributesStructure()
{
	Attribute_t attribute;

	attribute.Attribute = FILE_ATTRIBUTE_ARCHIVE;
	attribute.uControlId = IDC_CHECK_ARCHIVE;
	m_AttributeList.push_back(attribute);

	attribute.Attribute = FILE_ATTRIBUTE_HIDDEN;
	attribute.uControlId = IDC_CHECK_HIDDEN;
	m_AttributeList.push_back(attribute);

	attribute.Attribute = FILE_ATTRIBUTE_SYSTEM;
	attribute.uControlId = IDC_CHECK_SYSTEM;
	m_AttributeList.push_back(attribute);

	attribute.Attribute = FILE_ATTRIBUTE_READONLY;
	attribute.uControlId = IDC_CHECK_READONLY;
	m_AttributeList.push_back(attribute);

	attribute.Attribute = FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
	attribute.uControlId = IDC_CHECK_NOT_INDEXED;
	m_AttributeList.push_back(attribute);
}

INT_PTR SetFileAttributesDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDC_MODIFICATION_RESET:
		OnDateReset(DateTimeType::Modified);
		break;

	case IDC_CREATION_RESET:
		OnDateReset(DateTimeType::Created);
		break;

	case IDC_ACCESS_RESET:
		OnDateReset(DateTimeType::Accessed);
		break;

	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

INT_PTR SetFileAttributesDialog::OnNotify(NMHDR *pnmhdr)
{
	switch (pnmhdr->code)
	{
	case DTN_DATETIMECHANGE:
	{
		auto *pdtc = reinterpret_cast<NMDATETIMECHANGE *>(pnmhdr);

		switch (pnmhdr->idFrom)
		{
		case IDC_MODIFICATIONDATE:
			m_bModificationDateEnabled = (pdtc->dwFlags == GDT_VALID);
			EnableWindow(GetDlgItem(m_hDlg, IDC_MODIFICATIONTIME), pdtc->dwFlags == GDT_VALID);
			EnableWindow(GetDlgItem(m_hDlg, IDC_MODIFICATION_RESET), pdtc->dwFlags == GDT_VALID);
			break;

		case IDC_CREATIONDATE:
			m_bCreationDateEnabled = (pdtc->dwFlags == GDT_VALID);
			EnableWindow(GetDlgItem(m_hDlg, IDC_CREATIONTIME), pdtc->dwFlags == GDT_VALID);
			EnableWindow(GetDlgItem(m_hDlg, IDC_CREATION_RESET), pdtc->dwFlags == GDT_VALID);
			break;

		case IDC_ACCESSDATE:
			m_bAccessDateEnabled = (pdtc->dwFlags == GDT_VALID);
			EnableWindow(GetDlgItem(m_hDlg, IDC_ACCESSTIME), pdtc->dwFlags == GDT_VALID);
			EnableWindow(GetDlgItem(m_hDlg, IDC_ACCESS_RESET), pdtc->dwFlags == GDT_VALID);
			break;
		}
	}
	break;
	}

	return 0;
}

INT_PTR SetFileAttributesDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void SetFileAttributesDialog::OnOk()
{
	FILETIME *plw = nullptr;
	FILETIME *plc = nullptr;
	FILETIME *pla = nullptr;
	FILETIME lastWriteTime;
	FILETIME creationTime;
	FILETIME accessTime;
	DWORD allFileAttributes = FILE_ATTRIBUTE_NORMAL;
	DWORD fileAttributes;

	if (m_bModificationDateEnabled)
	{
		SYSTEMTIME localWrite;
		SYSTEMTIME localWriteDate;
		SYSTEMTIME localWriteTime;

		DateTime_GetSystemtime(GetDlgItem(m_hDlg, IDC_MODIFICATIONDATE), &localWriteDate);
		DateTime_GetSystemtime(GetDlgItem(m_hDlg, IDC_MODIFICATIONTIME), &localWriteTime);

		MergeDateTime(&localWrite, &localWriteDate, &localWriteTime);

		LocalSystemTimeToFileTime(&localWrite, &lastWriteTime);
		plw = &lastWriteTime;
	}

	if (m_bCreationDateEnabled)
	{
		SYSTEMTIME localCreation;
		SYSTEMTIME localCreationDate;
		SYSTEMTIME localCreationTime;

		DateTime_GetSystemtime(GetDlgItem(m_hDlg, IDC_CREATIONDATE), &localCreationDate);
		DateTime_GetSystemtime(GetDlgItem(m_hDlg, IDC_CREATIONTIME), &localCreationTime);

		MergeDateTime(&localCreation, &localCreationDate, &localCreationTime);

		LocalSystemTimeToFileTime(&localCreation, &creationTime);
		plc = &creationTime;
	}

	if (m_bAccessDateEnabled)
	{
		SYSTEMTIME localAccess;
		SYSTEMTIME localAccessDate;
		SYSTEMTIME localAccessTime;

		DateTime_GetSystemtime(GetDlgItem(m_hDlg, IDC_ACCESSDATE), &localAccessDate);
		DateTime_GetSystemtime(GetDlgItem(m_hDlg, IDC_ACCESSTIME), &localAccessTime);

		MergeDateTime(&localAccess, &localAccessDate, &localAccessTime);

		LocalSystemTimeToFileTime(&localAccess, &accessTime);
		pla = &accessTime;
	}

	/* Build up list of attributes. Add all positive
	attributes (i.e. those that are active for all files).
	Any attributes which are indeterminate will not change
	(note that they are per-file). */
	for (auto &attribute : m_AttributeList)
	{
		attribute.uChecked = static_cast<UINT>(
			SendMessage(GetDlgItem(m_hDlg, attribute.uControlId), BM_GETCHECK, 0, 0));

		if (attribute.uChecked == BST_CHECKED)
		{
			allFileAttributes |= attribute.Attribute;
		}
	}

	for (const auto &item : m_items)
	{
		fileAttributes = allFileAttributes;

		for (const auto &attribute : m_AttributeList)
		{
			/* If the check box is indeterminate, this attribute will
			stay the same (i.e. if a file had the attribute applied
			initially, it will still have it applied, and vice versa). */
			if (attribute.uChecked == BST_INDETERMINATE)
			{
				if (item.findData.dwFileAttributes & attribute.Attribute)
				{
					fileAttributes |= attribute.Attribute;
				}
			}
		}

		SetFileAttributes(item.path.c_str(), fileAttributes);

		HANDLE hFile = CreateFile(item.path.c_str(), FILE_WRITE_ATTRIBUTES, 0, nullptr,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			SetFileTime(hFile, plc, pla, plw);
			CloseHandle(hFile);
		}
	}

	EndDialog(m_hDlg, 1);
}

void SetFileAttributesDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

void SetFileAttributesDialog::OnDateReset(DateTimeType dateTimeType)
{
	switch (dateTimeType)
	{
	case DateTimeType::Modified:
		DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_MODIFICATIONDATE), GDT_VALID, &m_LocalWrite);
		DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_MODIFICATIONTIME), GDT_VALID, &m_LocalWrite);
		break;

	case DateTimeType::Created:
		DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_CREATIONDATE), GDT_VALID, &m_LocalCreation);
		DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_CREATIONTIME), GDT_VALID, &m_LocalCreation);
		break;

	case DateTimeType::Accessed:
		DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_ACCESSDATE), GDT_VALID, &m_LocalCreation);
		DateTime_SetSystemtime(GetDlgItem(m_hDlg, IDC_ACCESSTIME), GDT_VALID, &m_LocalCreation);
		break;
	}
}

void SetFileAttributesDialog::SaveState()
{
	m_psfadps->SaveDialogPosition(m_hDlg);

	m_psfadps->m_bStateSaved = TRUE;
}

void SetFileAttributesDialog::SetAttributeCheckState(HWND hwnd, int nAttributes, int nSelected)
{
	UINT checkState;

	if (nAttributes == 0)
	{
		checkState = BST_UNCHECKED;
	}
	else if (nAttributes == nSelected)
	{
		checkState = BST_CHECKED;
	}
	else
	{
		checkState = BST_INDETERMINATE;
	}

	SendMessage(hwnd, BM_SETCHECK, checkState, 0);
}

void SetFileAttributesDialog::ResetButtonState(HWND hwnd, BOOL bReset)
{
	if (!bReset)
	{
		return;
	}

	SendMessage(hwnd, BM_SETSTYLE, BS_AUTOCHECKBOX, MAKELPARAM(FALSE, 0));
}

SetFileAttributesDialogPersistentSettings::SetFileAttributesDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
}

SetFileAttributesDialogPersistentSettings &SetFileAttributesDialogPersistentSettings::GetInstance()
{
	static SetFileAttributesDialogPersistentSettings sfadps;
	return sfadps;
}
