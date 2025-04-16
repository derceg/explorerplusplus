// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SetFileAttributesDialog.h"
#include "MainResource.h"
#include "../Helper/TimeHelper.h"
#include <list>

const TCHAR SetFileAttributesDialogPersistentSettings::SETTINGS_KEY[] = _T("SetFileAttributes");

SetFileAttributesDialog::SetFileAttributesDialog(const ResourceLoader *resourceLoader, HWND hParent,
	const std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo> &sfaiList) :
	BaseDialog(resourceLoader, IDD_SETFILEATTRIBUTES, hParent, DialogSizingType::None)
{
	assert(!sfaiList.empty());

	m_FileList = sfaiList;

	m_psfadps = &SetFileAttributesDialogPersistentSettings::GetInstance();
}

INT_PTR SetFileAttributesDialog::OnInitDialog()
{
	InitializeAttributesStructure();
	InitializeDateFields();

	int nItems = static_cast<int>(m_FileList.size());

	int nArchived = 0;
	int nHidden = 0;
	int nSystem = 0;
	int nReadOnly = 0;
	int nNotIndexed = 0;

	for (const auto &file : m_FileList)
	{
		if (WI_IsFlagSet(file.wfd.dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE))
		{
			nArchived++;
		}

		if (WI_IsFlagSet(file.wfd.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
		{
			nHidden++;
		}

		if (WI_IsFlagSet(file.wfd.dwFileAttributes, FILE_ATTRIBUTE_SYSTEM))
		{
			nSystem++;
		}

		if (WI_IsFlagSet(file.wfd.dwFileAttributes, FILE_ATTRIBUTE_READONLY))
		{
			nReadOnly++;
		}

		if (WI_IsFlagSet(file.wfd.dwFileAttributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
		{
			nNotIndexed++;
		}
	}

	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_ARCHIVE), nArchived == 0 || nArchived == nItems);
	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_HIDDEN), nHidden == 0 || nHidden == nItems);
	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_SYSTEM), nSystem == 0 || nSystem == nItems);
	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_READONLY), nReadOnly == 0 || nReadOnly == nItems);
	ResetButtonState(GetDlgItem(m_hDlg, IDC_CHECK_NOT_INDEXED),
		nNotIndexed == 0 || nNotIndexed == nItems);

	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_ARCHIVE), nArchived, nItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_HIDDEN), nHidden, nItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_SYSTEM), nSystem, nItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_READONLY), nReadOnly, nItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg, IDC_CHECK_NOT_INDEXED), nNotIndexed, nItems);

	m_bModificationDateEnabled = FALSE;
	m_bCreationDateEnabled = FALSE;
	m_bAccessDateEnabled = FALSE;

	m_psfadps->RestoreDialogPosition(m_hDlg, false);

	return 0;
}

void SetFileAttributesDialog::InitializeDateFields()
{
	WIN32_FIND_DATA *pwfd = &(m_FileList.begin()->wfd);

	/* Use the dates of the first file... */
	FileTimeToLocalSystemTime(&pwfd->ftLastWriteTime, &m_LocalWrite);
	FileTimeToLocalSystemTime(&pwfd->ftCreationTime, &m_LocalCreation);
	FileTimeToLocalSystemTime(&pwfd->ftLastAccessTime, &m_LocalAccess);

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

	for (const auto &file : m_FileList)
	{
		fileAttributes = allFileAttributes;

		for (const auto &attribute : m_AttributeList)
		{
			/* If the check box is indeterminate, this attribute will
			stay the same (i.e. if a file had the attribute applied
			initially, it will still have it applied, and vice versa). */
			if (attribute.uChecked == BST_INDETERMINATE)
			{
				if (file.wfd.dwFileAttributes & attribute.Attribute)
				{
					fileAttributes |= attribute.Attribute;
				}
			}
		}

		SetFileAttributes(file.szFullFileName, fileAttributes);

		HANDLE hFile = CreateFile(file.szFullFileName, FILE_WRITE_ATTRIBUTES, 0, nullptr,
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
