// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AppStorage.h"
#include "Storage.h"
#include <wil/com.h>
#include <MsXml2.h>

class BookmarkTree;

class XmlAppStorage : public AppStorage
{
public:
	XmlAppStorage(wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument,
		wil::com_ptr_nothrow<IXMLDOMNode> rootNode, const std::wstring &configFilePath,
		Storage::OperationType operationType);

	void LoadConfig(Config &config) override;
	[[nodiscard]] std::vector<WindowStorageData> LoadWindows() override;
	void LoadBookmarks(BookmarkTree *bookmarkTree) override;
	void LoadColorRules(ColorRuleModel *model) override;
	void LoadApplications(Applications::ApplicationModel *model) override;
	void LoadDialogStates() override;
	void LoadDefaultColumns(FolderColumns &defaultColumns) override;
	void LoadFrequentLocations(FrequentLocationsModel *frequentLocationsModel) override;

	void SaveConfig(const Config &config) override;
	void SaveWindows(const std::vector<WindowStorageData> &windows) override;
	void SaveBookmarks(const BookmarkTree *bookmarkTree) override;
	void SaveColorRules(const ColorRuleModel *model) override;
	void SaveApplications(const Applications::ApplicationModel *model) override;
	void SaveDialogStates() override;
	void SaveDefaultColumns(const FolderColumns &defaultColumns) override;
	void SaveFrequentLocations(const FrequentLocationsModel *frequentLocationsModel) override;
	void Commit() override;

private:
	const wil::com_ptr_nothrow<IXMLDOMDocument> m_xmlDocument;
	const wil::com_ptr_nothrow<IXMLDOMNode> m_rootNode;
	const std::wstring m_configFilePath;
	const Storage::OperationType m_operationType;
};
