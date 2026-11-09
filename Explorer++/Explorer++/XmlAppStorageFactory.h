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
	// If the primary cannot be read, return a valid backup for recovery. error keeps
	// the primary failure so the caller can protect both files from automatic saves.
	static std::unique_ptr<XmlAppStorage> LoadWithRecovery(const std::wstring &configFilePath,
		HRESULT &error);
	static std::unique_ptr<XmlAppStorage> MaybeCreate(const std::wstring &configFilePath,
		Storage::OperationType operationType, HRESULT *error = nullptr);

private:
	static std::unique_ptr<XmlAppStorage> BuildForLoad(const std::wstring &configFilePath,
		HRESULT &error);
	static std::unique_ptr<XmlAppStorage> BuildForSave(const std::wstring &configFilePath,
		HRESULT &error);
};
