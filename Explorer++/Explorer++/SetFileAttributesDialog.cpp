/******************************************************************
 *
 * Project: Explorer++
 * File: SetFileAttributesDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides the ability to set various attributes
 * of a file.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Misc.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"

#define NUM_ATTRIBUTES		5

#define DATE_TIME_MODIFIED	0
#define DATE_TIME_CREATED	1
#define DATE_TIME_ACCESSED	2

void SetAttributeCheckState(HWND hwnd,int nAttribute,int nSelected);
void ResetButtonState(HWND hwnd,BOOL bReset);

typedef struct
{
	DWORD Attribute;
	UINT uControlId;
	UINT uChecked;

	/* Set if the attribute state is on
	when the particular attribute is off. */
	BOOL bReversed;
} Attributes_t;

Attributes_t Attributes[6];

SYSTEMTIME g_LocalWrite;
SYSTEMTIME g_LocalCreation;
SYSTEMTIME g_LocalAccess;

INT_PTR CALLBACK SetFileAttributesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	if(pContainer != NULL)
	{
		return pContainer->SetFileAttributesProc(hDlg,uMsg,wParam,lParam);
	}
	else
	{
		return 0;
	}
}

INT_PTR CALLBACK CContainer::SetFileAttributesProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static HANDLE hFile;
	static TCHAR FileName[MAX_PATH];

	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnInitializeFileAttributesDlg(hDlg,lParam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_MODIFICATION_RESET:
					OnDateReset(hDlg,DATE_TIME_MODIFIED);
					break;

				case IDC_CREATION_RESET:
					OnDateReset(hDlg,DATE_TIME_CREATED);
					break;

				case IDC_ACCESS_RESET:
					OnDateReset(hDlg,DATE_TIME_ACCESSED);
					break;

				case IDOK:
					OnFileAttributesDlgOk(hDlg);
					break;

				case IDCANCEL:
					SetFileAttributesSaveState(hDlg);
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_NOTIFY:
			OnSetFileAttributesNotify(hDlg,wParam,lParam);
			break;

		case WM_CLOSE:
			SetFileAttributesSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnInitializeFileAttributesDlg(HWND hDlg,LPARAM lParam)
{
	list<SetFileAttributesInfo_t>::iterator itr;
	int nItems;
	int nArchive = 0;
	int nHidden = 0;
	int nSystem = 0;
	int nReadOnly = 0;
	int nIndexed = 0;

	InitializeAttributesStructure();

	InitializeDateFields(hDlg);

	nItems = (int)m_sfaiList.size();

	for(itr = m_sfaiList.begin();itr != m_sfaiList.end();itr++)
	{
		if(itr->wfd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			nArchive++;

		if((itr->wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN)
			nHidden++;

		if(itr->wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
			nSystem++;

		if(itr->wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			nReadOnly++;

		if(!(itr->wfd.dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
			nIndexed++;
	}

	ResetButtonState(GetDlgItem(hDlg,IDC_CHECK_ARCHIVE),nArchive == 0 || nArchive == nItems);
	ResetButtonState(GetDlgItem(hDlg,IDC_CHECK_HIDDEN),nHidden == 0 || nHidden == nItems);
	ResetButtonState(GetDlgItem(hDlg,IDC_CHECK_SYSTEM),nSystem == 0 || nSystem == nItems);
	ResetButtonState(GetDlgItem(hDlg,IDC_CHECK_READONLY),nReadOnly == 0 || nReadOnly == nItems);
	ResetButtonState(GetDlgItem(hDlg,IDC_CHECK_INDEXED),nIndexed == 0 || nIndexed == nItems);

	SetAttributeCheckState(GetDlgItem(hDlg,IDC_CHECK_ARCHIVE),nArchive,nItems);
	SetAttributeCheckState(GetDlgItem(hDlg,IDC_CHECK_HIDDEN),nHidden,nItems);
	SetAttributeCheckState(GetDlgItem(hDlg,IDC_CHECK_SYSTEM),nSystem,nItems);
	SetAttributeCheckState(GetDlgItem(hDlg,IDC_CHECK_READONLY),nReadOnly,nItems);
	SetAttributeCheckState(GetDlgItem(hDlg,IDC_CHECK_INDEXED),nIndexed,nItems);

	m_bModificationDateEnabled = FALSE;
	m_bCreationDateEnabled = FALSE;
	m_bAccessDateEnabled = FALSE;

	if(m_bSetFileAttributesDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptWildcardSelect.x,
			m_ptWildcardSelect.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}
}

void SetAttributeCheckState(HWND hwnd,int nAttribute,int nSelected)
{
	UINT CheckState;

	if(nAttribute == 0)
		CheckState = BST_UNCHECKED;
	else if(nAttribute == nSelected)
		CheckState = BST_CHECKED;
	else
		CheckState = BST_INDETERMINATE;

	SendMessage(hwnd,BM_SETCHECK,(WPARAM)CheckState,(LPARAM)0);
}

void ResetButtonState(HWND hwnd,BOOL bReset)
{
	if(!bReset)
		return;

	SendMessage(hwnd,BM_SETSTYLE,(WPARAM)BS_AUTOCHECKBOX,MAKELPARAM(FALSE,0));
}

void CContainer::OnFileAttributesDlgOk(HWND hDlg)
{
	HANDLE hFile;
	list<SetFileAttributesInfo_t>::iterator itr;
	FILETIME CreationTime;
	FILETIME AccessTime;
	FILETIME LastWriteTime;
	SYSTEMTIME LocalWrite;
	SYSTEMTIME LocalCreation;
	SYSTEMTIME LocalAccess;
	FILETIME *plw = NULL;
	FILETIME *plc = NULL;
	FILETIME *pla = NULL;
	DWORD AllFileAttributes = FILE_ATTRIBUTE_NORMAL;
	DWORD FileAttributes;
	int i = 0;

	if(m_bModificationDateEnabled)
	{
		SYSTEMTIME LocalWriteDate;
		SYSTEMTIME LocalWriteTime;

		DateTime_GetSystemtime(GetDlgItem(hDlg,IDC_MODIFICATIONDATE),&LocalWriteDate);
		DateTime_GetSystemtime(GetDlgItem(hDlg,IDC_MODIFICATIONTIME),&LocalWriteTime);

		MergeDateTime(&LocalWrite,&LocalWriteDate,&LocalWriteTime);

		LocalSystemTimeToFileTime(&LocalWrite,&LastWriteTime);
		plw = &LastWriteTime;
	}

	if(m_bCreationDateEnabled)
	{
		SYSTEMTIME LocalCreationDate;
		SYSTEMTIME LocalCreationTime;

		DateTime_GetSystemtime(GetDlgItem(hDlg,IDC_CREATIONDATE),&LocalCreationDate);
		DateTime_GetSystemtime(GetDlgItem(hDlg,IDC_CREATIONTIME),&LocalCreationTime);

		MergeDateTime(&LocalCreation,&LocalCreationDate,&LocalCreationTime);

		LocalSystemTimeToFileTime(&LocalCreation,&CreationTime);
		plc = &CreationTime;
	}

	if(m_bAccessDateEnabled)
	{
		SYSTEMTIME LocalAccessDate;
		SYSTEMTIME LocalAccessTime;

		DateTime_GetSystemtime(GetDlgItem(hDlg,IDC_ACCESSDATE),&LocalAccessDate);
		DateTime_GetSystemtime(GetDlgItem(hDlg,IDC_ACCESSTIME),&LocalAccessTime);

		MergeDateTime(&LocalAccess,&LocalAccessDate,&LocalAccessTime);

		LocalSystemTimeToFileTime(&LocalAccess,&AccessTime);
		pla = &AccessTime;
	}

	for(i = 0;i < NUM_ATTRIBUTES;i++)
	{
		Attributes[i].uChecked = (UINT)SendMessage(GetDlgItem(hDlg,Attributes[i].uControlId),
		BM_GETCHECK,0,0);
	}

	for(i = 0;i < NUM_ATTRIBUTES;i++)
	{
		if(!Attributes[i].bReversed)
		{
			if(Attributes[i].uChecked == BST_CHECKED)
			{
				AllFileAttributes |= Attributes[i].Attribute;
			}
		}
		else
		{
			if(Attributes[i].uChecked != BST_CHECKED)
			{
				AllFileAttributes |= Attributes[i].Attribute;
			}
		}
	}

	for(itr = m_sfaiList.begin();itr != m_sfaiList.end();itr++)
	{
		FileAttributes = AllFileAttributes;

		for(i = 0;i < NUM_ATTRIBUTES;i++)
		{
			/* If the check box is indeterminate, this attribute will
			stay the same (i.e. if a file had it applied initially, it
			will still have it applied, and vice versa). */
			if(Attributes[i].uChecked == BST_INDETERMINATE)
			{
				if((!Attributes[i].bReversed) &&
					(itr->wfd.dwFileAttributes & Attributes[i].Attribute))
					FileAttributes |= Attributes[i].Attribute;
			}
		}

		SetFileAttributes(itr->szFullFileName,FileAttributes);

		hFile = CreateFile(itr->szFullFileName,FILE_WRITE_ATTRIBUTES,0,NULL,OPEN_EXISTING,
			NULL,NULL);

		if(hFile != INVALID_HANDLE_VALUE)
		{
			SetFileTime(hFile,plc,pla,plw);

			CloseHandle(hFile);
		}
	}

	SetFileAttributesSaveState(hDlg);

	EndDialog(hDlg,1);
}

void CContainer::OnSetFileAttributesNotify(HWND hDlg,WPARAM wParam,LPARAM lParam)
{
	NMHDR *nmhdr;

	nmhdr = (NMHDR *)lParam;

	switch(nmhdr->code)
	{
		case DTN_DATETIMECHANGE:
			{
				NMDATETIMECHANGE *pdtc;

				pdtc = (NMDATETIMECHANGE *)lParam;

				switch(nmhdr->idFrom)
				{
					case IDC_MODIFICATIONDATE:
						m_bModificationDateEnabled = (pdtc->dwFlags == GDT_VALID);
						EnableWindow(GetDlgItem(hDlg,IDC_MODIFICATIONTIME),pdtc->dwFlags == GDT_VALID);
						EnableWindow(GetDlgItem(hDlg,IDC_MODIFICATION_RESET),pdtc->dwFlags == GDT_VALID);
						break;

					case IDC_CREATIONDATE:
						m_bCreationDateEnabled = (pdtc->dwFlags == GDT_VALID);
						EnableWindow(GetDlgItem(hDlg,IDC_CREATIONTIME),pdtc->dwFlags == GDT_VALID);
						EnableWindow(GetDlgItem(hDlg,IDC_CREATION_RESET),pdtc->dwFlags == GDT_VALID);
						break;

					case IDC_ACCESSDATE:
						m_bAccessDateEnabled = (pdtc->dwFlags == GDT_VALID);
						EnableWindow(GetDlgItem(hDlg,IDC_ACCESSTIME),pdtc->dwFlags == GDT_VALID);
						EnableWindow(GetDlgItem(hDlg,IDC_ACCESS_RESET),pdtc->dwFlags == GDT_VALID);
						break;
				}
			}
			break;
	}
}

void CContainer::OnDateReset(HWND hDlg,int FieldToReset)
{
	switch(FieldToReset)
	{
		case DATE_TIME_MODIFIED:
			DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_MODIFICATIONDATE),GDT_VALID,&g_LocalWrite);
			DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_MODIFICATIONTIME),GDT_VALID,&g_LocalWrite);
			break;

		case DATE_TIME_CREATED:
			DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_CREATIONDATE),GDT_VALID,&g_LocalCreation);
			DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_CREATIONTIME),GDT_VALID,&g_LocalCreation);
			break;

		case DATE_TIME_ACCESSED:
			DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_ACCESSDATE),GDT_VALID,&g_LocalAccess);
			DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_ACCESSTIME),GDT_VALID,&g_LocalAccess);
			break;
	}
}

void CContainer::InitializeAttributesStructure(void)
{
	Attributes[0].Attribute		= FILE_ATTRIBUTE_ARCHIVE;
	Attributes[0].uControlId	= IDC_CHECK_ARCHIVE;
	Attributes[0].bReversed		= FALSE;

	Attributes[1].Attribute		= FILE_ATTRIBUTE_HIDDEN;
	Attributes[1].uControlId	= IDC_CHECK_HIDDEN;
	Attributes[1].bReversed		= FALSE;

	Attributes[2].Attribute		= FILE_ATTRIBUTE_SYSTEM;
	Attributes[2].uControlId	= IDC_CHECK_SYSTEM;
	Attributes[2].bReversed		= FALSE;

	Attributes[3].Attribute		= FILE_ATTRIBUTE_READONLY;
	Attributes[3].uControlId	= IDC_CHECK_READONLY;
	Attributes[3].bReversed		= FALSE;

	Attributes[4].Attribute		= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
	Attributes[4].uControlId	= IDC_CHECK_INDEXED;
	Attributes[4].bReversed		= TRUE;
}

void CContainer::InitializeDateFields(HWND hDlg)
{
	/* Use the dates of the first file... */
	FileTimeToLocalSystemTime(&(m_sfaiList.begin())->wfd.ftLastWriteTime,&g_LocalWrite);
	FileTimeToLocalSystemTime(&(m_sfaiList.begin())->wfd.ftCreationTime,&g_LocalCreation);
	FileTimeToLocalSystemTime(&(m_sfaiList.begin())->wfd.ftLastAccessTime,&g_LocalAccess);

	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_MODIFICATIONDATE),GDT_VALID,&g_LocalWrite);
	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_MODIFICATIONTIME),GDT_VALID,&g_LocalWrite);

	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_CREATIONDATE),GDT_VALID,&g_LocalCreation);
	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_CREATIONTIME),GDT_VALID,&g_LocalCreation);

	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_ACCESSDATE),GDT_VALID,&g_LocalAccess);
	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_ACCESSTIME),GDT_VALID,&g_LocalAccess);

	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_MODIFICATIONDATE),GDT_NONE,NULL);
	EnableWindow(GetDlgItem(hDlg,IDC_MODIFICATIONTIME),FALSE);
	EnableWindow(GetDlgItem(hDlg,IDC_MODIFICATION_RESET),FALSE);

	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_CREATIONDATE),GDT_NONE,NULL);
	EnableWindow(GetDlgItem(hDlg,IDC_CREATIONTIME),FALSE);
	EnableWindow(GetDlgItem(hDlg,IDC_CREATION_RESET),FALSE);

	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_ACCESSDATE),GDT_NONE,NULL);
	EnableWindow(GetDlgItem(hDlg,IDC_ACCESSTIME),FALSE);
	EnableWindow(GetDlgItem(hDlg,IDC_ACCESS_RESET),FALSE);
}

void CContainer::SetFileAttributesSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptSetFileAttributes.x = rcTemp.left;
	m_ptSetFileAttributes.y = rcTemp.top;

	m_bSetFileAttributesDlgStateSaved = TRUE;
}