// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "XmlAppStorageFactory.h"
#include "Storage.h"
#include "XmlAppStorage.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/XMLSettings.h"

std::unique_ptr<XmlAppStorage> XmlAppStorageFactory::MaybeCreate()
{
	auto xmlDocument = XMLSettings::CreateXmlDocument();

	if (!xmlDocument)
	{
		return nullptr;
	}

	wchar_t currentProcessPath[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), currentProcessPath,
		SIZEOF_ARRAY(currentProcessPath));

	std::filesystem::path configFilePath(currentProcessPath);
	configFilePath.replace_filename(Storage::CONFIG_FILE_FILENAME);

	auto configFilePathVariant = wil::make_variant_bstr_failfast(configFilePath.c_str());
	VARIANT_BOOL status;
	xmlDocument->load(configFilePathVariant, &status);

	if (status != VARIANT_TRUE)
	{
		return nullptr;
	}

	return std::make_unique<XmlAppStorage>(xmlDocument);
}
