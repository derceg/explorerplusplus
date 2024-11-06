// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AppStorage.h"
#include <wil/com.h>
#include <MsXml2.h>

class BookmarkTree;

class XmlAppStorage : public AppStorage
{
public:
	XmlAppStorage(wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument);

	void LoadBookmarks(BookmarkTree *bookmarkTree) override;
	void LoadColorRules(ColorRuleModel *model) override;

private:
	const wil::com_ptr_nothrow<IXMLDOMDocument> m_xmlDocument;
};
