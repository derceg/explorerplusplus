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
#include "DialogHelper.h"
#include "FrequentLocationsXmlStorage.h"
#include "MainRebarStorage.h"
#include "TabStorage.h"
#include "WindowStorage.h"
#include "WindowXmlStorage.h"

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
	DialogHelper::LoadDialogStatesFromXML(m_xmlDocument.get());
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
	ConfigXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), config);
}

void XmlAppStorage::SaveWindows(const std::vector<WindowStorageData> &windows)
{
	WindowXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), windows);
}

void XmlAppStorage::SaveBookmarks(const BookmarkTree *bookmarkTree)
{
	BookmarkXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), bookmarkTree);
}

void XmlAppStorage::SaveColorRules(const ColorRuleModel *model)
{
	ColorRuleXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), model);
}

void XmlAppStorage::SaveApplications(const Applications::ApplicationModel *model)
{
	Applications::ApplicationToolbarXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), model);
}

void XmlAppStorage::SaveDialogStates()
{
	DialogHelper::SaveDialogStatesToXML(m_xmlDocument.get(), m_rootNode.get());
}

void XmlAppStorage::SaveDefaultColumns(const FolderColumns &defaultColumns)
{
	DefaultColumnXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(), defaultColumns);
}

void XmlAppStorage::SaveFrequentLocations(const FrequentLocationsModel *frequentLocationsModel)
{
	FrequentLocationsXmlStorage::Save(m_xmlDocument.get(), m_rootNode.get(),
		frequentLocationsModel);
}

void XmlAppStorage::Commit()
{
	if (m_operationType != Storage::OperationType::Save)
	{
		DCHECK(false);
		return;
	}

	auto destination = wil::make_variant_bstr_failfast(m_configFilePath.c_str());
	m_xmlDocument->save(destination);
}
