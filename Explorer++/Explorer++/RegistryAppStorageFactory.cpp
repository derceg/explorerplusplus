// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RegistryAppStorageFactory.h"
#include "RegistryAppStorage.h"
#include <ktmw32.h>
#include <vector>

#pragma comment(lib, "KtmW32.lib")

namespace
{
LSTATUS ClearKey(HKEY key, HANDLE transaction)
{
	// Explicitly enlist every child: transactions are not inherited by subkeys.
	for (;;)
	{
		wchar_t name[256];
		DWORD length = static_cast<DWORD>(std::size(name));
		auto status = RegEnumKeyEx(key, 0, name, &length, nullptr, nullptr, nullptr, nullptr);
		if (status == ERROR_NO_MORE_ITEMS)
			break;
		if (status != ERROR_SUCCESS)
			return status;
		wil::unique_hkey child;
		status = RegOpenKeyTransacted(key, name, 0, KEY_READ | KEY_WRITE | DELETE, child.put(),
			transaction, nullptr);
		if (status != ERROR_SUCCESS)
			return status;
		status = ClearKey(child.get(), transaction);
		child.reset();
		if (status != ERROR_SUCCESS)
			return status;
		status = RegDeleteKeyTransacted(key, name, 0, 0, transaction, nullptr);
		if (status != ERROR_SUCCESS)
			return status;
	}
	DWORD maximumName = 0;
	auto status = RegQueryInfoKey(key, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, &maximumName, nullptr, nullptr, nullptr);
	if (status != ERROR_SUCCESS)
		return status;
	std::vector<wchar_t> name(static_cast<size_t>(maximumName) + 1);
	for (;;)
	{
		DWORD length = static_cast<DWORD>(name.size());
		status = RegEnumValue(key, 0, name.data(), &length, nullptr, nullptr, nullptr, nullptr);
		if (status == ERROR_NO_MORE_ITEMS)
			return ERROR_SUCCESS;
		if (status != ERROR_SUCCESS)
			return status;
		status = RegDeleteValue(key, name.data());
		if (status != ERROR_SUCCESS)
			return status;
	}
}
}

std::unique_ptr<RegistryAppStorage> RegistryAppStorageFactory::MaybeCreate(
	const std::wstring &applicationKeyPath, Storage::OperationType operationType, HRESULT *error)
{
	if (error)
		*error = S_OK;
	auto fail = [error](LSTATUS status) -> std::unique_ptr<RegistryAppStorage>
	{
		if (error)
			*error = HRESULT_FROM_WIN32(status);
		return nullptr;
	};
	wil::unique_hkey applicationKey;
	if (operationType == Storage::OperationType::Load)
	{
		auto status = RegOpenKeyEx(HKEY_CURRENT_USER, applicationKeyPath.c_str(), 0, KEY_READ,
			applicationKey.put());
		if (status != ERROR_SUCCESS)
			return fail(status);
		return std::make_unique<RegistryAppStorage>(std::move(applicationKey));
	}

	HANDLE handle = CreateTransaction(nullptr, nullptr, 0, 0, 0, 0, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
		return fail(GetLastError());
	wil::unique_handle transaction(handle);
	auto status = RegCreateKeyTransacted(HKEY_CURRENT_USER, applicationKeyPath.c_str(), 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE | DELETE, nullptr, applicationKey.put(),
		nullptr, transaction.get(), nullptr);
	if (status == ERROR_SUCCESS)
		status = ClearKey(applicationKey.get(), transaction.get());
	if (status != ERROR_SUCCESS)
	{
		applicationKey.reset();
		RollbackTransaction(transaction.get());
		return fail(status);
	}
	return std::make_unique<RegistryAppStorage>(std::move(applicationKey), std::move(transaction));
}
