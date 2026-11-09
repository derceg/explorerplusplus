// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "XmlAppStorage.h"
#include "ApplicationToolbarXmlStorage.h"
#include "Bookmarks/BookmarkXmlStorage.h"
#include "ColorRuleXmlStorage.h"
#include "ConfigXmlStorage.h"
#include "DefaultColumnXmlStorage.h"
#include "DialogStorageHelper.h"
#include "FrequentLocationsXmlStorage.h"
#include "RebarBandStorageData.h"
#include "TabStorage.h"
#include "WindowStorage.h"
#include "WindowXmlStorage.h"
#include "../Helper/AtomicFile.h"
#include "../Helper/XMLSettings.h"
#include <Shlwapi.h>
#include <propvarutil.h>

XmlAppStorage::XmlAppStorage(wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument,
	wil::com_ptr_nothrow<IXMLDOMNode> rootNode, const std::wstring &configFilePath,
	Storage::OperationType operationType) :
	m_xmlDocument(xmlDocument),
	m_rootNode(rootNode),
	m_configFilePath(configFilePath),
	m_operationType(operationType)
{
}

void XmlAppStorage::LoadConfig(Config &config)
{
	ConfigXmlStorage::Load(m_rootNode.get(), config);
}

std::vector<WindowStorageData> XmlAppStorage::LoadWindows()
{
	return WindowXmlStorage::Load(m_rootNode.get());
}

void XmlAppStorage::LoadBookmarks(BookmarkTree *bookmarkTree)
{
	BookmarkXmlStorage::Load(m_rootNode.get(), bookmarkTree);
}

void XmlAppStorage::LoadColorRules(ColorRuleModel *model)
{
	ColorRuleXmlStorage::Load(m_rootNode.get(), model);
}

void XmlAppStorage::LoadApplications(Applications::ApplicationModel *model)
{
	Applications::ApplicationToolbarXmlStorage::Load(m_rootNode.get(), model);
}

void XmlAppStorage::LoadDialogStates()
{
	DialogStorageHelper::LoadDialogStatesFromXML(m_xmlDocument.get());
}

void XmlAppStorage::LoadDefaultColumns(FolderColumns &defaultColumns)
{
	DefaultColumnXmlStorage::Load(m_rootNode.get(), defaultColumns);
}

void XmlAppStorage::LoadFrequentLocations(FrequentLocationsModel *frequentLocationsModel)
{
	FrequentLocationsXmlStorage::Load(m_rootNode.get(), frequentLocationsModel);
}

void XmlAppStorage::SaveConfig(const Config &config)
{
	StorageWriteScope writeScope(m_writeError);
	ConfigXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), config);
}

void XmlAppStorage::SaveWindows(const std::vector<WindowStorageData> &windows)
{
	StorageWriteScope writeScope(m_writeError);
	WindowXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), windows);
}

void XmlAppStorage::SaveBookmarks(const BookmarkTree *bookmarkTree)
{
	StorageWriteScope writeScope(m_writeError);
	BookmarkXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), bookmarkTree);
}

void XmlAppStorage::SaveColorRules(const ColorRuleModel *model)
{
	StorageWriteScope writeScope(m_writeError);
	ColorRuleXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), model);
}

void XmlAppStorage::SaveApplications(const Applications::ApplicationModel *model)
{
	StorageWriteScope writeScope(m_writeError);
	Applications::ApplicationToolbarXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), model);
}

void XmlAppStorage::SaveDialogStates()
{
	StorageWriteScope writeScope(m_writeError);
	DialogStorageHelper::SaveDialogStatesToXML(m_xmlDocument.get(), m_rootNode.get());
}

void XmlAppStorage::SaveDefaultColumns(const FolderColumns &defaultColumns)
{
	StorageWriteScope writeScope(m_writeError);
	DefaultColumnXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), defaultColumns);
}

void XmlAppStorage::SaveFrequentLocations(const FrequentLocationsModel *frequentLocationsModel)
{
	StorageWriteScope writeScope(m_writeError);
	FrequentLocationsXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(),
		frequentLocationsModel);
}

HRESULT XmlAppStorage::Commit()
{
	RETURN_HR_IF(E_UNEXPECTED, m_operationType != Storage::OperationType::Save || m_committed);
	RETURN_IF_FAILED(m_writeError);
	RETURN_IF_FAILED(XMLSettings::FormatXmlDocument(m_xmlDocument.get()));

	// Serialize completely before touching the current file. The exclusive sidecar
	// handle also serializes concurrent writers, including backup replacement.
	wil::unique_hfile lock(CreateFile((m_configFilePath + L".lock").c_str(), GENERIC_WRITE, 0,
		nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
	RETURN_LAST_ERROR_IF(!lock);
	wil::com_ptr_nothrow<IStream> stream;
	stream.attach(SHCreateMemStream(nullptr, 0));
	RETURN_HR_IF_NULL(E_OUTOFMEMORY, stream);
	wil::unique_variant destination;
	RETURN_IF_FAILED(InitVariantFromUnknown(stream.get(), &destination));
	RETURN_IF_FAILED(m_xmlDocument->save(destination));
	LARGE_INTEGER start{};
	RETURN_IF_FAILED(stream->Seek(start, STREAM_SEEK_SET, nullptr));
	AtomicFile file;
	RETURN_IF_FAILED(file.Create(m_configFilePath));
	char buffer[65536];
	for (;;)
	{
		ULONG read = 0;
		RETURN_IF_FAILED(stream->Read(buffer, sizeof(buffer), &read));
		if (read == 0)
			break;
		RETURN_IF_FAILED(file.Write(buffer, read));
	}
	RETURN_IF_FAILED(file.Flush());
	RETURN_IF_FAILED(file.Publish(true, m_configFilePath + L".bak"));
	file.Accept();
	m_committed = true;
	return S_OK;
}
