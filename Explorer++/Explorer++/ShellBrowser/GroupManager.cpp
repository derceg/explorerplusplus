// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "App.h"
#include "ColumnDataRetrieval.h"
#include "Config.h"
#include "ItemData.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "SortModes.h"
#include "../Helper/Helper.h"
#include "../Helper/ScopedRedrawDisabler.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/TimeHelper.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/integer_traits.hpp>
#include <glog/logging.h>
#include <wil/common.h>
#include <iphlpapi.h>
#include <propkey.h>
#include <cassert>
#include <format>

namespace
{
const uint64_t KBYTE = 1024;
const uint64_t MBYTE = 1024 * 1024;
const uint64_t GBYTE = 1024 * 1024 * 1024;
}

bool ShellBrowserImpl::GetShowInGroups() const
{
	return m_folderSettings.showInGroups;
}

void ShellBrowserImpl::SetShowInGroups(bool showInGroups)
{
	if (showInGroups == m_folderSettings.showInGroups)
	{
		return;
	}

	m_folderSettings.showInGroups = showInGroups;

	if (!showInGroups)
	{
		ListView_EnableGroupView(m_listView, false);
		ListView_RemoveAllGroups(m_listView);
		m_directoryState.groups.clear();
	}
	else
	{
		MoveItemsIntoGroups();
	}
}

int CALLBACK ShellBrowserImpl::GroupComparisonStub(int id1, int id2, void *data)
{
	auto *shellBrowser = reinterpret_cast<ShellBrowserImpl *>(data);
	return shellBrowser->GroupComparison(id1, id2);
}

int ShellBrowserImpl::GroupComparison(int id1, int id2)
{
	const auto &group1 = GetListViewGroupById(id1);
	const auto &group2 = GetListViewGroupById(id2);
	int comparisonResult = 0;

	if (group1.relativeSortPosition == INT_MIN && group2.relativeSortPosition != INT_MIN)
	{
		comparisonResult = -1;
	}
	else if (group1.relativeSortPosition != INT_MIN && group2.relativeSortPosition == INT_MIN)
	{
		comparisonResult = 1;
	}
	else if (group1.relativeSortPosition == INT_MIN && group2.relativeSortPosition == INT_MIN)
	{
		comparisonResult = 0;
	}
	else if (group1.relativeSortPosition == INT_MAX && group2.relativeSortPosition != INT_MAX)
	{
		comparisonResult = 1;
	}
	else if (group1.relativeSortPosition != INT_MAX && group2.relativeSortPosition == INT_MAX)
	{
		comparisonResult = -1;
	}
	else if (group1.relativeSortPosition == INT_MAX && group2.relativeSortPosition == INT_MAX)
	{
		comparisonResult = 0;
	}
	else
	{
		switch (m_folderSettings.groupMode)
		{
		case SortMode::Size:
		case SortMode::TotalSize:
		case SortMode::Created:
		case SortMode::DateModified:
		case SortMode::Accessed:
			comparisonResult = GroupRelativePositionComparison(group1, group2);
			break;

		default:
			comparisonResult = GroupNameComparison(group1, group2);
			break;
		}
	}

	if (comparisonResult == 0)
	{
		comparisonResult = GroupNameComparison(group1, group2);
	}

	if (m_folderSettings.groupSortDirection == +SortDirection::Descending)
	{
		comparisonResult = -comparisonResult;
	}

	return comparisonResult;
}

int ShellBrowserImpl::GroupNameComparison(const ListViewGroup &group1, const ListViewGroup &group2)
{
	return group1.name.compare(group2.name);
}

int ShellBrowserImpl::GroupRelativePositionComparison(const ListViewGroup &group1,
	const ListViewGroup &group2)
{
	return group1.relativeSortPosition - group2.relativeSortPosition;
}

const ShellBrowserImpl::ListViewGroup ShellBrowserImpl::GetListViewGroupById(int groupId)
{
	auto itr = m_directoryState.groups.get<0>().find(groupId);
	CHECK(itr != m_directoryState.groups.get<0>().end());
	return *itr;
}

int ShellBrowserImpl::DetermineItemGroup(int iItemInternal)
{
	BasicItemInfo_t basicItemInfo = getBasicItemInfo(iItemInternal);
	std::optional<GroupInfo> groupInfo;

	switch (m_folderSettings.groupMode)
	{
	case SortMode::Name:
		groupInfo = DetermineItemNameGroup(basicItemInfo);
		break;

	case SortMode::Type:
		groupInfo = DetermineItemTypeGroupVirtual(basicItemInfo);
		break;

	case SortMode::Size:
		groupInfo = DetermineItemSizeGroup(basicItemInfo);
		break;

	case SortMode::DateModified:
		groupInfo = DetermineItemDateGroup(basicItemInfo, GroupByDateType::Modified);
		break;

	case SortMode::TotalSize:
		groupInfo = DetermineItemTotalSizeGroup(basicItemInfo);
		break;

	case SortMode::FreeSpace:
		groupInfo = DetermineItemFreeSpaceGroup(basicItemInfo);
		break;

	case SortMode::DateDeleted:
		break;

	case SortMode::OriginalLocation:
		groupInfo = DetermineItemSummaryGroup(basicItemInfo, &SCID_ORIGINAL_LOCATION,
			m_config->globalFolderSettings);
		break;

	case SortMode::Attributes:
		groupInfo = DetermineItemAttributeGroup(basicItemInfo);
		break;

	case SortMode::ShortName:
		groupInfo = DetermineItemNameGroup(basicItemInfo);
		break;

	case SortMode::Owner:
		groupInfo = DetermineItemOwnerGroup(basicItemInfo);
		break;

	case SortMode::ProductName:
		groupInfo = DetermineItemVersionGroup(basicItemInfo, _T("ProductName"));
		break;

	case SortMode::Company:
		groupInfo = DetermineItemVersionGroup(basicItemInfo, _T("CompanyName"));
		break;

	case SortMode::Description:
		groupInfo = DetermineItemVersionGroup(basicItemInfo, _T("FileDescription"));
		break;

	case SortMode::FileVersion:
		groupInfo = DetermineItemVersionGroup(basicItemInfo, _T("FileVersion"));
		break;

	case SortMode::ProductVersion:
		groupInfo = DetermineItemVersionGroup(basicItemInfo, _T("ProductVersion"));
		break;

	case SortMode::ShortcutTo:
		break;

	case SortMode::HardLinks:
		break;

	case SortMode::Extension:
		groupInfo = DetermineItemExtensionGroup(basicItemInfo);
		break;

	case SortMode::Created:
		groupInfo = DetermineItemDateGroup(basicItemInfo, GroupByDateType::Created);
		break;

	case SortMode::Accessed:
		groupInfo = DetermineItemDateGroup(basicItemInfo, GroupByDateType::Accessed);
		break;

	case SortMode::Title:
		groupInfo =
			DetermineItemSummaryGroup(basicItemInfo, &PKEY_Title, m_config->globalFolderSettings);
		break;

	case SortMode::Subject:
		groupInfo =
			DetermineItemSummaryGroup(basicItemInfo, &PKEY_Subject, m_config->globalFolderSettings);
		break;

	case SortMode::Authors:
		groupInfo =
			DetermineItemSummaryGroup(basicItemInfo, &PKEY_Author, m_config->globalFolderSettings);
		break;

	case SortMode::Keywords:
		groupInfo = DetermineItemSummaryGroup(basicItemInfo, &PKEY_Keywords,
			m_config->globalFolderSettings);
		break;

	case SortMode::Comments:
		groupInfo =
			DetermineItemSummaryGroup(basicItemInfo, &PKEY_Comment, m_config->globalFolderSettings);
		break;

	case SortMode::CameraModel:
		groupInfo = DetermineItemCameraPropertyGroup(basicItemInfo, PropertyTagEquipModel);
		break;

	case SortMode::DateTaken:
		groupInfo = DetermineItemCameraPropertyGroup(basicItemInfo, PropertyTagDateTime);
		break;

	case SortMode::Width:
		groupInfo = DetermineItemCameraPropertyGroup(basicItemInfo, PropertyTagImageWidth);
		break;

	case SortMode::Height:
		groupInfo = DetermineItemCameraPropertyGroup(basicItemInfo, PropertyTagImageHeight);
		break;

	case SortMode::VirtualComments:
		break;

	case SortMode::FileSystem:
		groupInfo = DetermineItemFileSystemGroup(basicItemInfo);
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
		groupInfo = DetermineItemNetworkStatus(basicItemInfo);
		break;

	default:
		assert(false);
		break;
	}

	if (!groupInfo)
	{
		groupInfo =
			GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_UNSPECIFIED), INT_MIN);
	}

	return GetOrCreateListViewGroup(*groupInfo);
}

int ShellBrowserImpl::GetOrCreateListViewGroup(const GroupInfo &groupInfo)
{
	// Note that this will return an existing group, if a group already exists with the specified
	// name.
	auto &groupNameIndex = m_directoryState.groups.get<1>();
	auto [group, inserted] = groupNameIndex.emplace(groupInfo);
	return group->id;
}

/* TODO: These groups have changed as of Windows Vista.*/
std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemNameGroup(
	const BasicItemInfo_t &itemInfo) const
{
	/* Take the first character of the item's name,
	and use it to determine which group it belongs to. */
	TCHAR ch = itemInfo.szDisplayName[0];

	if (iswalpha(ch))
	{
		return GroupInfo(std::wstring(1, towupper(ch)));
	}
	else
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_NAME_OTHER), INT_MAX);
	}
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemSizeGroup(
	const BasicItemInfo_t &itemInfo) const
{
	if ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_SIZE_FOLDERS), 0);
	}
	else if (!itemInfo.isFindDataValid)
	{
		return std::nullopt;
	}

	struct SizeGroup
	{
		int nameResourceId;
		uint64_t upperLimit;
	};

	// If the limits here are adjusted, the entries in the string table should be updated as well
	// (since they reference the limits as well).
	SizeGroup sizeGroups[] = { { IDS_GROUPBY_SIZE_EMPTY, 0 }, { IDS_GROUPBY_SIZE_TINY, 16 * KBYTE },
		{ IDS_GROUPBY_SIZE_SMALL, MBYTE }, { IDS_GROUPBY_SIZE_MEDIUM, 128 * MBYTE },
		{ IDS_GROUPBY_SIZE_LARGE, GBYTE }, { IDS_GROUPBY_SIZE_HUGE, 4 * GBYTE },
		{ IDS_GROUPBY_SIZE_GIGANTIC, boost::integer_traits<uint64_t>::const_max } };

	ULARGE_INTEGER fileSize = { { itemInfo.wfd.nFileSizeLow, itemInfo.wfd.nFileSizeHigh } };
	size_t currentIndex = 0;

	while (fileSize.QuadPart > sizeGroups[currentIndex].upperLimit
		&& currentIndex < (std::size(sizeGroups) - 1))
	{
		currentIndex++;
	}

	return GroupInfo(
		m_app->GetResourceLoader()->LoadString(sizeGroups[currentIndex].nameResourceId),
		static_cast<int>(currentIndex + 1));
}

/* TODO: These groups have changed as of Windows Vista. */
std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemTotalSizeGroup(
	const BasicItemInfo_t &itemInfo) const
{
	IShellFolder *pShellFolder = nullptr;
	PCITEMID_CHILD pidlRelative = nullptr;
	const TCHAR *sizeGroups[] = { _T("Small"), _T("Medium"), _T("Huge"), _T("Gigantic") };
	TCHAR szItem[MAX_PATH];
	STRRET str;
	ULARGE_INTEGER nTotalBytes;
	ULARGE_INTEGER nFreeBytes;
	BOOL bRoot;
	BOOL bRes = FALSE;
	ULARGE_INTEGER totalSizeGroupLimits[6];
	int iSize = 0;
	int i;

	totalSizeGroupLimits[0].QuadPart = 0;
	totalSizeGroupLimits[1].QuadPart = 0;
	totalSizeGroupLimits[2].QuadPart = GBYTE;
	totalSizeGroupLimits[3].QuadPart = 20 * totalSizeGroupLimits[2].QuadPart;
	totalSizeGroupLimits[4].QuadPart = 100 * totalSizeGroupLimits[2].QuadPart;

	SHBindToParent(itemInfo.pidlComplete.get(), IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative, SHGDN_FORPARSING, &str);
	StrRetToBuf(&str, pidlRelative, szItem, std::size(szItem));

	bRoot = PathIsRoot(szItem);

	if (bRoot)
	{
		bRes = GetDiskFreeSpaceEx(szItem, nullptr, &nTotalBytes, &nFreeBytes);

		pShellFolder->Release();

		i = std::size(sizeGroups) - 1;

		while (nTotalBytes.QuadPart < totalSizeGroupLimits[i].QuadPart && i > 0)
		{
			i--;
		}

		iSize = i;
	}

	if (!bRoot || !bRes)
	{
		return std::nullopt;
	}

	return GroupInfo(sizeGroups[iSize], iSize);
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemTypeGroupVirtual(
	const BasicItemInfo_t &itemInfo) const
{
	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo((LPTSTR) itemInfo.pidlComplete.get(), 0, &shfi, sizeof(shfi),
		SHGFI_PIDL | SHGFI_TYPENAME);

	if (!res)
	{
		return std::nullopt;
	}

	return GroupInfo(shfi.szTypeName);
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemDateGroup(
	const BasicItemInfo_t &itemInfo, GroupByDateType dateType) const
{
	if (!itemInfo.isFindDataValid)
	{
		return std::nullopt;
	}

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
		LOG(FATAL) << "Incorrect date type";
	}

	if (!ret)
	{
		return std::nullopt;
	}

	FILETIME localFileTime;
	ret = SystemTimeToFileTime(&stFileTime, &localFileTime);

	if (!ret)
	{
		return std::nullopt;
	}

	auto filePosixTime = from_ftime<ptime>(localFileTime);
	date fileDate = filePosixTime.date();

	date today = day_clock::local_day();
	int relativeSortPosition = 0;

	if (fileDate > today)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_FUTURE),
			relativeSortPosition);
	}

	relativeSortPosition--;

	if (fileDate == today)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_TODAY),
			relativeSortPosition);
	}

	date yesterday = today - days(1);

	relativeSortPosition--;

	if (fileDate == yesterday)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_YESTERDAY),
			relativeSortPosition);
	}

	// Note that this assumes that Sunday is the first day of the week.
	unsigned short currentWeekday = today.day_of_week().as_number();
	date startOfWeek = today - days(currentWeekday);

	relativeSortPosition--;

	if (fileDate >= startOfWeek)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_THIS_WEEK),
			relativeSortPosition);
	}

	date startOfLastWeek = startOfWeek - weeks(1);

	relativeSortPosition--;

	if (fileDate >= startOfLastWeek)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_LAST_WEEK),
			relativeSortPosition);
	}

	date startOfMonth = date(today.year(), today.month(), 1);

	relativeSortPosition--;

	if (fileDate >= startOfMonth)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_THIS_MONTH),
			relativeSortPosition);
	}

	date startOfLastMonth = startOfMonth - months(1);

	relativeSortPosition--;

	if (fileDate >= startOfLastMonth)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_LAST_MONTH),
			relativeSortPosition);
	}

	date startOfYear = date(today.year(), 1, 1);

	relativeSortPosition--;

	if (fileDate >= startOfYear)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_THIS_YEAR),
			relativeSortPosition);
	}

	date startOfLastYear = startOfYear - years(1);

	relativeSortPosition--;

	if (fileDate >= startOfLastYear)
	{
		return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_LAST_YEAR),
			relativeSortPosition);
	}

	relativeSortPosition--;

	return GroupInfo(m_app->GetResourceLoader()->LoadString(IDS_GROUPBY_DATE_LONG_AGO),
		relativeSortPosition);
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemSummaryGroup(
	const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid,
	const GlobalFolderSettings &globalFolderSettings) const
{
	TCHAR szDetail[512];
	HRESULT hr =
		GetItemDetails(itemInfo, pscid, szDetail, std::size(szDetail), globalFolderSettings);

	if (SUCCEEDED(hr) && lstrlen(szDetail) > 0)
	{
		return GroupInfo(szDetail);
	}
	else
	{
		return std::nullopt;
	}
}

/* TODO: Need to sort based on percentage free. */
std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemFreeSpaceGroup(
	const BasicItemInfo_t &itemInfo) const
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
	StrRetToBuf(&str, pidlRelative, szItem, std::size(szItem));

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
		StringCchPrintf(szFreeSpace, std::size(szFreeSpace), _T("%I64d%% free"),
			(((nFreeBytes.QuadPart * lDiv1.QuadPart) / nTotalBytes.QuadPart) / lDiv2.QuadPart)
				* lDiv2.QuadPart);
	}

	if (!bRoot || !bRes)
	{
		return std::nullopt;
	}

	return GroupInfo(szFreeSpace);
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemAttributeGroup(
	const BasicItemInfo_t &itemInfo) const
{
	if (!itemInfo.isFindDataValid)
	{
		return std::nullopt;
	}

	auto attributesString = BuildFileAttributesString(itemInfo.wfd.dwFileAttributes);
	return GroupInfo(attributesString);
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemOwnerGroup(
	const BasicItemInfo_t &itemInfo) const
{
	std::wstring fullFileName = itemInfo.getFullPath();

	TCHAR szOwner[512];
	BOOL ret = GetFileOwner(fullFileName.c_str(), szOwner, std::size(szOwner));

	if (!ret)
	{
		return std::nullopt;
	}

	return GroupInfo(szOwner);
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemVersionGroup(
	const BasicItemInfo_t &itemInfo, const TCHAR *szVersionType) const
{
	std::wstring fullFileName = itemInfo.getFullPath();

	TCHAR szVersion[512];
	BOOL bVersionInfoObtained = GetVersionInfoString(fullFileName.c_str(), szVersionType, szVersion,
		static_cast<UINT>(std::size(szVersion)));

	if (!bVersionInfoObtained)
	{
		return std::nullopt;
	}

	return GroupInfo(szVersion);
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemCameraPropertyGroup(
	const BasicItemInfo_t &itemInfo, PROPID PropertyId) const
{
	std::wstring fullFileName = itemInfo.getFullPath();

	TCHAR szProperty[512];
	BOOL bRes = ReadImageProperty(fullFileName.c_str(), PropertyId, szProperty,
		static_cast<int>(std::size(szProperty)));

	if (!bRes)
	{
		return std::nullopt;
	}

	return GroupInfo(szProperty);
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemExtensionGroup(
	const BasicItemInfo_t &itemInfo) const
{
	if (WI_IsFlagSet(itemInfo.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		return std::nullopt;
	}

	std::wstring fullFileName = itemInfo.getFullPath();
	TCHAR *pExt = PathFindExtension(fullFileName.c_str());

	if (*pExt == '\0')
	{
		return std::nullopt;
	}

	return GroupInfo(pExt);
}

std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemFileSystemGroup(
	const BasicItemInfo_t &itemInfo) const
{
	std::wstring fullPath = itemInfo.getFullPath();
	BOOL isRoot = PathIsRoot(fullPath.c_str());

	if (!isRoot)
	{
		return std::nullopt;
	}

	TCHAR fileSystemName[MAX_PATH];
	BOOL res = GetVolumeInformation(fullPath.c_str(), nullptr, 0, nullptr, nullptr, nullptr,
		fileSystemName, std::size(fileSystemName));

	if (!res)
	{
		return std::nullopt;
	}

	return GroupInfo(fileSystemName);
}

/* TODO: Fix. Need to check for each adapter. */
std::optional<ShellBrowserImpl::GroupInfo> ShellBrowserImpl::DetermineItemNetworkStatus(
	const BasicItemInfo_t &itemInfo) const
{
	/* When this function is
	properly implemented, this
	can be removed. */
	UNREFERENCED_PARAMETER(itemInfo);

	TCHAR szStatus[32] = L"";
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

	LoadString(m_resourceInstance, uStatusID, szStatus, std::size(szStatus));

	return GroupInfo(szStatus);
}

void ShellBrowserImpl::MoveItemsIntoGroups()
{
	LVITEM item;
	int nItems;
	int iGroupId;
	int i = 0;

	ScopedRedrawDisabler redrawDisabler(m_listView);

	ListView_RemoveAllGroups(m_listView);
	m_directoryState.groups.clear();

	ListView_EnableGroupView(m_listView, true);

	nItems = ListView_GetItemCount(m_listView);

	for (i = 0; i < nItems; i++)
	{
		item.mask = LVIF_PARAM;
		item.iItem = i;
		item.iSubItem = 0;
		ListView_GetItem(m_listView, &item);

		iGroupId = DetermineItemGroup((int) item.lParam);

		InsertItemIntoGroup(i, iGroupId);
	}
}

void ShellBrowserImpl::InsertItemIntoGroup(int index, int groupId)
{
	auto previousGroupId = GetItemGroupId(index);

	if (previousGroupId && *previousGroupId == groupId)
	{
		return;
	}

	EnsureGroupExistsInListView(groupId);

	LVITEM item;
	item.mask = LVIF_GROUPID;
	item.iItem = index;
	item.iSubItem = 0;
	item.iGroupId = groupId;
	BOOL res = ListView_SetItem(m_listView, &item);

	if (res)
	{
		if (previousGroupId)
		{
			OnItemRemovedFromGroup(*previousGroupId);
		}

		OnItemAddedToGroup(groupId);
	}
}

void ShellBrowserImpl::EnsureGroupExistsInListView(int groupId)
{
	ListViewGroup group = GetListViewGroupById(groupId);

	if (group.numItems == 0)
	{
		InsertGroupIntoListView(group);
	}
}

void ShellBrowserImpl::InsertGroupIntoListView(const ListViewGroup &listViewGroup)
{
	std::wstring header = GenerateGroupHeader(listViewGroup);

	LVINSERTGROUPSORTED lvigs;
	lvigs.lvGroup.cbSize = sizeof(LVGROUP);
	lvigs.lvGroup.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	lvigs.lvGroup.state = LVGS_COLLAPSIBLE;
	lvigs.lvGroup.pszHeader = header.data();
	lvigs.lvGroup.iGroupId = listViewGroup.id;
	lvigs.lvGroup.stateMask = 0;
	lvigs.pfnGroupCompare = GroupComparisonStub;
	lvigs.pvData = this;
	ListView_InsertGroupSorted(m_listView, &lvigs);
}

void ShellBrowserImpl::RemoveGroupFromListView(const ListViewGroup &listViewGroup)
{
	ListView_RemoveGroup(m_listView, listViewGroup.id);
}

void ShellBrowserImpl::UpdateGroupHeader(const ListViewGroup &listViewGroup)
{
	std::wstring header = GenerateGroupHeader(listViewGroup);

	LVGROUP lvGroup;
	lvGroup.cbSize = sizeof(LVGROUP);
	lvGroup.mask = LVGF_HEADER;
	lvGroup.pszHeader = header.data();
	ListView_SetGroupInfo(m_listView, listViewGroup.id, &lvGroup);
}

std::wstring ShellBrowserImpl::GenerateGroupHeader(const ListViewGroup &listViewGroup)
{
	return std::format(L"{} ({})", listViewGroup.name, listViewGroup.numItems);
}

void ShellBrowserImpl::OnItemRemovedFromGroup(int groupId)
{
	auto &groupIdIndex = m_directoryState.groups.get<0>();
	auto itr = groupIdIndex.find(groupId);
	CHECK(itr != groupIdIndex.end());

	auto updatedGroup = *itr;
	updatedGroup.numItems--;
	m_directoryState.groups.replace(itr, updatedGroup);

	if (updatedGroup.numItems == 0)
	{
		RemoveGroupFromListView(updatedGroup);
	}
	else
	{
		UpdateGroupHeader(updatedGroup);
	}
}

void ShellBrowserImpl::OnItemAddedToGroup(int groupId)
{
	auto &groupIdIndex = m_directoryState.groups.get<0>();
	auto itr = groupIdIndex.find(groupId);
	CHECK(itr != groupIdIndex.end());

	auto updatedGroup = *itr;
	updatedGroup.numItems++;
	m_directoryState.groups.replace(itr, updatedGroup);

	UpdateGroupHeader(updatedGroup);
}

std::optional<int> ShellBrowserImpl::GetItemGroupId(int index)
{
	LVITEM item;
	item.mask = LVIF_GROUPID;
	item.iItem = index;
	item.iSubItem = 0;
	BOOL res = ListView_GetItem(m_listView, &item);

	if (!res || item.iGroupId == I_GROUPIDNONE)
	{
		return std::nullopt;
	}

	return item.iGroupId;
}
