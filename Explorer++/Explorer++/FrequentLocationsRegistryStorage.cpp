// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FrequentLocationsRegistryStorage.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsStorageHelper.h"
#include "LocationVisitInfo.h"
#include "../Helper/RegistrySettings.h"
#include <wil/registry.h>
#include <optional>
#include <vector>

namespace
{

constexpr wchar_t FREQUENT_LOCATIONS_KEY_PATH[] = L"FrequentLocations";

constexpr wchar_t SETTING_LOCATION[] = L"Location";
constexpr wchar_t SETTING_NUM_VISITS[] = L"NumVisits";
constexpr wchar_t SETTING_LAST_VISIT_TIME[] = L"LastVisitTime";

std::optional<LocationVisitInfo> LoadFrequentLocation(HKEY frequentLocationsKey)
{
	PidlAbsolute pidl;
	auto res = RegistrySettings::ReadPidl(frequentLocationsKey, SETTING_LOCATION, pidl);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	int numVisits;
	res = RegistrySettings::Read32BitValueFromRegistry(frequentLocationsKey, SETTING_NUM_VISITS,
		numVisits);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	FrequentLocationsStorageHelper::StorageDurationType::rep timeSinceEpoch;
	res = RegistrySettings::Read64BitValueFromRegistry(frequentLocationsKey,
		SETTING_LAST_VISIT_TIME, timeSinceEpoch);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	return LocationVisitInfo{ pidl, numVisits,
		LocationVisitInfo::Clock::time_point(
			FrequentLocationsStorageHelper::StorageDurationType(timeSinceEpoch)) };
};

void LoadFromKey(HKEY frequentLocationsKey, FrequentLocationsModel *model)
{
	std::vector<LocationVisitInfo> frequentLocations;
	wil::unique_hkey childKey;
	int index = 0;

	while (SUCCEEDED(wil::reg::open_unique_key_nothrow(frequentLocationsKey,
		std::to_wstring(index).c_str(), childKey)))
	{
		auto frequentLocation = LoadFrequentLocation(childKey.get());

		if (frequentLocation)
		{
			frequentLocations.push_back(*frequentLocation);
		}

		index++;
	}

	model->SetLocationVisits(frequentLocations);
}

void SaveFrequentLocation(HKEY frequentLocationKey, const LocationVisitInfo &frequentLocation)
{
	RegistrySettings::SavePidl(frequentLocationKey, SETTING_LOCATION,
		frequentLocation.GetLocation().Raw());
	RegistrySettings::SaveDword(frequentLocationKey, SETTING_NUM_VISITS,
		frequentLocation.GetNumVisits());
	RegistrySettings::SaveQword(frequentLocationKey, SETTING_LAST_VISIT_TIME,
		std::chrono::duration_cast<FrequentLocationsStorageHelper::StorageDurationType>(
			frequentLocation.GetLastVisitTime().time_since_epoch())
			.count());
}

void SaveToKey(HKEY frequentLocationsKey, const FrequentLocationsModel *model)
{
	size_t index = 0;

	for (const auto &frequentLocation :
		model->GetVisits() | std::views::take(FrequentLocationsStorageHelper::MAX_ITEMS_TO_STORE))
	{
		wil::unique_hkey childKey;
		HRESULT hr = wil::reg::create_unique_key_nothrow(frequentLocationsKey,
			std::to_wstring(index).c_str(), childKey, wil::reg::key_access::readwrite);

		if (SUCCEEDED(hr))
		{
			SaveFrequentLocation(childKey.get(), frequentLocation);

			index++;
		}
	}
}

}

namespace FrequentLocationsRegistryStorage
{

void Load(HKEY applicationKey, FrequentLocationsModel *model)
{
	wil::unique_hkey frequentLocationsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(applicationKey, FREQUENT_LOCATIONS_KEY_PATH,
		frequentLocationsKey, wil::reg::key_access::read);

	if (FAILED(hr))
	{
		return;
	}

	LoadFromKey(frequentLocationsKey.get(), model);
}

void Save(HKEY applicationKey, const FrequentLocationsModel *model)
{
	wil::unique_hkey frequentLocationsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(applicationKey, FREQUENT_LOCATIONS_KEY_PATH,
		frequentLocationsKey, wil::reg::key_access::readwrite);

	if (FAILED(hr))
	{
		return;
	}

	SaveToKey(frequentLocationsKey.get(), model);
}

}
