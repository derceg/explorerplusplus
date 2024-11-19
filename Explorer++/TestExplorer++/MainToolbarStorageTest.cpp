// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainToolbarStorage.h"
#include "MainToolbarButtons.h"
#include "RegistryStorageTestHelper.h"
#include "ResourceTestHelper.h"
#include "XmlStorageTestHelper.h"
#include "../Helper/XMLSettings.h"
#include <gtest/gtest.h>
#include <format>

using namespace testing;

namespace
{

MainToolbarStorage::MainToolbarButtons BuildMainToolbarLoadSaveReference()
{
	return MainToolbarStorage::MainToolbarButtons({ MainToolbarButton::Folders,
		MainToolbarButton::Separator, MainToolbarButton::Up, MainToolbarButton::Back,
		MainToolbarButton::Forward, MainToolbarButton::Separator, MainToolbarButton::Search,
		MainToolbarButton::Separator, MainToolbarButton::Paste, MainToolbarButton::Delete,
		MainToolbarButton::Copy, MainToolbarButton::Cut, MainToolbarButton::DeletePermanently,
		MainToolbarButton::Separator, MainToolbarButton::NewFolder, MainToolbarButton::CopyTo,
		MainToolbarButton::MoveTo, MainToolbarButton::Separator,
		MainToolbarButton::OpenCommandPrompt, MainToolbarButton::Refresh,
		MainToolbarButton::Separator, MainToolbarButton::AddBookmark, MainToolbarButton::Separator,
		MainToolbarButton::NewTab, MainToolbarButton::MergeFiles, MainToolbarButton::Separator,
		MainToolbarButton::CloseTab });
}

}

TEST(MainToolbarButtonsTest, DuplicateButtons)
{
	MainToolbarStorage::MainToolbarButtons buttons;
	buttons.AddButton(MainToolbarButton::Up);
	buttons.AddButton(MainToolbarButton::OpenCommandPrompt);
	EXPECT_EQ(buttons.GetButtons().size(), 2U);

	// It shouldn't be possible to add a standard button more than once, so this call should have no
	// effect.
	buttons.AddButton(MainToolbarButton::Up);
	EXPECT_EQ(buttons.GetButtons().size(), 2U);
}

TEST(MainToolbarButtonsTest, DuplicateSeparators)
{
	MainToolbarStorage::MainToolbarButtons buttons;
	buttons.AddButton(MainToolbarButton::Properties);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::Refresh);
	EXPECT_EQ(buttons.GetButtons().size(), 3U);

	// Unlike standard buttons, it should be possible for separators to appear multiple times.
	buttons.AddButton(MainToolbarButton::Separator);
	EXPECT_EQ(buttons.GetButtons().size(), 4U);
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

	std::wstring xmlFilePath = GetResourcePath(L"main-toolbar-config.xml");
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
