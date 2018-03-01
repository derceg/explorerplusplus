/******************************************************************
 *
 * Project: ShellBrowser
 * File: GroupManager.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Includes code for placing items into
 * groups.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include <cassert>
#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"


namespace
{
	static const UINT KBYTE = 1024;
	static const UINT MBYTE = 1024 * 1024;
	static const UINT GBYTE = 1024 * 1024 *1024;
}

#define GROUP_BY_DATECREATED	0
#define GROUP_BY_DATEMODIFIED	1
#define GROUP_BY_DATEACCESSED	2

#define GROUP_BY_SIZE			0
#define GROUP_BY_TOTALSIZE		1

#define GROUP_OTHER				27

TCHAR *RetrieveGroupHeader(int iGroupId);

/* Group sorting. */
INT CALLBACK	NameComparison(INT Group1_ID, INT Group2_ID, void *pvData);
INT CALLBACK	FreeSpaceComparison(INT Group1_ID, INT Group2_ID, void *pvData);

BOOL g_bSortAscending;

std::list<TypeGroup_t> *g_pGroupList = NULL;

/* Simply sets the grouping flag, without actually moving
items into groups. */
void CShellBrowser::SetGroupingFlag(BOOL bShowInGroups)
{
	m_bShowInGroups = bShowInGroups;
}

void CShellBrowser::SetGrouping(BOOL bShowInGroups)
{
	m_bShowInGroups = bShowInGroups;

	if(!m_bShowInGroups)
	{
		ListView_EnableGroupView(m_hListView,FALSE);
		SortFolder(m_SortMode);
		return;
	}
	else
	{
		MoveItemsIntoGroups();
	}
}

void CShellBrowser::ToggleGrouping(void)
{
	m_bShowInGroups = !m_bShowInGroups;

	if(!m_bShowInGroups)
	{
		ListView_EnableGroupView(m_hListView,FALSE);
		SortFolder(m_SortMode);
		return;
	}
	else
	{
		MoveItemsIntoGroups();
	}
}

INT CALLBACK NameComparison(INT Group1_ID, INT Group2_ID, void *pvData)
{
	UNREFERENCED_PARAMETER(pvData);

	TCHAR *pszGroupHeader1 = NULL;
	TCHAR *pszGroupHeader2 = NULL;
	int iReturnValue;

	pszGroupHeader1 = RetrieveGroupHeader(Group1_ID);
	pszGroupHeader2 = RetrieveGroupHeader(Group2_ID);

	if(lstrcmpi(pszGroupHeader1,_T("Other")) == 0 &&
		lstrcmpi(pszGroupHeader2,_T("Other")) != 0)
		iReturnValue = 1;
	else if(lstrcmpi(pszGroupHeader1,_T("Other")) != 0 &&
		lstrcmpi(pszGroupHeader2,_T("Other")) == 0)
		iReturnValue = -1;
	else if(lstrcmpi(pszGroupHeader1,_T("Other")) == 0 &&
		lstrcmpi(pszGroupHeader2,_T("Other")) == 0)
		iReturnValue = 0;
	else
		iReturnValue = StrCmpLogicalW(pszGroupHeader1,pszGroupHeader2);

	if(!g_bSortAscending)
		iReturnValue = -iReturnValue;

	return iReturnValue;
}

INT CALLBACK FreeSpaceComparison(INT Group1_ID, INT Group2_ID, void *pvData)
{
	UNREFERENCED_PARAMETER(pvData);

	TCHAR *pszGroupHeader1 = NULL;
	TCHAR *pszGroupHeader2 = NULL;
	int iReturnValue;

	pszGroupHeader1 = RetrieveGroupHeader(Group1_ID);
	pszGroupHeader2 = RetrieveGroupHeader(Group2_ID);

	if(lstrcmpi(pszGroupHeader1,_T("Unspecified")) == 0 &&
		lstrcmpi(pszGroupHeader2,_T("Unspecified")) != 0)
		iReturnValue = 1;
	else if(lstrcmpi(pszGroupHeader1,_T("Unspecified")) != 0 &&
		lstrcmpi(pszGroupHeader2,_T("Unspecified")) == 0)
		iReturnValue = -1;
	else if(lstrcmpi(pszGroupHeader1,_T("Unspecified")) == 0 &&
		lstrcmpi(pszGroupHeader2,_T("Unspecified")) == 0)
		iReturnValue = 0;
	else
		iReturnValue = StrCmpLogicalW(pszGroupHeader1,pszGroupHeader2);

	if(!g_bSortAscending)
		iReturnValue = -iReturnValue;

	return iReturnValue;
}

TCHAR *RetrieveGroupHeader(int iGroupId)
{
	std::list<TypeGroup_t>::iterator	itr;

	for(itr = g_pGroupList->begin();itr != g_pGroupList->end();itr++)
	{
		if(itr->iGroupId == iGroupId)
		{
			return itr->szHeader;
		}
	}

	return NULL;
}

/*
 * Determines the id of the group the specified
 * item belongs to.
 */
int CShellBrowser::DetermineItemGroup(int iItemInternal)
{
	PFNLVGROUPCOMPARE	pfnGroupCompare = NULL;
	TCHAR				szGroupHeader[512];
	int					iGroupId;

	switch(m_SortMode)
	{
		case FSM_NAME:
			DetermineItemNameGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_TYPE:
			DetermineItemTypeGroupVirtual(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_SIZE:
			DetermineItemSizeGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_DATEMODIFIED:
			DetermineItemDateGroup(iItemInternal,GROUP_BY_DATEMODIFIED,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_TOTALSIZE:
			DetermineItemTotalSizeGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_FREESPACE:
			DetermineItemFreeSpaceGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = FreeSpaceComparison;
			break;

		case FSM_DATEDELETED:
			break;

		case FSM_ORIGINALLOCATION:
			break;

		case FSM_ATTRIBUTES:
			DetermineItemAttributeGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_SHORTNAME:
			DetermineItemNameGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_OWNER:
			DetermineItemOwnerGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_PRODUCTNAME:
			DetermineItemVersionGroup(iItemInternal,_T("ProductName"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_COMPANY:
			DetermineItemVersionGroup(iItemInternal,_T("CompanyName"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_DESCRIPTION:
			DetermineItemVersionGroup(iItemInternal,_T("FileDescription"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_FILEVERSION:
			DetermineItemVersionGroup(iItemInternal,_T("FileVersion"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_PRODUCTVERSION:
			DetermineItemVersionGroup(iItemInternal,_T("ProductVersion"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_SHORTCUTTO:
			break;

		case FSM_HARDLINKS:
			break;

		case FSM_EXTENSION:
			DetermineItemExtensionGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_CREATED:
			DetermineItemDateGroup(iItemInternal,GROUP_BY_DATECREATED,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_ACCESSED:
			DetermineItemDateGroup(iItemInternal,GROUP_BY_DATEACCESSED,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_TITLE:
			DetermineItemSummaryGroup(iItemInternal,&SCID_TITLE,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_SUBJECT:
			DetermineItemSummaryGroup(iItemInternal,&SCID_SUBJECT,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_AUTHOR:
			DetermineItemSummaryGroup(iItemInternal,&SCID_AUTHOR,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_KEYWORDS:
			DetermineItemSummaryGroup(iItemInternal,&SCID_KEYWORDS,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_COMMENTS:
			DetermineItemSummaryGroup(iItemInternal,&SCID_COMMENTS,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;


		case FSM_CAMERAMODEL:
			DetermineItemCameraPropertyGroup(iItemInternal,PropertyTagEquipModel,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_DATETAKEN:
			DetermineItemCameraPropertyGroup(iItemInternal,PropertyTagDateTime,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_WIDTH:
			DetermineItemCameraPropertyGroup(iItemInternal,PropertyTagImageWidth,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_HEIGHT:
			DetermineItemCameraPropertyGroup(iItemInternal,PropertyTagImageHeight,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;


		case FSM_VIRTUALCOMMENTS:
			break;

		case FSM_FILESYSTEM:
			DetermineItemFileSystemGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		case FSM_NUMPRINTERDOCUMENTS:
			break;

		case FSM_PRINTERSTATUS:
			break;

		case FSM_PRINTERCOMMENTS:
			break;

		case FSM_PRINTERLOCATION:
			break;

		case FSM_NETWORKADAPTER_STATUS:
			DetermineItemNetworkStatus(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = NameComparison;
			break;

		default:
			assert(false);
			break;
	}

	iGroupId = CheckGroup(szGroupHeader,pfnGroupCompare);

	return iGroupId;
}

/*
 * Checks if a group with specified id is already
 * in the listview. If not, the group is inserted
 * into its sorted position with the specified
 * header text.
 */
int CShellBrowser::CheckGroup(const TCHAR *szGroupHeader,
PFNLVGROUPCOMPARE pfnGroupCompare)
{
	LVINSERTGROUPSORTED			lvigs;
	std::list<TypeGroup_t>::iterator	itr;
	TypeGroup_t					TypeGroup;
	WCHAR						wszHeader[512];
	BOOL						bFoundGroup = FALSE;
	int							iGroupId = -1;

	/* Check to see if the group has already been inserted... */
	for(itr = m_GroupList.begin();itr != m_GroupList.end();itr++)
	{
		if(lstrcmpi(szGroupHeader,itr->szHeader) == 0)
		{
			LVGROUP lvGroup;

			bFoundGroup = TRUE;
			iGroupId = itr->iGroupId;
			itr->nItems++;

			StringCchPrintf(wszHeader, SIZEOF_ARRAY(wszHeader),
				_T("%s (%d)"), szGroupHeader, itr->nItems);

			lvGroup.cbSize = sizeof(LVGROUP);
			lvGroup.mask = LVGF_HEADER;
			lvGroup.pszHeader = wszHeader;

			ListView_SetGroupInfo(m_hListView, iGroupId, &lvGroup);

			break;
		}
	}

	if(!bFoundGroup)
	{
		iGroupId = m_iGroupId++;

		StringCchCopy(TypeGroup.szHeader,SIZEOF_ARRAY(TypeGroup.szHeader),
			szGroupHeader);
		TypeGroup.iGroupId = iGroupId;
		TypeGroup.nItems = 1;
		m_GroupList.push_back(TypeGroup);

		StringCchPrintf(wszHeader, SIZEOF_ARRAY(wszHeader),
			_T("%s (%d)"), szGroupHeader, TypeGroup.nItems);

		/* The group is not in the listview, so insert it in. */
		lvigs.lvGroup.cbSize	= sizeof(LVGROUP);
		lvigs.lvGroup.mask		= LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
		lvigs.lvGroup.state		= LVGS_COLLAPSIBLE;
		lvigs.lvGroup.pszHeader	= wszHeader;
		lvigs.lvGroup.iGroupId	= iGroupId;
		lvigs.lvGroup.stateMask	= 0;
		lvigs.pfnGroupCompare = (PFNLVGROUPCOMPARE)pfnGroupCompare;
		lvigs.pvData = NULL;

		ListView_InsertGroupSorted(m_hListView,&lvigs);
	}

	return iGroupId;
}

/*
 * Determines the id of the group to which the specified
 * item belongs, based on the item's name.
 * Also returns the text header for the group when szGroupHeader
 * is non-NULL.
 */
/* TODO: These groups have changed as of Windows Visa.*/
void CShellBrowser::DetermineItemNameGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR ch;
	std::list<TypeGroup_t>::iterator itr;

	/* Take the first character of the item's name,
	and use it to determine which group it belongs to. */
	ch = m_pExtraItemInfo[iItemInternal].szDisplayName[0];

	if(iswalpha(ch))
	{
		StringCchPrintf(szGroupHeader,cchMax,
			_T("%c"),towupper(ch));
	}
	else
	{
		StringCchCopy(szGroupHeader,cchMax,
			_T("Other"));
	}
}

/*
 * Determines the id of the group to which the specified
 * item belongs, based on the item's size.
 * Also returns the text header for the group when szGroupHeader
 * is non-NULL.
 */
void CShellBrowser::DetermineItemSizeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR *SizeGroups[] = {_T("Folders"),_T("Tiny"),_T("Small"),_T("Medium"),_T("Large"),_T("Huge")};
	int SizeGroupLimits[] = {0,0,32 * KBYTE,100 * KBYTE,MBYTE,10 * MBYTE};
	int nGroups = 6;
	int iSize;
	int i;

	if((m_pwfdFiles[iItemInternal].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	== FILE_ATTRIBUTE_DIRECTORY)
	{
		/* This item is a folder. */
		iSize = 0;
	}
	else
	{
		i = nGroups - 1;

		double FileSize = m_pwfdFiles[iItemInternal].nFileSizeLow +
		(m_pwfdFiles[iItemInternal].nFileSizeHigh * pow(2.0,32.0));

		/* Check which of the size groups this item belongs to. */
		while(FileSize < SizeGroupLimits[i]
		&& i > 0)
			i--;

		iSize = i;
	}

	StringCchCopy(szGroupHeader,cchMax,SizeGroups[iSize]);
}

/*
 * Determines the id of the group to which the specified
 * drive/folder item belongs, based on the item's total size.
 * Also returns the text header for the group when szGroupHeader
 * is non-NULL.
 */
/* TODO: These groups have changed as of Windows Vista. */
void CShellBrowser::DetermineItemTotalSizeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	IShellFolder *pShellFolder	= NULL;
	LPITEMIDLIST pidlComplete	= NULL;
	LPITEMIDLIST pidlDirectory	= NULL;
	LPITEMIDLIST pidlRelative	= NULL;
	TCHAR *SizeGroups[] = {_T("Unspecified"),_T("Small"),_T("Medium"),_T("Huge"),_T("Gigantic")};
	TCHAR szItem[MAX_PATH];
	STRRET str;
	ULARGE_INTEGER nTotalBytes;
	ULARGE_INTEGER nFreeBytes;
	BOOL bRoot;
	BOOL bRes = FALSE;
	ULARGE_INTEGER TotalSizeGroupLimits[6];
	int nGroups = 5;
	int iSize = 0;
	int i;

	TotalSizeGroupLimits[0].QuadPart	= 0;
	TotalSizeGroupLimits[1].QuadPart	= 0;
	TotalSizeGroupLimits[2].QuadPart	= GBYTE;
	TotalSizeGroupLimits[3].QuadPart	= 20 * TotalSizeGroupLimits[2].QuadPart;
	TotalSizeGroupLimits[4].QuadPart	= 100 * TotalSizeGroupLimits[2].QuadPart;

	GetIdlFromParsingName(m_CurDir,&pidlDirectory);

	pidlComplete = ILCombine(pidlDirectory,m_pExtraItemInfo[iItemInternal].pridl);

	SHBindToParent(pidlComplete, IID_PPV_ARGS(&pShellFolder), (LPCITEMIDLIST *) &pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szItem,SIZEOF_ARRAY(szItem));

	bRoot = PathIsRoot(szItem);

	if(bRoot)
	{
		bRes = GetDiskFreeSpaceEx(szItem,NULL,&nTotalBytes,&nFreeBytes);

		CoTaskMemFree(pidlDirectory);
		CoTaskMemFree(pidlComplete);
		pShellFolder->Release();

		i = nGroups - 1;

		while(nTotalBytes.QuadPart < TotalSizeGroupLimits[i].QuadPart && i > 0)
			i--;

		iSize = i;
	}

	if(!bRoot || !bRes)
	{
		iSize = 0;
	}

	StringCchCopy(szGroupHeader,cchMax,SizeGroups[iSize]);
}

void CShellBrowser::DetermineItemTypeGroupVirtual(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	LPITEMIDLIST				pidlComplete = NULL;
	LPITEMIDLIST				pidlDirectory = NULL;
	SHFILEINFO					shfi;
	std::list<TypeGroup_t>::iterator	itr;

	GetIdlFromParsingName(m_CurDir,&pidlDirectory);

	pidlComplete = ILCombine(pidlDirectory,m_pExtraItemInfo[iItemInternal].pridl);

	SHGetFileInfo((LPTSTR)pidlComplete,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_TYPENAME);

	StringCchCopy(szGroupHeader,cchMax,shfi.szTypeName);

	CoTaskMemFree(pidlComplete);
	CoTaskMemFree(pidlDirectory);
}

void CShellBrowser::DetermineItemDateGroup(int iItemInternal,int iDateType,TCHAR *szGroupHeader,int cchMax) const
{
	/* TODO: Move strings into string table. */
	SYSTEMTIME	stCurrentTime;
	SYSTEMTIME	stFileTime;
	FILETIME	ftLocalFileTime;
	TCHAR		*ModifiedGroups[] = {_T("Today"),_T("Yesterday"),_T("This Week"),_T("Last Week"),_T("This Month"),
		_T("Last Month"),_T("This Year"),_T("Last Year"),_T("Two Years Ago"),_T("Long ago"),_T("Unspecified")};
	int			iModified;

	GetLocalTime(&stCurrentTime);

	switch(iDateType)
	{
	case GROUP_BY_DATEMODIFIED:
		FileTimeToLocalFileTime(&m_pwfdFiles[iItemInternal].ftLastWriteTime,&ftLocalFileTime);
		break;

	case GROUP_BY_DATECREATED:
		FileTimeToLocalFileTime(&m_pwfdFiles[iItemInternal].ftCreationTime,&ftLocalFileTime);
		break;

	case GROUP_BY_DATEACCESSED:
		FileTimeToLocalFileTime(&m_pwfdFiles[iItemInternal].ftLastAccessTime,&ftLocalFileTime);
		break;
	}

	FileTimeToSystemTime(&ftLocalFileTime,&stFileTime);

	if(stFileTime.wYear == stCurrentTime.wYear)
	{
		if(stFileTime.wMonth == stCurrentTime.wMonth)
		{
			if(stFileTime.wDay == stCurrentTime.wDay)
				iModified = 0;
			else if(stFileTime.wDay == (stCurrentTime.wDay - 1))
				iModified = 1;
			else if(stFileTime.wDay >= (stCurrentTime.wDay - 7))
				iModified = 3;
			else
				iModified = 4;
		}
		else
		{
			if(stFileTime.wMonth == (stCurrentTime.wMonth - 1))
				iModified = 5;
			else
				iModified = 6;
		}
	}
	else
	{
		if(stFileTime.wYear == (stCurrentTime.wYear - 1))
			iModified = 6;
		else if(stFileTime.wYear == (stCurrentTime.wYear - 2))
			iModified = 7;
		else
			iModified = 8;
	}

	StringCchCopy(szGroupHeader,cchMax,ModifiedGroups[iModified]);
}

void CShellBrowser::DetermineItemSummaryGroup(int iItemInternal, const SHCOLUMNID *pscid, TCHAR *szGroupHeader, size_t cchMax) const
{
	TCHAR szDetail[512];
	HRESULT hr = GetItemDetails(iItemInternal, pscid, szDetail, SIZEOF_ARRAY(szDetail));

	if(SUCCEEDED(hr) && lstrlen(szDetail) > 0)
	{
		StringCchCopy(szGroupHeader, cchMax, szDetail);
	}
	else
	{
		StringCchCopy(szGroupHeader, cchMax, _T("Unspecified"));
	}
}

/* TODO: Need to sort based on percentage free. */
void CShellBrowser::DetermineItemFreeSpaceGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	std::list<TypeGroup_t>::iterator itr;
	LPITEMIDLIST pidlComplete	= NULL;
	LPITEMIDLIST pidlDirectory	= NULL;
	TCHAR szFreeSpace[MAX_PATH];
	IShellFolder *pShellFolder	= NULL;
	LPITEMIDLIST pidlRelative	= NULL;
	STRRET str;
	TCHAR szItem[MAX_PATH];
	ULARGE_INTEGER nTotalBytes;
	ULARGE_INTEGER nFreeBytes;
	BOOL bRoot;
	BOOL bRes = FALSE;

	GetIdlFromParsingName(m_CurDir,&pidlDirectory);
	pidlComplete = ILCombine(pidlDirectory,m_pExtraItemInfo[iItemInternal].pridl);
	SHBindToParent(pidlComplete, IID_PPV_ARGS(&pShellFolder), (LPCITEMIDLIST *)&pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szItem,SIZEOF_ARRAY(szItem));

	CoTaskMemFree(pidlDirectory);
	CoTaskMemFree(pidlComplete);
	pShellFolder->Release();

	bRoot = PathIsRoot(szItem);

	if(bRoot)
	{
		bRes = GetDiskFreeSpaceEx(szItem,NULL,&nTotalBytes,&nFreeBytes);

		LARGE_INTEGER lDiv1;
		LARGE_INTEGER lDiv2;

		lDiv1.QuadPart = 100;
		lDiv2.QuadPart = 10;

		/* Divide by 10 to remove the one's digit, then multiply
		by 10 so that only the ten's digit rmains. */
		StringCchPrintf(szFreeSpace,SIZEOF_ARRAY(szFreeSpace),
			_T("%I64d%% free"),(((nFreeBytes.QuadPart * lDiv1.QuadPart) / nTotalBytes.QuadPart) / lDiv2.QuadPart) * lDiv2.QuadPart);
	}
	
	if(!bRoot || !bRes)
	{
		StringCchCopy(szFreeSpace,SIZEOF_ARRAY(szFreeSpace),_T("Unspecified"));
	}

	StringCchCopy(szGroupHeader,cchMax,szFreeSpace);
}

void CShellBrowser::DetermineItemAttributeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR FullFileName[MAX_PATH];
	std::list<TypeGroup_t>::iterator itr;
	TCHAR szAttributes[32];

	StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
	PathAppend(FullFileName,m_pwfdFiles[iItemInternal].cFileName);

	BuildFileAttributeString(FullFileName,szAttributes,
		SIZEOF_ARRAY(szAttributes));

	StringCchCopy(szGroupHeader,cchMax,szAttributes);
}

void CShellBrowser::DetermineItemOwnerGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR FullFileName[MAX_PATH];
	std::list<TypeGroup_t>::iterator itr;
	TCHAR szOwner[512];

	StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
	PathAppend(FullFileName,m_pwfdFiles[iItemInternal].cFileName);

	BOOL ret = GetFileOwner(FullFileName,szOwner,SIZEOF_ARRAY(szOwner));

	if(!ret)
	{
		StringCchCopy(szOwner,SIZEOF_ARRAY(szOwner),EMPTY_STRING);
	}

	StringCchCopy(szGroupHeader,cchMax,szOwner);
}

void CShellBrowser::DetermineItemVersionGroup(int iItemInternal,TCHAR *szVersionType,TCHAR *szGroupHeader,int cchMax) const
{
	BOOL bGroupFound = FALSE;
	TCHAR FullFileName[MAX_PATH];
	std::list<TypeGroup_t>::iterator itr;
	TCHAR szVersion[512];
	BOOL bVersionInfoObtained;

	StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
	PathAppend(FullFileName,m_pwfdFiles[iItemInternal].cFileName);

	bVersionInfoObtained = GetVersionInfoString(FullFileName,
		szVersionType,szVersion,SIZEOF_ARRAY(szVersion));

	bGroupFound = FALSE;

	if(!bVersionInfoObtained)
		StringCchCopy(szVersion,SIZEOF_ARRAY(szVersion),_T("Unspecified"));

	StringCchCopy(szGroupHeader,cchMax,szVersion);
}

void CShellBrowser::DetermineItemCameraPropertyGroup(int iItemInternal,PROPID PropertyId,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR szFullFileName[MAX_PATH];
	std::list<TypeGroup_t>::iterator itr;
	TCHAR szProperty[512];
	BOOL bRes;

	StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_CurDir);
	PathAppend(szFullFileName,m_pwfdFiles[iItemInternal].cFileName);

	bRes = ReadImageProperty(szFullFileName,PropertyId,szProperty,
		SIZEOF_ARRAY(szProperty));

	if(!bRes)
		StringCchCopy(szProperty,SIZEOF_ARRAY(szProperty),_T("Other"));

	StringCchCopy(szGroupHeader,cchMax,szProperty);
}

void CShellBrowser::DetermineItemExtensionGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR FullFileName[MAX_PATH];
	TCHAR *pExt;

	StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
	PathAppend(FullFileName,m_pwfdFiles[iItemInternal].cFileName);

	pExt = PathFindExtension(FullFileName);

	/* TODO: Folder? */
	if(*pExt == '\0')
	{
		/* TODO: Move into string table. */
		StringCchCopy(szGroupHeader,cchMax,_T("None"));
	}
	else
	{
		StringCchCopy(szGroupHeader,cchMax,pExt);
	}
}

void CShellBrowser::DetermineItemFileSystemGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	LPITEMIDLIST pidlComplete = NULL;
	IShellFolder *pShellFolder	= NULL;
	LPITEMIDLIST pidlRelative	= NULL;
	TCHAR szFileSystemName[MAX_PATH];
	TCHAR szItem[MAX_PATH];
	STRRET str;
	BOOL bRoot;
	BOOL bRes;

	pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[iItemInternal].pridl);

	SHBindToParent(pidlComplete, IID_PPV_ARGS(&pShellFolder), (LPCITEMIDLIST *)&pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szItem,SIZEOF_ARRAY(szItem));

	bRoot = PathIsRoot(szItem);

	if(bRoot)
	{
		bRes = GetVolumeInformation(szItem,NULL,0,NULL,NULL,NULL,szFileSystemName,
			SIZEOF_ARRAY(szFileSystemName));

		if(!bRes || *szFileSystemName == '\0')
		{
			/* TODO: Move into string table. */
			StringCchCopy(szFileSystemName,SIZEOF_ARRAY(szFileSystemName),_T("Unspecified"));
		}
	}
	else
	{
		/* TODO: Move into string table. */
		StringCchCopy(szFileSystemName,SIZEOF_ARRAY(szFileSystemName),_T("Unspecified"));
	}

	StringCchCopy(szGroupHeader,cchMax,szFileSystemName);

	pShellFolder->Release();
	CoTaskMemFree(pidlComplete);
}

/* TODO: Fix. Need to check for each adapter. */
void CShellBrowser::DetermineItemNetworkStatus(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	/* When this function is
	properly implemented, this
	can be removed. */
	UNREFERENCED_PARAMETER(iItemInternal);

	std::list<TypeGroup_t>::iterator itr;

	TCHAR szStatus[32] = EMPTY_STRING;
	IP_ADAPTER_ADDRESSES *pAdapterAddresses = NULL;
	UINT uStatusID = 0;
	ULONG ulOutBufLen = 0;

	GetAdaptersAddresses(AF_UNSPEC,0,NULL,NULL,&ulOutBufLen);

	pAdapterAddresses = (IP_ADAPTER_ADDRESSES *)malloc(ulOutBufLen);

	GetAdaptersAddresses(AF_UNSPEC,0,NULL,pAdapterAddresses,&ulOutBufLen);

	/* TODO: These strings need to be setup correctly. */
	/*switch(pAdapterAddresses->OperStatus)
	{
		case IfOperStatusUp:
			uStatusID = IDS_NETWORKADAPTER_CONNECTED;
			break;

		case IfOperStatusDown:
			uStatusID = IDS_NETWORKADAPTER_DISCONNECTED;
			break;

		case IfOperStatusTesting:
			uStatusID = IDS_NETWORKADAPTER_TESTING;
			break;

		case IfOperStatusUnknown:
			uStatusID = IDS_NETWORKADAPTER_UNKNOWN;
			break;

		case IfOperStatusDormant:
			uStatusID = IDS_NETWORKADAPTER_DORMANT;
			break;

		case IfOperStatusNotPresent:
			uStatusID = IDS_NETWORKADAPTER_NOTPRESENT;
			break;

		case IfOperStatusLowerLayerDown:
			uStatusID = IDS_NETWORKADAPTER_LOWLAYER;
			break;
	}*/

	LoadString(m_hResourceModule,uStatusID,
		szStatus,SIZEOF_ARRAY(szStatus));

	StringCchCopy(szGroupHeader,cchMax,szStatus);
}

void CShellBrowser::InsertItemIntoGroup(int iItem,int iGroupId)
{
	LVITEM Item;

	/* Move the item into the group. */
	Item.mask		= LVIF_GROUPID;
	Item.iItem		= iItem;
	Item.iSubItem	= 0;
	Item.iGroupId	= iGroupId;
	ListView_SetItem(m_hListView,&Item);
}

void CShellBrowser::MoveItemsIntoGroups(void)
{
	LVITEM Item;
	int nItems;
	int iGroupId;
	int i = 0;

	ListView_RemoveAllGroups(m_hListView);
	ListView_EnableGroupView(m_hListView,TRUE);

	nItems = ListView_GetItemCount(m_hListView);

	SendMessage(m_hListView,WM_SETREDRAW,(WPARAM)FALSE,(LPARAM)NULL);

	m_GroupList.clear();
	m_iGroupId = 0;

	g_bSortAscending = m_bSortAscending;

	g_pGroupList = &m_GroupList;

	for(i = 0;i < nItems ;i++)
	{
		Item.mask		= LVIF_PARAM;
		Item.iItem		= i;
		Item.iSubItem	= 0;
		ListView_GetItem(m_hListView,&Item);

		iGroupId = DetermineItemGroup((int)Item.lParam);

		InsertItemIntoGroup(i,iGroupId);
	}

	SendMessage(m_hListView,WM_SETREDRAW,(WPARAM)TRUE,(LPARAM)NULL);
}