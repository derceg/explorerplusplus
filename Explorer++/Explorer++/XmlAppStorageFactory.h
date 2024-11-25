// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Storage.h"
#include <memory>

class XmlAppStorage;

class XmlAppStorageFactory
{
public:
	static std::unique_ptr<XmlAppStorage> MaybeCreate(const std::wstring &configFilePath,
		Storage::OperationType operationType);

private:
	static std::unique_ptr<XmlAppStorage> BuildForLoad(const std::wstring &configFilePath);
	static std::unique_ptr<XmlAppStorage> BuildForSave(const std::wstring &configFilePath);
};
