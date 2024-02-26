// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColumnXmlStorage.h"
#include "ShellBrowser/FolderSettings.h"
#include "../Helper/Helper.h"
#include "../Helper/XMLSettings.h"
#include <boost/bimap.hpp>

namespace ColumnXmlStorage
{

namespace
{

const WCHAR REAL_FOLDER_COLUMNS_NODE_NAME[] = L"Generic";
const WCHAR MY_COMPUTER_COLUMNS_NODE_NAME[] = L"MyComputer";
const WCHAR CONTROL_PANEL_COLUMNS_NODE_NAME[] = L"ControlPanel";
const WCHAR RECYCLE_BIN_COLUMNS_NODE_NAME[] = L"RecycleBin";
const WCHAR PRINTERS_COLUMNS_NODE_NAME[] = L"Printers";
const WCHAR NETWORK_COLUMNS_NODE_NAME[] = L"Network";
const WCHAR NETWORK_PLACES_COLUMNS_NODE_NAME[] = L"NetworkPlaces";

const WCHAR WIDTH_SUFFIX[] = L"_Width";

// These names are used when loading and saving columns and shouldn't be changed.
// clang-format off
const boost::bimap<ColumnType, std::wstring> COLUMN_TYPE_NAME_MAPPINGS = MakeBimap<ColumnType,
	std::wstring>({
	{ ColumnType::Name, L"Name" },
	{ ColumnType::Type, L"Type" },
	{ ColumnType::Size, L"Size" },
	{ ColumnType::DateModified, L"DateModified" },
	{ ColumnType::Attributes, L"Attributes" },
	{ ColumnType::RealSize, L"SizeOnDisk" },
	{ ColumnType::ShortName, L"ShortName" },
	{ ColumnType::Owner, L"Owner" },
	{ ColumnType::ProductName, L"ProductName" },
	{ ColumnType::Company, L"Company" },
	{ ColumnType::Description, L"Description" },
	{ ColumnType::FileVersion, L"FileVersion" },
	{ ColumnType::ProductVersion, L"ProductVersion" },
	{ ColumnType::ShortcutTo, L"ShortcutTo" },
	{ ColumnType::HardLinks, L"HardLinks" },
	{ ColumnType::Extension, L"Extension" },
	{ ColumnType::Created, L"Created" },
	{ ColumnType::Accessed, L"Accessed" },
	{ ColumnType::Title, L"Title" },
	{ ColumnType::Subject, L"Subject" },
	{ ColumnType::Authors, L"Author" },
	{ ColumnType::Keywords, L"Keywords" },
	{ ColumnType::Comment, L"Comment" },
	{ ColumnType::CameraModel, L"CameraModel" },
	{ ColumnType::DateTaken, L"DateTaken" },
	{ ColumnType::Width, L"Width" },
	{ ColumnType::Height, L"Height" },
	{ ColumnType::VirtualComments, L"VirtualComments" },
	{ ColumnType::TotalSize, L"TotalSize" },
	{ ColumnType::FreeSpace, L"FreeSpace" },
	{ ColumnType::FileSystem, L"FileSystem" },
	{ ColumnType::OriginalLocation, L"OriginalLocation" },
	{ ColumnType::DateDeleted, L"DateDeleted" },
	{ ColumnType::PrinterNumDocuments, L"Documents" },
	{ ColumnType::PrinterStatus, L"Status" },
	{ ColumnType::PrinterComments, L"PrinterComments" },
	{ ColumnType::PrinterLocation, L"PrinterLocation" },
	{ ColumnType::NetworkAdaptorStatus, L"NetworkAdaptorStatus" },
	{ ColumnType::MediaBitrate, L"MediaBitrate" },
	{ ColumnType::MediaCopyright, L"MediaCopyright" },
	{ ColumnType::MediaDuration, L"MediaDuration" },
	{ ColumnType::MediaProtected, L"MediaProtected" },
	{ ColumnType::MediaRating, L"MediaRating" },
	{ ColumnType::MediaAlbumArtist, L"MediaAlbumArtist" },
	{ ColumnType::MediaAlbum, L"MediaAlbum" },
	{ ColumnType::MediaBeatsPerMinute, L"MediaBeatsPerMinute" },
	{ ColumnType::MediaComposer, L"MediaComposer" },
	{ ColumnType::MediaConductor, L"MediaConductor" },
	{ ColumnType::MediaDirector, L"MediaDirector" },
	{ ColumnType::MediaGenre, L"MediaGenre" },
	{ ColumnType::MediaLanguage, L"MediaLanguage" },
	{ ColumnType::MediaBroadcastDate, L"MediaBroadcastDate" },
	{ ColumnType::MediaChannel, L"MediaChannel" },
	{ ColumnType::MediaStationName, L"MediaStationName" },
	{ ColumnType::MediaMood, L"MediaMood" },
	{ ColumnType::MediaParentalRating, L"MediaParentalRating" },
	{ ColumnType::MediaParentalRatingReason, L"MediaParentalRatingReason" },
	{ ColumnType::MediaPeriod, L"MediaPeriod" },
	{ ColumnType::MediaProducer, L"MediaProducer" },
	{ ColumnType::MediaPublisher, L"MediaPublisher" },
	{ ColumnType::MediaWriter, L"MediaWriter" },
	{ ColumnType::MediaYear, L"MediaYear" },
	{ ColumnType::PrinterModel, L"PrinterModel" }
});
// clang-format on

void LoadColumnSetFromXml(IXMLDOMNode *parentNode, const std::wstring &columnSetName,
	std::vector<Column_t> &outputColumnSet)
{
	auto queryString =
		wil::make_bstr_nothrow((L"./Column[@name='" + columnSetName + L"']").c_str());

	wil::com_ptr_nothrow<IXMLDOMNode> matchingNode;
	HRESULT hr = parentNode->selectSingleNode(queryString.get(), &matchingNode);

	if (hr != S_OK)
	{
		return;
	}

	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	hr = matchingNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return;
	}

	std::vector<Column_t> columns;
	wil::com_ptr_nothrow<IXMLDOMNode> attributeNode;

	while (attributeMap->nextNode(&attributeNode) == S_OK)
	{
		wil::unique_bstr attributeName;
		hr = attributeNode->get_nodeName(&attributeName);

		if (FAILED(hr))
		{
			continue;
		}

		auto itr = COLUMN_TYPE_NAME_MAPPINGS.right.find(attributeName.get());

		if (itr == COLUMN_TYPE_NAME_MAPPINGS.right.end())
		{
			continue;
		}

		bool checked;
		hr = NXMLSettings::GetBoolFromMap(attributeMap.get(), itr->first, checked);

		if (FAILED(hr))
		{
			continue;
		}

		Column_t column;
		column.type = itr->second;
		column.checked = checked;
		column.width = DEFAULT_COLUMN_WIDTH;

		int width;
		hr = NXMLSettings::GetIntFromMap(attributeMap.get(), itr->first + WIDTH_SUFFIX, width);

		if (SUCCEEDED(hr))
		{
			column.width = width;
		}

		columns.push_back(column);
	}

	if (columns.empty())
	{
		return;
	}

	outputColumnSet = columns;
}

void SaveColumnSetToXml(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const std::vector<Column_t> &columnSet, const std::wstring &columnSetName)
{
	wil::com_ptr_nothrow<IXMLDOMElement> columnNode;
	NXMLSettings::CreateElementNode(xmlDocument, &columnNode, parentNode, _T("Column"),
		columnSetName.c_str());

	for (const auto &column : columnSet)
	{
		auto columnName = COLUMN_TYPE_NAME_MAPPINGS.left.at(column.type);
		NXMLSettings::AddAttributeToNode(xmlDocument, columnNode.get(), columnName.c_str(),
			NXMLSettings::EncodeBoolValue(column.checked));

		auto widthFieldName = columnName + WIDTH_SUFFIX;
		NXMLSettings::AddAttributeToNode(xmlDocument, columnNode.get(), widthFieldName.c_str(),
			NXMLSettings::EncodeIntValue(column.width));
	}
}

}

void LoadAllColumnSets(IXMLDOMNode *parentNode, FolderColumns &folderColumns)
{
	LoadColumnSetFromXml(parentNode, REAL_FOLDER_COLUMNS_NODE_NAME,
		folderColumns.realFolderColumns);
	LoadColumnSetFromXml(parentNode, MY_COMPUTER_COLUMNS_NODE_NAME,
		folderColumns.myComputerColumns);
	LoadColumnSetFromXml(parentNode, CONTROL_PANEL_COLUMNS_NODE_NAME,
		folderColumns.controlPanelColumns);
	LoadColumnSetFromXml(parentNode, RECYCLE_BIN_COLUMNS_NODE_NAME,
		folderColumns.recycleBinColumns);
	LoadColumnSetFromXml(parentNode, PRINTERS_COLUMNS_NODE_NAME, folderColumns.printersColumns);
	LoadColumnSetFromXml(parentNode, NETWORK_COLUMNS_NODE_NAME,
		folderColumns.networkConnectionsColumns);
	LoadColumnSetFromXml(parentNode, NETWORK_PLACES_COLUMNS_NODE_NAME,
		folderColumns.myNetworkPlacesColumns);
}

void SaveAllColumnSets(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const FolderColumns &folderColumns)
{
	SaveColumnSetToXml(xmlDocument, parentNode, folderColumns.realFolderColumns,
		REAL_FOLDER_COLUMNS_NODE_NAME);
	SaveColumnSetToXml(xmlDocument, parentNode, folderColumns.myComputerColumns,
		MY_COMPUTER_COLUMNS_NODE_NAME);
	SaveColumnSetToXml(xmlDocument, parentNode, folderColumns.controlPanelColumns,
		CONTROL_PANEL_COLUMNS_NODE_NAME);
	SaveColumnSetToXml(xmlDocument, parentNode, folderColumns.recycleBinColumns,
		RECYCLE_BIN_COLUMNS_NODE_NAME);
	SaveColumnSetToXml(xmlDocument, parentNode, folderColumns.printersColumns,
		PRINTERS_COLUMNS_NODE_NAME);
	SaveColumnSetToXml(xmlDocument, parentNode, folderColumns.networkConnectionsColumns,
		NETWORK_COLUMNS_NODE_NAME);
	SaveColumnSetToXml(xmlDocument, parentNode, folderColumns.myNetworkPlacesColumns,
		NETWORK_PLACES_COLUMNS_NODE_NAME);
}

}
