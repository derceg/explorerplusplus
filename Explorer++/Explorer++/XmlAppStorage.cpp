// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "XmlAppStorage.h"
#include "Bookmarks/BookmarkXmlStorage.h"

XmlAppStorage::XmlAppStorage(wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument) :
	m_xmlDocument(xmlDocument)
{
}

void XmlAppStorage::LoadBookmarks(BookmarkTree *bookmarkTree)
{
	BookmarkXmlStorage::Load(m_xmlDocument.get(), bookmarkTree);
}
