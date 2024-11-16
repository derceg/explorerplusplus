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
	XmlAppStorage(wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument,
		wil::com_ptr_nothrow<IXMLDOMNode> rootNode);

	[[nodiscard]] std::vector<WindowStorageData> LoadWindows() override;
	void LoadBookmarks(BookmarkTree *bookmarkTree) override;
	void LoadColorRules(ColorRuleModel *model) override;
	void LoadApplications(Applications::ApplicationModel *model) override;
	void LoadDialogStates() override;
	void LoadDefaultColumns(FolderColumns &defaultColumns) override;

private:
	const wil::com_ptr_nothrow<IXMLDOMDocument> m_xmlDocument;
	const wil::com_ptr_nothrow<IXMLDOMNode> m_rootNode;
};
