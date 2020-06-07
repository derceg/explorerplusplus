// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"
#include "ItemData.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "SortModes.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/TimeHelper.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <wil/common.h>
#include <iphlpapi.h>
#include <propkey.h>
#include <cassert>
#include <list>

namespace
{
const UINT KBYTE = 1024;
const UINT MBYTE = 1024 * 1024;
const UINT GBYTE = 1024 * 1024 * 1024;
}

BOOL ShellBrowser::GetShowInGroups() const
{
	return m_folderSettings.showInGroups;
}

/* Simply sets the grouping flag, without actually moving
items into groups. */
void ShellBrowser::SetShowInGroupsFlag(BOOL bShowInGroups)
{
	m_folderSettings.showInGroups = bShowInGroups;
}

void ShellBrowser::SetShowInGroups(BOOL bShowInGroups)
{
	m_folderSettings.showInGroups = bShowInGroups;

	if (!m_folderSettings.showInGroups)
	{
		ListView_EnableGroupView(m_hListView, FALSE);
		SortFolder(m_folderSettings.sortMode);
		return;
	}
	else
	{
		MoveItemsIntoGroups();
	}
}

INT CALLBACK ShellBrowser::GroupNameComparisonStub(INT Group1_ID, INT Group2_ID, void *pvData)
{
	auto *shellBrowser = reinterpret_cast<ShellBrowser *>(pvData);
	return shellBrowser->GroupNameComparison(Group1_ID, Group2_ID);
}

INT CALLBACK ShellBrowser::GroupNameComparison(INT Group1_ID, INT Group2_ID)
{
	int iReturnValue;

	std::wstring groupHeader1 = RetrieveGroupHeader(Group1_ID);
	std::wstring groupHeader2 = RetrieveGroupHeader(Group2_ID);

	if (groupHeader1 == L"Other" && groupHeader2 != L"Other")
	{
		iReturnValue = 1;
	}
	else if (groupHeader1 != L"Other" && groupHeader2 == L"Other")
	{
		iReturnValue = -1;
	}
	else if (groupHeader1 == L"Other" && groupHeader2 == L"Other")
	{
		iReturnValue = 0;
	}
	else
	{
		iReturnValue = groupHeader1.compare(groupHeader2);
	}

	if (!m_folderSettings.sortAscending)
	{
		iReturnValue = -iReturnValue;
	}

	return iReturnValue;
}

INT CALLBACK ShellBrowser::GroupFreeSpaceComparisonStub(INT Group1_ID, INT Group2_ID, void *pvData)
{
	auto *shellBrowser = reinterpret_cast<ShellBrowser *>(pvData);
	return shellBrowser->GroupFreeSpaceComparison(Group1_ID, Group2_ID);
}

INT CALLBACK ShellBrowser::GroupFreeSpaceComparison(INT Group1_ID, INT Group2_ID)
{
	int iReturnValue;

	std::wstring groupHeader1 = RetrieveGroupHeader(Group1_ID);
	std::wstring groupHeader2 = RetrieveGroupHeader(Group2_ID);

	if (groupHeader1 == L"Unspecified" && groupHeader2 != L"Unspecified")
	{
		iReturnValue = 1;
	}
	else if (groupHeader1 != L"Unspecified" && groupHeader2 == L"Unspecified")
	{
		iReturnValue = -1;
	}
	else if (groupHeader1 == L"Unspecified" && groupHeader2 == L"Unspecified")
	{
		iReturnValue = 0;
	}
	else
	{
		iReturnValue = groupHeader1.compare(groupHeader2);
	}

	if (!m_folderSettings.sortAscending)
	{
		iReturnValue = -iReturnValue;
	}

	return iReturnValue;
}

std::wstring ShellBrowser::RetrieveGroupHeader(int groupId)
{
	auto itr =
		std::find_if(m_GroupList.begin(), m_GroupList.end(), [groupId](const TypeGroup_t &group) {
			return group.iGroupId == groupId;
		});

	return itr->header;
}

/*
 * Determines the id of the group the specified
 * item belongs to.
 */
int ShellBrowser::DetermineItemGroup(int iItemInternal)
{
	BasicItemInfo_t basicItemInfo = getBasicItemInfo(iItemInternal);
	PFNLVGROUPCOMPARE groupComparison = nullptr;
	std::wstring groupHeader;

	switch (m_folderSettings.sortMode)
	{
	case SortMode::Name:
		groupHeader = DetermineItemNameGroup(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Type:
		groupHeader = DetermineItemTypeGroupVirtual(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Size:
		groupHeader = DetermineItemSizeGroup(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::DateModified:
		groupHeader = DetermineItemDateGroup(basicItemInfo, GroupByDateType::Modified);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::TotalSize:
		groupHeader = DetermineItemTotalSizeGroup(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::FreeSpace:
		groupHeader = DetermineItemFreeSpaceGroup(basicItemInfo);
		groupComparison = GroupFreeSpaceComparisonStub;
		break;

	case SortMode::DateDeleted:
		break;

	case SortMode::OriginalLocation:
		groupHeader = DetermineItemSummaryGroup(
			basicItemInfo, &SCID_ORIGINAL_LOCATION, m_config->globalFolderSettings);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Attributes:
		groupHeader = DetermineItemAttributeGroup(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::ShortName:
		groupHeader = DetermineItemNameGroup(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Owner:
		groupHeader = DetermineItemOwnerGroup(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::ProductName:
		groupHeader = DetermineItemVersionGroup(basicItemInfo, _T("ProductName"));
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Company:
		groupHeader = DetermineItemVersionGroup(basicItemInfo, _T("CompanyName"));
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Description:
		groupHeader = DetermineItemVersionGroup(basicItemInfo, _T("FileDescription"));
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::FileVersion:
		groupHeader = DetermineItemVersionGroup(basicItemInfo, _T("FileVersion"));
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::ProductVersion:
		groupHeader = DetermineItemVersionGroup(basicItemInfo, _T("ProductVersion"));
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::ShortcutTo:
		break;

	case SortMode::HardLinks:
		break;

	case SortMode::Extension:
		groupHeader = DetermineItemExtensionGroup(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Created:
		groupHeader = DetermineItemDateGroup(basicItemInfo, GroupByDateType::Created);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Accessed:
		groupHeader = DetermineItemDateGroup(basicItemInfo, GroupByDateType::Accessed);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Title:
		groupHeader =
			DetermineItemSummaryGroup(basicItemInfo, &PKEY_Title, m_config->globalFolderSettings);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Subject:
		groupHeader =
			DetermineItemSummaryGroup(basicItemInfo, &PKEY_Subject, m_config->globalFolderSettings);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Authors:
		groupHeader =
			DetermineItemSummaryGroup(basicItemInfo, &PKEY_Author, m_config->globalFolderSettings);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Keywords:
		groupHeader = DetermineItemSummaryGroup(
			basicItemInfo, &PKEY_Keywords, m_config->globalFolderSettings);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Comments:
		groupHeader =
			DetermineItemSummaryGroup(basicItemInfo, &PKEY_Comment, m_config->globalFolderSettings);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::CameraModel:
		groupHeader = DetermineItemCameraPropertyGroup(basicItemInfo, PropertyTagEquipModel);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::DateTaken:
		groupHeader = DetermineItemCameraPropertyGroup(basicItemInfo, PropertyTagDateTime);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Width:
		groupHeader = DetermineItemCameraPropertyGroup(basicItemInfo, PropertyTagImageWidth);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::Height:
		groupHeader = DetermineItemCameraPropertyGroup(basicItemInfo, PropertyTagImageHeight);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::VirtualComments:
		break;

	case SortMode::FileSystem:
		groupHeader = DetermineItemFileSystemGroup(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	case SortMode::NumPrinterDocuments:
		break;

	case SortMode::PrinterStatus:
		break;

	case SortMode::PrinterComments:
		break;

	case SortMode::PrinterLocation:
		break;

	case SortMode::NetworkAdapterStatus:
		groupHeader = DetermineItemNetworkStatus(basicItemInfo);
		groupComparison = GroupNameComparisonStub;
		break;

	default:
		assert(false);
		break;
	}

	return CheckGroup(groupHeader, groupComparison);
}

/*
 * Checks if a group with specified id is already
 * in the listview. If not, the group is inserted
 * into its sorted position with the specified
 * header text.
 */
int ShellBrowser::CheckGroup(std::wstring_view groupHeader, PFNLVGROUPCOMPARE groupComparison)
{
	auto itr = std::find_if(
		m_GroupList.begin(), m_GroupList.end(), [groupHeader](const TypeGroup_t &group) {
			return group.header == groupHeader;
		});

	auto generateListViewHeader = [](std::wstring_view header, int numItems) {
		return std::wstring(header) + L" (" + std::to_wstring(numItems) + L")";
	};

	if (itr != m_GroupList.end())
	{
		itr->nItems++;

		std::wstring listViewHeader = generateListViewHeader(groupHeader, itr->nItems);

		LVGROUP lvGroup;
		lvGroup.cbSize = sizeof(LVGROUP);
		lvGroup.mask = LVGF_HEADER;
		lvGroup.pszHeader = listViewHeader.data();
		ListView_SetGroupInfo(m_hListView, itr->iGroupId, &lvGroup);

		return itr->iGroupId;
	}

	int groupId = m_iGroupId++;

	TypeGroup_t typeGroup;
	typeGroup.header = groupHeader;
	typeGroup.iGroupId = groupId;
	typeGroup.nItems = 1;
	m_GroupList.push_back(typeGroup);

	std::wstring listViewHeader = generateListViewHeader(groupHeader, typeGroup.nItems);

	LVINSERTGROUPSORTED lvigs;
	lvigs.lvGroup.cbSize = sizeof(LVGROUP);
	lvigs.lvGroup.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	lvigs.lvGroup.state = LVGS_COLLAPSIBLE;
	lvigs.lvGroup.pszHeader = listViewHeader.data();
	lvigs.lvGroup.iGroupId = groupId;
	lvigs.lvGroup.stateMask = 0;
	lvigs.pfnGroupCompare = groupComparison;
	lvigs.pvData = reinterpret_cast<void *>(this);
	ListView_InsertGroupSorted(m_hListView, &lvigs);

	return groupId;
}

/*
 * Determines the id of the group to which the specified
 * item belongs, based on the item's name.
 * Also returns the text header for the group when szGroupHeader
 * is non-NULL.
 */
/* TODO: These groups have changed as of Windows Visa.*/
std::wstring ShellBrowser::DetermineItemNameGroup(const BasicItemInfo_t &itemInfo) const
{
	/* Take the first character of the item's name,
	and use it to determine which group it belongs to. */
	TCHAR ch = itemInfo.szDisplayName[0];

	if (iswalpha(ch))
	{
		return std::wstring(1, towupper(ch));
	}
	else
	{
		return L"Other";
	}
}

/*
 * Determines the id of the group to which the specified
 * item belongs, based on the item's size.
 * Also returns the text header for the group when szGroupHeader
 * is non-NULL.
 */
std::wstring ShellBrowser::DetermineItemSizeGroup(const BasicItemInfo_t &itemInfo) const
{
	TCHAR *sizeGroups[] = { _T("Folders"), _T("Tiny"), _T("Small"), _T("Medium"), _T("Large"),
		_T("Huge") };
	int sizeGroupLimits[] = { 0, 0, 32 * KBYTE, 100 * KBYTE, MBYTE, 10 * MBYTE };
	int nGroups = 6;
	int iSize;
	int i;

	if ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		/* This item is a folder. */
		iSize = 0;
	}
	else
	{
		i = nGroups - 1;

		double fileSize = itemInfo.wfd.nFileSizeLow + (itemInfo.wfd.nFileSizeHigh * pow(2.0, 32.0));

		/* Check which of the size groups this item belongs to. */
		while (fileSize < sizeGroupLimits[i] && i > 0)
		{
			i--;
		}

		iSize = i;
	}

	return sizeGroups[iSize];
}

/*
 * Determines the id of the group to which the specified
 * drive/folder item belongs, based on the item's total size.
 * Also returns the text header for the group when szGroupHeader
 * is non-NULL.
 */
/* TODO: These groups have changed as of Windows Vista. */
std::wstring ShellBrowser::DetermineItemTotalSizeGroup(const BasicItemInfo_t &itemInfo) const
{
	IShellFolder *pShellFolder = nullptr;
	PCITEMID_CHILD pidlRelative = nullptr;
	TCHAR *sizeGroups[] = { _T("Unspecified"), _T("Small"), _T("Medium"), _T("Huge"),
		_T("Gigantic") };
	TCHAR szItem[MAX_PATH];
	STRRET str;
	ULARGE_INTEGER nTotalBytes;
	ULARGE_INTEGER nFreeBytes;
	BOOL bRoot;
	BOOL bRes = FALSE;
	ULARGE_INTEGER totalSizeGroupLimits[6];
	int nGroups = 5;
	int iSize = 0;
	int i;

	totalSizeGroupLimits[0].QuadPart = 0;
	totalSizeGroupLimits[1].QuadPart = 0;
	totalSizeGroupLimits[2].QuadPart = GBYTE;
	totalSizeGroupLimits[3].QuadPart = 20 * totalSizeGroupLimits[2].QuadPart;
	totalSizeGroupLimits[4].QuadPart = 100 * totalSizeGroupLimits[2].QuadPart;

	SHBindToParent(itemInfo.pidlComplete.get(), IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative, SHGDN_FORPARSING, &str);
	StrRetToBuf(&str, pidlRelative, szItem, SIZEOF_ARRAY(szItem));

	bRoot = PathIsRoot(szItem);

	if (bRoot)
	{
		bRes = GetDiskFreeSpaceEx(szItem, nullptr, &nTotalBytes, &nFreeBytes);

		pShellFolder->Release();

		i = nGroups - 1;

		while (nTotalBytes.QuadPart < totalSizeGroupLimits[i].QuadPart && i > 0)
		{
			i--;
		}

		iSize = i;
	}

	if (!bRoot || !bRes)
	{
		iSize = 0;
	}

	return sizeGroups[iSize];
}

std::wstring ShellBrowser::DetermineItemTypeGroupVirtual(const BasicItemInfo_t &itemInfo) const
{
	SHFILEINFO shfi;
	SHGetFileInfo(
		(LPTSTR) itemInfo.pidlComplete.get(), 0, &shfi, sizeof(shfi), SHGFI_PIDL | SHGFI_TYPENAME);

	return shfi.szTypeName;
}

std::wstring ShellBrowser::DetermineItemDateGroup(
	const BasicItemInfo_t &itemInfo, GroupByDateType dateType) const
{
	using namespace boost::gregorian;
	using namespace boost::posix_time;

	SYSTEMTIME stFileTime;
	BOOL ret = FALSE;

	switch (dateType)
	{
	case GroupByDateType::Modified:
		ret = FileTimeToLocalSystemTime(&itemInfo.wfd.ftLastWriteTime, &stFileTime);
		break;

	case GroupByDateType::Created:
		ret = FileTimeToLocalSystemTime(&itemInfo.wfd.ftCreationTime, &stFileTime);
		break;

	case GroupByDateType::Accessed:
		ret = FileTimeToLocalSystemTime(&itemInfo.wfd.ftLastAccessTime, &stFileTime);
		break;

	default:
		throw std::runtime_error("Incorrect date type");
	}

	if (!ret)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_UNSPECIFIED);
	}

	FILETIME localFileTime;
	ret = SystemTimeToFileTime(&stFileTime, &localFileTime);

	if (!ret)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_UNSPECIFIED);
	}

	auto filePosixTime = from_ftime<ptime>(localFileTime);
	date fileDate = filePosixTime.date();

	date today = day_clock::local_day();

	if (fileDate > today)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_FUTURE);
	}

	if (fileDate == today)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_TODAY);
	}

	date yesterday = today - days(1);

	if (fileDate == yesterday)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_YESTERDAY);
	}

	// Note that this assumes that Sunday is the first day of the week.
	unsigned short currentWeekday = today.day_of_week().as_number();
	date startOfWeek = today - days(currentWeekday);

	if (fileDate >= startOfWeek)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_THIS_WEEK);
	}

	date startOfLastWeek = startOfWeek - weeks(1);

	if (fileDate >= startOfLastWeek)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_LAST_WEEK);
	}

	date startOfMonth = date(today.year(), today.month(), 1);

	if (fileDate >= startOfMonth)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_THIS_MONTH);
	}

	date startOfLastMonth = startOfMonth - months(1);

	if (fileDate >= startOfLastMonth)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_LAST_MONTH);
	}

	date startOfYear = date(today.year(), 1, 1);

	if (fileDate >= startOfYear)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_THIS_YEAR);
	}

	date startOfLastYear = startOfYear - years(1);

	if (fileDate >= startOfLastYear)
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_LAST_YEAR);
	}

	return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_DATE_LONG_AGO);
}

std::wstring ShellBrowser::DetermineItemSummaryGroup(const BasicItemInfo_t &itemInfo,
	const SHCOLUMNID *pscid, const GlobalFolderSettings &globalFolderSettings) const
{
	TCHAR szDetail[512];
	HRESULT hr =
		GetItemDetails(itemInfo, pscid, szDetail, SIZEOF_ARRAY(szDetail), globalFolderSettings);

	if (SUCCEEDED(hr) && lstrlen(szDetail) > 0)
	{
		return szDetail;
	}
	else
	{
		return L"Unspecified";
	}
}

/* TODO: Need to sort based on percentage free. */
std::wstring ShellBrowser::DetermineItemFreeSpaceGroup(const BasicItemInfo_t &itemInfo) const
{
	TCHAR szFreeSpace[MAX_PATH];
	IShellFolder *pShellFolder = nullptr;
	PCITEMID_CHILD pidlRelative = nullptr;
	STRRET str;
	TCHAR szItem[MAX_PATH];
	ULARGE_INTEGER nTotalBytes;
	ULARGE_INTEGER nFreeBytes;
	BOOL bRoot;
	BOOL bRes = FALSE;

	SHBindToParent(itemInfo.pidlComplete.get(), IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative, SHGDN_FORPARSING, &str);
	StrRetToBuf(&str, pidlRelative, szItem, SIZEOF_ARRAY(szItem));

	pShellFolder->Release();

	bRoot = PathIsRoot(szItem);

	if (bRoot)
	{
		bRes = GetDiskFreeSpaceEx(szItem, nullptr, &nTotalBytes, &nFreeBytes);

		LARGE_INTEGER lDiv1;
		LARGE_INTEGER lDiv2;

		lDiv1.QuadPart = 100;
		lDiv2.QuadPart = 10;

		/* Divide by 10 to remove the one's digit, then multiply
		by 10 so that only the ten's digit rmains. */
		StringCchPrintf(szFreeSpace, SIZEOF_ARRAY(szFreeSpace), _T("%I64d%% free"),
			(((nFreeBytes.QuadPart * lDiv1.QuadPart) / nTotalBytes.QuadPart) / lDiv2.QuadPart)
				* lDiv2.QuadPart);
	}

	if (!bRoot || !bRes)
	{
		StringCchCopy(szFreeSpace, SIZEOF_ARRAY(szFreeSpace), _T("Unspecified"));
	}

	return szFreeSpace;
}

std::wstring ShellBrowser::DetermineItemAttributeGroup(const BasicItemInfo_t &itemInfo) const
{
	std::wstring fullFileName = itemInfo.getFullPath();

	TCHAR szAttributes[32];
	BuildFileAttributeString(fullFileName.c_str(), szAttributes, std::size(szAttributes));

	return szAttributes;
}

std::wstring ShellBrowser::DetermineItemOwnerGroup(const BasicItemInfo_t &itemInfo) const
{
	std::wstring fullFileName = itemInfo.getFullPath();

	TCHAR szOwner[512];
	BOOL ret = GetFileOwner(fullFileName.c_str(), szOwner, std::size(szOwner));

	if (ret)
	{
		return szOwner;
	}

	return std::wstring();
}

std::wstring ShellBrowser::DetermineItemVersionGroup(
	const BasicItemInfo_t &itemInfo, const TCHAR *szVersionType) const
{
	std::wstring fullFileName = itemInfo.getFullPath();

	TCHAR szVersion[512];
	BOOL bVersionInfoObtained = GetVersionInfoString(
		fullFileName.c_str(), szVersionType, szVersion, static_cast<UINT>(std::size(szVersion)));

	if (bVersionInfoObtained)
	{
		return szVersion;
	}

	return L"Unspecified";
}

std::wstring ShellBrowser::DetermineItemCameraPropertyGroup(
	const BasicItemInfo_t &itemInfo, PROPID PropertyId) const
{
	std::wstring fullFileName = itemInfo.getFullPath();

	TCHAR szProperty[512];
	BOOL bRes = ReadImageProperty(
		fullFileName.c_str(), PropertyId, szProperty, static_cast<int>(std::size(szProperty)));

	if (bRes)
	{
		return szProperty;
	}

	return L"Other";
}

std::wstring ShellBrowser::DetermineItemExtensionGroup(const BasicItemInfo_t &itemInfo) const
{
	if (WI_IsFlagSet(itemInfo.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_EXTENSION_FOLDER);
	}

	std::wstring fullFileName = itemInfo.getFullPath();
	TCHAR *pExt = PathFindExtension(fullFileName.c_str());

	if (*pExt != '\0')
	{
		return pExt;
	}

	return ResourceHelper::LoadString(m_hResourceModule, IDS_GROUPBY_EXTENSION_NONE);
}

std::wstring ShellBrowser::DetermineItemFileSystemGroup(const BasicItemInfo_t &itemInfo) const
{
	IShellFolder *pShellFolder = nullptr;
	PCITEMID_CHILD pidlRelative = nullptr;
	TCHAR szFileSystemName[MAX_PATH];
	TCHAR szItem[MAX_PATH];
	STRRET str;
	BOOL bRoot;
	BOOL bRes;

	SHBindToParent(itemInfo.pidlComplete.get(), IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative, SHGDN_FORPARSING, &str);
	StrRetToBuf(&str, pidlRelative, szItem, SIZEOF_ARRAY(szItem));

	bRoot = PathIsRoot(szItem);

	if (bRoot)
	{
		bRes = GetVolumeInformation(szItem, nullptr, 0, nullptr, nullptr, nullptr, szFileSystemName,
			SIZEOF_ARRAY(szFileSystemName));

		if (!bRes || *szFileSystemName == '\0')
		{
			LoadString(m_hResourceModule, IDS_GROUPBY_UNSPECIFIED, szFileSystemName,
				SIZEOF_ARRAY(szFileSystemName));
		}
	}
	else
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_UNSPECIFIED, szFileSystemName,
			SIZEOF_ARRAY(szFileSystemName));
	}

	pShellFolder->Release();

	return szFileSystemName;
}

/* TODO: Fix. Need to check for each adapter. */
std::wstring ShellBrowser::DetermineItemNetworkStatus(const BasicItemInfo_t &itemInfo) const
{
	/* When this function is
	properly implemented, this
	can be removed. */
	UNREFERENCED_PARAMETER(itemInfo);

	TCHAR szStatus[32] = EMPTY_STRING;
	IP_ADAPTER_ADDRESSES *pAdapterAddresses = nullptr;
	UINT uStatusID = 0;
	ULONG ulOutBufLen = 0;

	GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &ulOutBufLen);

	pAdapterAddresses = (IP_ADAPTER_ADDRESSES *) malloc(ulOutBufLen);

	GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, pAdapterAddresses, &ulOutBufLen);

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

	LoadString(m_hResourceModule, uStatusID, szStatus, SIZEOF_ARRAY(szStatus));

	return szStatus;
}

void ShellBrowser::InsertItemIntoGroup(int iItem, int iGroupId)
{
	LVITEM item;

	/* Move the item into the group. */
	item.mask = LVIF_GROUPID;
	item.iItem = iItem;
	item.iSubItem = 0;
	item.iGroupId = iGroupId;
	ListView_SetItem(m_hListView, &item);
}

void ShellBrowser::MoveItemsIntoGroups()
{
	LVITEM item;
	int nItems;
	int iGroupId;
	int i = 0;

	ListView_RemoveAllGroups(m_hListView);
	ListView_EnableGroupView(m_hListView, TRUE);

	nItems = ListView_GetItemCount(m_hListView);

	SendMessage(m_hListView, WM_SETREDRAW, (WPARAM) FALSE, (LPARAM) NULL);

	m_GroupList.clear();
	m_iGroupId = 0;

	for (i = 0; i < nItems; i++)
	{
		item.mask = LVIF_PARAM;
		item.iItem = i;
		item.iSubItem = 0;
		ListView_GetItem(m_hListView, &item);

		iGroupId = DetermineItemGroup((int) item.lParam);

		InsertItemIntoGroup(i, iGroupId);
	}

	SendMessage(m_hListView, WM_SETREDRAW, (WPARAM) TRUE, (LPARAM) NULL);
}