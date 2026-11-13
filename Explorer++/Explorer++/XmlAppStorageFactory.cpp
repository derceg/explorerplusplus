// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "XmlAppStorageFactory.h"
#include "XmlAppStorage.h"
#include "../Helper/XMLSettings.h"

std::unique_ptr<XmlAppStorage> XmlAppStorageFactory::LoadWithRecovery(
	const std::wstring &configFilePath, HRESULT &error)
{
	auto storage = BuildForLoad(configFilePath, error);
	if (storage)
		return storage;
	HRESULT backupError = S_OK;
	auto backup = BuildForLoad(configFilePath + L".bak", backupError);
	if (backup)
		return backup;
	// A present but damaged backup also identifies an existing XML profile.
	if ((error == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
			|| error == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
		&& backupError != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
		&& backupError != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
		error = backupError;
	return nullptr;
}

std::unique_ptr<XmlAppStorage> XmlAppStorageFactory::MaybeCreate(const std::wstring &configFilePath,
	Storage::OperationType operationType, HRESULT *error)
{
	HRESULT status = S_OK;
	auto storage = operationType == Storage::OperationType::Load
		? BuildForLoad(configFilePath, status)
		: BuildForSave(configFilePath, status);
	if (error)
		*error = status;
	return storage;
}

std::unique_ptr<XmlAppStorage> XmlAppStorageFactory::BuildForLoad(
	const std::wstring &configFilePath, HRESULT &error)
{
	if (GetFileAttributes(configFilePath.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		error = HRESULT_FROM_WIN32(GetLastError());
		return nullptr;
	}
	auto xmlDocument = XMLSettings::CreateXmlDocument();
	if (!xmlDocument)
	{
		error = E_FAIL;
		return nullptr;
	}
	auto configFilePathVariant = wil::make_variant_bstr_failfast(configFilePath.c_str());
	VARIANT_BOOL status = VARIANT_FALSE;
	error = xmlDocument->load(configFilePathVariant, &status);
	if (FAILED(error) || status != VARIANT_TRUE)
	{
		if (SUCCEEDED(error))
			error = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		return nullptr;
	}
	wil::com_ptr_nothrow<IXMLDOMNode> rootNode;
	auto query = wil::make_bstr_failfast(Storage::CONFIG_FILE_ROOT_NODE_NAME);
	error = xmlDocument->selectSingleNode(query.get(), &rootNode);
	if (error != S_OK || !rootNode)
	{
		if (SUCCEEDED(error))
			error = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		return nullptr;
	}
	return std::make_unique<XmlAppStorage>(xmlDocument, rootNode, configFilePath,
		Storage::OperationType::Load);
}

std::unique_ptr<XmlAppStorage> XmlAppStorageFactory::BuildForSave(
	const std::wstring &configFilePath, HRESULT &error)
{
	StorageWriteScope writeScope(error);
	auto xmlDocument = XMLSettings::CreateXmlDocument();

	if (!xmlDocument)
	{
		error = E_FAIL;
		return nullptr;
	}

	wil::com_ptr_nothrow<IXMLDOMComment> comment;
	auto commentText = wil::make_bstr_failfast(L" Preference file for Explorer++ ");
	HRESULT hr = StorageWriteScope::Record(xmlDocument->createComment(commentText.get(), &comment));

	if (hr != S_OK)
	{
		return nullptr;
	}

	XMLSettings::AppendChildToParent(comment.get(), xmlDocument.get());

	auto rootTag = wil::make_bstr_nothrow(Storage::CONFIG_FILE_ROOT_NODE_NAME);
	wil::com_ptr_nothrow<IXMLDOMElement> rootNode;
	hr = StorageWriteScope::Record(xmlDocument->createElement(rootTag.get(), &rootNode));

	if (hr != S_OK)
	{
		return nullptr;
	}

	XMLSettings::AppendChildToParent(rootNode.get(), xmlDocument.get());
	if (FAILED(error))
		return nullptr;

	return std::make_unique<XmlAppStorage>(xmlDocument, rootNode, configFilePath,
		Storage::OperationType::Save);
}
