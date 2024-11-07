// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "XmlAppStorage.h"
#include "ApplicationToolbarXmlStorage.h"
#include "Bookmarks/BookmarkXmlStorage.h"
#include "ColorRuleXmlStorage.h"

XmlAppStorage::XmlAppStorage(wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument) :
	m_xmlDocument(xmlDocument)
{
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
