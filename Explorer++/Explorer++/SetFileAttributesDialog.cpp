/******************************************************************
 *
 * Project: Explorer++
 * File: SetFileAttributesDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides the ability to change the attributes
 * and creation/modification/access dates of a set
 * of files.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "SetFileAttributesDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"


const TCHAR CSetFileAttributesDialogPersistentSettings::SETTINGS_KEY[] = _T("SetFileAttributes");

CSetFileAttributesDialog::CSetFileAttributesDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t>
	sfaiList) :
CBaseDialog(hInstance,iResource,hParent,false)
{
	assert(sfaiList.size() > 0);

	m_FileList = sfaiList;

	m_psfadps = &CSetFileAttributesDialogPersistentSettings::GetInstance();
}

CSetFileAttributesDialog::~CSetFileAttributesDialog()
{

}

BOOL CSetFileAttributesDialog::OnInitDialog()
{
	InitializeAttributesStructure();
	InitializeDateFields();

	int nItems = static_cast<int>(m_FileList.size());

	int nArchived = 0;
	int nHidden = 0;
	int nSystem = 0;
	int nReadOnly = 0;
	int nIndexed = 0;

	for each(auto File in m_FileList)
	{
		if(File.wfd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			nArchived++;

		if((File.wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN)
			nHidden++;

		if(File.wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
			nSystem++;

		if(File.wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			nReadOnly++;

		if(!(File.wfd.dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
			nIndexed++;
	}

	ResetButtonState(GetDlgItem(m_hDlg,IDC_CHECK_ARCHIVE),nArchived == 0 || nArchived == nItems);
	ResetButtonState(GetDlgItem(m_hDlg,IDC_CHECK_HIDDEN),nHidden == 0 || nHidden == nItems);
	ResetButtonState(GetDlgItem(m_hDlg,IDC_CHECK_SYSTEM),nSystem == 0 || nSystem == nItems);
	ResetButtonState(GetDlgItem(m_hDlg,IDC_CHECK_READONLY),nReadOnly == 0 || nReadOnly == nItems);
	ResetButtonState(GetDlgItem(m_hDlg,IDC_CHECK_INDEXED),nIndexed == 0 || nIndexed == nItems);

	SetAttributeCheckState(GetDlgItem(m_hDlg,IDC_CHECK_ARCHIVE),nArchived,nItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg,IDC_CHECK_HIDDEN),nHidden,nItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg,IDC_CHECK_SYSTEM),nSystem,nItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg,IDC_CHECK_READONLY),nReadOnly,nItems);
	SetAttributeCheckState(GetDlgItem(m_hDlg,IDC_CHECK_INDEXED),nIndexed,nItems);

	m_bModificationDateEnabled = FALSE;
	m_bCreationDateEnabled = FALSE;
	m_bAccessDateEnabled = FALSE;

	m_psfadps->RestoreDialogPosition(m_hDlg,false);

	return 0;
}

void CSetFileAttributesDialog::InitializeDateFields()
{
	WIN32_FIND_DATA *pwfd = &(m_FileList.begin()->wfd);

	/* Use the dates of the first file... */
	FileTimeToLocalSystemTime(&pwfd->ftLastWriteTime,&m_LocalWrite);
	FileTimeToLocalSystemTime(&pwfd->ftCreationTime,&m_LocalCreation);
	FileTimeToLocalSystemTime(&pwfd->ftLastAccessTime,&m_LocalCreation);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_MODIFICATIONDATE),GDT_VALID,&m_LocalWrite);
	DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_MODIFICATIONTIME),GDT_VALID,&m_LocalWrite);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_CREATIONDATE),GDT_VALID,&m_LocalCreation);
	DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_CREATIONTIME),GDT_VALID,&m_LocalCreation);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_ACCESSDATE),GDT_VALID,&m_LocalCreation);
	DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_ACCESSTIME),GDT_VALID,&m_LocalCreation);

	/* All date/time fields are disabled initially. */
	DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_MODIFICATIONDATE),GDT_NONE,NULL);
	EnableWindow(GetDlgItem(m_hDlg,IDC_MODIFICATIONTIME),FALSE);
	EnableWindow(GetDlgItem(m_hDlg,IDC_MODIFICATION_RESET),FALSE);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_CREATIONDATE),GDT_NONE,NULL);
	EnableWindow(GetDlgItem(m_hDlg,IDC_CREATIONTIME),FALSE);
	EnableWindow(GetDlgItem(m_hDlg,IDC_CREATION_RESET),FALSE);

	DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_ACCESSDATE),GDT_NONE,NULL);
	EnableWindow(GetDlgItem(m_hDlg,IDC_ACCESSTIME),FALSE);
	EnableWindow(GetDlgItem(m_hDlg,IDC_ACCESS_RESET),FALSE);
}

void CSetFileAttributesDialog::InitializeAttributesStructure(void)
{
	Attribute_t Attribute;

	Attribute.Attribute		= FILE_ATTRIBUTE_ARCHIVE;
	Attribute.uControlId	= IDC_CHECK_ARCHIVE;
	Attribute.bReversed		= FALSE;
	m_AttributeList.push_back(Attribute);

	Attribute.Attribute		= FILE_ATTRIBUTE_HIDDEN;
	Attribute.uControlId	= IDC_CHECK_HIDDEN;
	Attribute.bReversed		= FALSE;
	m_AttributeList.push_back(Attribute);

	Attribute.Attribute		= FILE_ATTRIBUTE_SYSTEM;
	Attribute.uControlId	= IDC_CHECK_SYSTEM;
	Attribute.bReversed		= FALSE;
	m_AttributeList.push_back(Attribute);

	Attribute.Attribute		= FILE_ATTRIBUTE_READONLY;
	Attribute.uControlId	= IDC_CHECK_READONLY;
	Attribute.bReversed		= FALSE;
	m_AttributeList.push_back(Attribute);

	Attribute.Attribute		= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
	Attribute.uControlId	= IDC_CHECK_INDEXED;
	Attribute.bReversed		= TRUE;
	m_AttributeList.push_back(Attribute);
}

BOOL CSetFileAttributesDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDC_MODIFICATION_RESET:
		OnDateReset(DATE_TIME_MODIFIED);
		break;

	case IDC_CREATION_RESET:
		OnDateReset(DATE_TIME_CREATED);
		break;

	case IDC_ACCESS_RESET:
		OnDateReset(DATE_TIME_ACCESSED);
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

BOOL CSetFileAttributesDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case DTN_DATETIMECHANGE:
		{
			NMDATETIMECHANGE *pdtc = reinterpret_cast<NMDATETIMECHANGE *>(pnmhdr);

			switch(pnmhdr->idFrom)
			{
			case IDC_MODIFICATIONDATE:
				m_bModificationDateEnabled = (pdtc->dwFlags == GDT_VALID);
				EnableWindow(GetDlgItem(m_hDlg,IDC_MODIFICATIONTIME),pdtc->dwFlags == GDT_VALID);
				EnableWindow(GetDlgItem(m_hDlg,IDC_MODIFICATION_RESET),pdtc->dwFlags == GDT_VALID);
				break;

			case IDC_CREATIONDATE:
				m_bCreationDateEnabled = (pdtc->dwFlags == GDT_VALID);
				EnableWindow(GetDlgItem(m_hDlg,IDC_CREATIONTIME),pdtc->dwFlags == GDT_VALID);
				EnableWindow(GetDlgItem(m_hDlg,IDC_CREATION_RESET),pdtc->dwFlags == GDT_VALID);
				break;

			case IDC_ACCESSDATE:
				m_bAccessDateEnabled = (pdtc->dwFlags == GDT_VALID);
				EnableWindow(GetDlgItem(m_hDlg,IDC_ACCESSTIME),pdtc->dwFlags == GDT_VALID);
				EnableWindow(GetDlgItem(m_hDlg,IDC_ACCESS_RESET),pdtc->dwFlags == GDT_VALID);
				break;
			}
		}
		break;
	}

	return 0;
}

BOOL CSetFileAttributesDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CSetFileAttributesDialog::OnOk()
{
	FILETIME *plw = NULL;
	FILETIME *plc = NULL;
	FILETIME *pla = NULL;
	DWORD AllFileAttributes = FILE_ATTRIBUTE_NORMAL;
	DWORD FileAttributes;

	if(m_bModificationDateEnabled)
	{
		FILETIME LastWriteTime;
		SYSTEMTIME LocalWrite;
		SYSTEMTIME LocalWriteDate;
		SYSTEMTIME LocalWriteTime;

		DateTime_GetSystemtime(GetDlgItem(m_hDlg,IDC_MODIFICATIONDATE),&LocalWriteDate);
		DateTime_GetSystemtime(GetDlgItem(m_hDlg,IDC_MODIFICATIONTIME),&LocalWriteTime);

		MergeDateTime(&LocalWrite,&LocalWriteDate,&LocalWriteTime);

		LocalSystemTimeToFileTime(&LocalWrite,&LastWriteTime);
		plw = &LastWriteTime;
	}

	if(m_bCreationDateEnabled)
	{
		FILETIME CreationTime;
		SYSTEMTIME LocalCreation;
		SYSTEMTIME LocalCreationDate;
		SYSTEMTIME LocalCreationTime;

		DateTime_GetSystemtime(GetDlgItem(m_hDlg,IDC_CREATIONDATE),&LocalCreationDate);
		DateTime_GetSystemtime(GetDlgItem(m_hDlg,IDC_CREATIONTIME),&LocalCreationTime);

		MergeDateTime(&LocalCreation,&LocalCreationDate,&LocalCreationTime);

		LocalSystemTimeToFileTime(&LocalCreation,&CreationTime);
		plc = &CreationTime;
	}

	if(m_bAccessDateEnabled)
	{
		FILETIME AccessTime;
		SYSTEMTIME LocalAccess;
		SYSTEMTIME LocalAccessDate;
		SYSTEMTIME LocalAccessTime;

		DateTime_GetSystemtime(GetDlgItem(m_hDlg,IDC_ACCESSDATE),&LocalAccessDate);
		DateTime_GetSystemtime(GetDlgItem(m_hDlg,IDC_ACCESSTIME),&LocalAccessTime);

		MergeDateTime(&LocalAccess,&LocalAccessDate,&LocalAccessTime);

		LocalSystemTimeToFileTime(&LocalAccess,&AccessTime);
		pla = &AccessTime;
	}

	/* Build up list of attributes. Add all positive
	attributes (i.e. those that are active for all files).
	Any attributes which are inderterminate will not change
	(note that they are per-file). */
	for each(auto Attribute in m_AttributeList)
	{
		Attribute.uChecked = static_cast<UINT>(SendMessage(GetDlgItem(m_hDlg,
			Attribute.uControlId),BM_GETCHECK,0,0));

		if((!Attribute.bReversed && Attribute.uChecked == BST_CHECKED) ||
			(Attribute.bReversed && Attribute.uChecked != BST_CHECKED))
		{
			AllFileAttributes |= Attribute.Attribute;
		}
	}

	for each(auto File in m_FileList)
	{
		FileAttributes = AllFileAttributes;

		for each(auto Attribute in m_AttributeList)
		{
			/* If the check box is indeterminate, this attribute will
			stay the same (i.e. if a file had the attribute applied
			initially, it will still have it applied, and vice versa). */
			if(Attribute.uChecked == BST_INDETERMINATE)
			{
				if((!Attribute.bReversed) &&
					(File.wfd.dwFileAttributes & Attribute.Attribute))
				{
					FileAttributes |= Attribute.Attribute;
				}
			}
		}

		SetFileAttributes(File.szFullFileName,FileAttributes);

		HANDLE hFile = CreateFile(File.szFullFileName,FILE_WRITE_ATTRIBUTES,0,
			NULL,OPEN_EXISTING,NULL,NULL);

		if(hFile != INVALID_HANDLE_VALUE)
		{
			SetFileTime(hFile,plc,pla,plw);
			CloseHandle(hFile);
		}
	}

	EndDialog(m_hDlg,1);
}

void CSetFileAttributesDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CSetFileAttributesDialog::OnDateReset(DateTimeType_t DateTimeType)
{
	switch(DateTimeType)
	{
	case DATE_TIME_MODIFIED:
		DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_MODIFICATIONDATE),GDT_VALID,&m_LocalWrite);
		DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_MODIFICATIONTIME),GDT_VALID,&m_LocalWrite);
		break;

	case DATE_TIME_CREATED:
		DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_CREATIONDATE),GDT_VALID,&m_LocalCreation);
		DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_CREATIONTIME),GDT_VALID,&m_LocalCreation);
		break;

	case DATE_TIME_ACCESSED:
		DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_ACCESSDATE),GDT_VALID,&m_LocalCreation);
		DateTime_SetSystemtime(GetDlgItem(m_hDlg,IDC_ACCESSTIME),GDT_VALID,&m_LocalCreation);
		break;
	}
}

void CSetFileAttributesDialog::SaveState()
{
	m_psfadps->SaveDialogPosition(m_hDlg);

	m_psfadps->m_bStateSaved = TRUE;
}

void CSetFileAttributesDialog::SetAttributeCheckState(HWND hwnd,
	int nAttributes,int nSelected)
{
	UINT CheckState;

	if(nAttributes == 0)
		CheckState = BST_UNCHECKED;
	else if(nAttributes == nSelected)
		CheckState = BST_CHECKED;
	else
		CheckState = BST_INDETERMINATE;

	SendMessage(hwnd,BM_SETCHECK,CheckState,0);
}

void CSetFileAttributesDialog::ResetButtonState(HWND hwnd,BOOL bReset)
{
	if(!bReset)
		return;

	SendMessage(hwnd,BM_SETSTYLE,BS_AUTOCHECKBOX,MAKELPARAM(FALSE,0));
}

CSetFileAttributesDialogPersistentSettings::CSetFileAttributesDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CSetFileAttributesDialogPersistentSettings::~CSetFileAttributesDialogPersistentSettings()
{
	
}

CSetFileAttributesDialogPersistentSettings& CSetFileAttributesDialogPersistentSettings::GetInstance()
{
	static CSetFileAttributesDialogPersistentSettings sfadps;
	return sfadps;
}