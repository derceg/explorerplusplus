// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FrequentLocationsXmlStorage.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsStorageHelper.h"
#include "LocationVisitInfo.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/StringHelper.h"
#include "../Helper/XMLSettings.h"
#include <boost/lexical_cast.hpp>
#include <wil/com.h>
#include <optional>
#include <vector>

namespace
{

constexpr wchar_t FREQUENT_LOCATIONS_NODE_NAME[] = L"FrequentLocations";
constexpr wchar_t FREQUENT_LOCATION_NODE_NAME[] = L"FrequentLocation";

constexpr wchar_t SETTING_LOCATION[] = L"Location";
constexpr wchar_t SETTING_NUM_VISITS[] = L"NumVisits";
constexpr wchar_t SETTING_LAST_VISIT_TIME[] = L"LastVisitTime";

std::optional<LocationVisitInfo> LoadFrequentLocation(IXMLDOMNode *frequentLocationNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = frequentLocationNode->get_attributes(&attributeMap);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	std::wstring encodedPidl;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), SETTING_LOCATION, encodedPidl);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	auto encodedPidlNarrow = WstrToStr(encodedPidl);

	if (!encodedPidlNarrow)
	{
		return std::nullopt;
	}

	auto pidl = DecodePidlFromBase64(*encodedPidlNarrow);

	if (!pidl.HasValue())
	{
		return std::nullopt;
	}

	int numVisits;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_NUM_VISITS, numVisits);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	std::wstring timeSinceEpochText;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), SETTING_LAST_VISIT_TIME,
		timeSinceEpochText);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	FrequentLocationsStorageHelper::StorageDurationType::rep timeSinceEpoch;

	try
	{
		timeSinceEpoch =
			boost::lexical_cast<FrequentLocationsStorageHelper::StorageDurationType::rep>(
				timeSinceEpochText);
	}
	catch (const boost::bad_lexical_cast &)
	{
		return std::nullopt;
	}

	return LocationVisitInfo{ pidl, numVisits,
		SystemClock::TimePoint(
			FrequentLocationsStorageHelper::StorageDurationType(timeSinceEpoch)) };
}

void LoadFromNode(IXMLDOMNode *frequentLocationsNode, FrequentLocationsModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNodeList> frequentLocationNodes;
	auto queryString = wil::make_bstr_nothrow(FREQUENT_LOCATION_NODE_NAME);
	HRESULT hr = frequentLocationsNode->selectNodes(queryString.get(), &frequentLocationNodes);

	if (hr != S_OK)
	{
		return;
	}

	std::vector<LocationVisitInfo> frequentLocations;
	wil::com_ptr_nothrow<IXMLDOMNode> childNode;

	while (frequentLocationNodes->nextNode(&childNode) == S_OK)
	{
		auto frequentLocation = LoadFrequentLocation(childNode.get());

		if (frequentLocation)
		{
			frequentLocations.push_back(*frequentLocation);
		}
	}

	model->SetLocationVisits(frequentLocations);
}

void SaveFrequentLocation(IXMLDOMDocument *xmlDocument, IXMLDOMElement *frequentLocationNode,
	const LocationVisitInfo &frequentLocation)
{
	auto encodedPidl = EncodePidlToBase64(frequentLocation.GetLocation().Raw());
	auto encodedPidlWide = StrToWstr(encodedPidl);

	if (!encodedPidlWide)
	{
		return;
	}

	XMLSettings::AddAttributeToNode(xmlDocument, frequentLocationNode, SETTING_LOCATION,
		*encodedPidlWide);
	XMLSettings::AddAttributeToNode(xmlDocument, frequentLocationNode, SETTING_NUM_VISITS,
		XMLSettings::EncodeIntValue(frequentLocation.GetNumVisits()));
	XMLSettings::AddAttributeToNode(xmlDocument, frequentLocationNode, SETTING_LAST_VISIT_TIME,
		std::to_wstring(
			std::chrono::duration_cast<FrequentLocationsStorageHelper::StorageDurationType>(
				frequentLocation.GetLastVisitTime().time_since_epoch())
				.count()));
}

void SaveToNode(IXMLDOMDocument *xmlDocument, IXMLDOMElement *frequentLocationsNode,
	const FrequentLocationsModel *model)
{
	for (const auto &frequentLocation :
		model->GetVisits() | std::views::take(FrequentLocationsStorageHelper::MAX_ITEMS_TO_STORE))
	{
		wil::com_ptr_nothrow<IXMLDOMElement> frequentLocationNode;
		auto frequentLocationNodeName = wil::make_bstr_nothrow(FREQUENT_LOCATION_NODE_NAME);
		HRESULT hr =
			xmlDocument->createElement(frequentLocationNodeName.get(), &frequentLocationNode);

		if (hr == S_OK)
		{
			SaveFrequentLocation(xmlDocument, frequentLocationNode.get(), frequentLocation);
			XMLSettings::AppendChildToParent(frequentLocationNode.get(), frequentLocationsNode);
		}
	}
}

}

namespace FrequentLocationsXmlStorage
{

void Load(IXMLDOMNode *rootNode, FrequentLocationsModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNode> frequentLocationsNode;
	auto queryString = wil::make_bstr_nothrow(FREQUENT_LOCATIONS_NODE_NAME);
	HRESULT hr = rootNode->selectSingleNode(queryString.get(), &frequentLocationsNode);

	if (hr != S_OK)
	{
		return;
	}

	LoadFromNode(frequentLocationsNode.get(), model);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const FrequentLocationsModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMElement> frequentLocationsNode;
	auto nodeName = wil::make_bstr_nothrow(FREQUENT_LOCATIONS_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(nodeName.get(), &frequentLocationsNode);

	if (hr != S_OK)
	{
		return;
	}

	SaveToNode(xmlDocument, frequentLocationsNode.get(), model);

	XMLSettings::AppendChildToParent(frequentLocationsNode.get(), rootNode);
}

}
