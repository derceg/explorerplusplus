// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "XmlAppStorage.h"
#include "ApplicationToolbarXmlStorage.h"
#include "Bookmarks/BookmarkXmlStorage.h"
#include "ColorRuleXmlStorage.h"
#include "DefaultColumnXmlStorage.h"
#include "DialogHelper.h"
#include "WindowStorage.h"
#include "WindowXmlStorage.h"

XmlAppStorage::XmlAppStorage(wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument) :
	m_xmlDocument(xmlDocument)
{
}

std::vector<WindowStorageData> XmlAppStorage::LoadWindows()
{
	return WindowXmlStorage::Load(m_xmlDocument.get());
}

void XmlAppStorage::LoadBookmarks(BookmarkTree *bookmarkTree)
{
	BookmarkXmlStorage::Load(m_xmlDocument.get(), bookmarkTree);
}

void XmlAppStorage::LoadColorRules(ColorRuleModel *model)
{
	ColorRuleXmlStorage::Load(m_xmlDocument.get(), model);
}

void XmlAppStorage::LoadApplications(Applications::ApplicationModel *model)
{
	Applications::ApplicationToolbarXmlStorage::Load(m_xmlDocument.get(), model);
}

void XmlAppStorage::LoadDialogStates()
{
	DialogHelper::LoadDialogStatesFromXML(m_xmlDocument.get());
}

void XmlAppStorage::LoadDefaultColumns(FolderColumns &defaultColumns)
{
	DefaultColumnXmlStorage::Load(m_xmlDocument.get(), defaultColumns);
}
