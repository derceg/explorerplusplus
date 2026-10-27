// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainToolbarStorage.h"
#include "MainToolbarButtonId.h"
#include "MainToolbarLayout.h"
#include "RegistryStorageTestHelper.h"
#include "ResourceTestHelper.h"
#include "XmlStorageTestHelper.h"
#include "../Helper/XMLSettings.h"
#include <gtest/gtest.h>
#include <format>

using namespace testing;

namespace
{

MainToolbarLayout BuildMainToolbarLoadSaveReference()
{
	return MainToolbarLayout({
		MainToolbarButtonId::Folders,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::Up,
		MainToolbarButtonId::Back,
		MainToolbarButtonId::Forward,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::Search,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::Paste,
		MainToolbarButtonId::Delete,
		MainToolbarButtonId::Copy,
		MainToolbarButtonId::Cut,
		MainToolbarButtonId::DeletePermanently,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::NewFolder,
		MainToolbarButtonId::CopyTo,
		MainToolbarButtonId::MoveTo,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::OpenCommandPrompt,
		MainToolbarButtonId::Refresh,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::AddBookmark,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::NewTab,
		MainToolbarButtonId::MergeFiles,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::CloseTab,
	});
}

}

class MainToolbarRegistryStorageTest : public RegistryStorageTest
{
protected:
	static inline const WCHAR VALUE_NAME[] = L"ToolbarState";
};

TEST_F(MainToolbarRegistryStorageTest, Load)
{
	auto referenceButtons = BuildMainToolbarLoadSaveReference();

	ImportRegistryResource(L"main-toolbar.reg");

	auto loadedButtons =
		MainToolbarStorage::LoadFromRegistry(m_applicationTestKey.get(), VALUE_NAME);
	ASSERT_TRUE(loadedButtons.has_value());

	EXPECT_EQ(*loadedButtons, referenceButtons);
}

TEST_F(MainToolbarRegistryStorageTest, Save)
{
	auto referenceButtons = BuildMainToolbarLoadSaveReference();

	MainToolbarStorage::SaveToRegistry(m_applicationTestKey.get(), VALUE_NAME, referenceButtons);

	auto loadedButtons =
		MainToolbarStorage::LoadFromRegistry(m_applicationTestKey.get(), VALUE_NAME);
	ASSERT_TRUE(loadedButtons.has_value());

	EXPECT_EQ(*loadedButtons, referenceButtons);
}

class MainToolbarXmlStorageTest : public XmlStorageTest
{
protected:
	static inline const WCHAR NODE_NAME[] = L"ToolbarState";
};

TEST_F(MainToolbarXmlStorageTest, Load)
{
	auto referenceButtons = BuildMainToolbarLoadSaveReference();

	std::wstring xmlFilePath = GetResourcePath(L"main-toolbar.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	wil::com_ptr_nothrow<IXMLDOMNode> mainToolbarNode;
	auto queryString =
		wil::make_bstr_nothrow(std::format(L"Settings/Setting[@name='{}']", NODE_NAME).c_str());
	HRESULT hr = xmlDocumentData.rootNode->selectSingleNode(queryString.get(), &mainToolbarNode);
	ASSERT_EQ(hr, S_OK);

	auto loadedButtons = MainToolbarStorage::LoadFromXml(mainToolbarNode.get());
	ASSERT_TRUE(loadedButtons.has_value());

	EXPECT_EQ(*loadedButtons, referenceButtons);
}

TEST_F(MainToolbarXmlStorageTest, Save)
{
	auto referenceButtons = BuildMainToolbarLoadSaveReference();

	auto xmlDocumentData = CreateXmlDocument();

	wil::com_ptr_nothrow<IXMLDOMElement> settingsNode;
	auto bstr = wil::make_bstr_nothrow(L"Settings");
	HRESULT hr = xmlDocumentData.xmlDocument->createElement(bstr.get(), &settingsNode);
	ASSERT_HRESULT_SUCCEEDED(hr);

	wil::com_ptr_nothrow<IXMLDOMElement> mainToolbarNode;
	XMLSettings::CreateElementNode(xmlDocumentData.xmlDocument.get(), &mainToolbarNode,
		settingsNode.get(), L"Setting", NODE_NAME);
	MainToolbarStorage::SaveToXml(xmlDocumentData.xmlDocument.get(), mainToolbarNode.get(),
		referenceButtons);

	auto loadedButtons = MainToolbarStorage::LoadFromXml(mainToolbarNode.get());
	ASSERT_TRUE(loadedButtons.has_value());

	EXPECT_EQ(*loadedButtons, referenceButtons);
}
