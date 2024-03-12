// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainToolbarStorage.h"
#include "MainToolbarButtons.h"
#include "RegistryStorageHelper.h"
#include "ResourceHelper.h"
#include "XmlStorageHelper.h"
#include "../Helper/XMLSettings.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{

MainToolbarStorage::MainToolbarButtons BuildMainToolbarLoadSaveReference()
{
	MainToolbarStorage::MainToolbarButtons buttons;
	buttons.AddButton(MainToolbarButton::Folders);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::Up);
	buttons.AddButton(MainToolbarButton::Back);
	buttons.AddButton(MainToolbarButton::Forward);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::Search);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::Paste);
	buttons.AddButton(MainToolbarButton::Delete);
	buttons.AddButton(MainToolbarButton::Copy);
	buttons.AddButton(MainToolbarButton::Cut);
	buttons.AddButton(MainToolbarButton::DeletePermanently);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::NewFolder);
	buttons.AddButton(MainToolbarButton::CopyTo);
	buttons.AddButton(MainToolbarButton::MoveTo);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::OpenCommandPrompt);
	buttons.AddButton(MainToolbarButton::Refresh);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::AddBookmark);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::NewTab);
	buttons.AddButton(MainToolbarButton::MergeFiles);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::CloseTab);
	return buttons;
}

}

namespace MainToolbarStorage
{

bool operator==(const MainToolbarButtons &first, const MainToolbarButtons &second)
{
	return first.GetButtons() == second.GetButtons();
}

}

TEST(MainToolbarButtonsTest, DuplicateButtons)
{
	MainToolbarStorage::MainToolbarButtons buttons;
	buttons.AddButton(MainToolbarButton::Up);
	buttons.AddButton(MainToolbarButton::OpenCommandPrompt);
	EXPECT_EQ(buttons.GetButtons().size(), 2);

	// It shouldn't be possible to add a standard button more than once, so this call should have no
	// effect.
	buttons.AddButton(MainToolbarButton::Up);
	EXPECT_EQ(buttons.GetButtons().size(), 2);
}

TEST(MainToolbarButtonsTest, DuplicateSeparators)
{
	MainToolbarStorage::MainToolbarButtons buttons;
	buttons.AddButton(MainToolbarButton::Properties);
	buttons.AddButton(MainToolbarButton::Separator);
	buttons.AddButton(MainToolbarButton::Refresh);
	EXPECT_EQ(buttons.GetButtons().size(), 3);

	// Unlike standard buttons, it should be possible for separators to appear multiple times.
	buttons.AddButton(MainToolbarButton::Separator);
	EXPECT_EQ(buttons.GetButtons().size(), 4);
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
	auto xmlDocument = LoadXmlDocument(xmlFilePath);
	ASSERT_TRUE(xmlDocument);

	wil::com_ptr_nothrow<IXMLDOMNode> mainToolbarNode;
	auto queryString = wil::make_bstr_nothrow(
		(std::wstring(L"/ExplorerPlusPlus/Settings/Setting[@name='") + NODE_NAME + L"']").c_str());
	HRESULT hr = xmlDocument->selectSingleNode(queryString.get(), &mainToolbarNode);
	ASSERT_EQ(hr, S_OK);

	auto loadedButtons = MainToolbarStorage::LoadFromXml(mainToolbarNode.get());
	ASSERT_TRUE(loadedButtons.has_value());

	EXPECT_EQ(*loadedButtons, referenceButtons);
}

TEST_F(MainToolbarXmlStorageTest, Save)
{
	auto referenceButtons = BuildMainToolbarLoadSaveReference();

	auto xmlDocumentData = CreateXmlDocument();
	ASSERT_TRUE(xmlDocumentData);

	wil::com_ptr_nothrow<IXMLDOMElement> settingsNode;
	auto bstr = wil::make_bstr_nothrow(L"Settings");
	HRESULT hr = xmlDocumentData->xmlDocument->createElement(bstr.get(), &settingsNode);
	ASSERT_HRESULT_SUCCEEDED(hr);

	wil::com_ptr_nothrow<IXMLDOMElement> mainToolbarNode;
	NXMLSettings::CreateElementNode(xmlDocumentData->xmlDocument.get(), &mainToolbarNode,
		settingsNode.get(), L"Setting", NODE_NAME);
	MainToolbarStorage::SaveToXml(xmlDocumentData->xmlDocument.get(), mainToolbarNode.get(),
		referenceButtons);

	auto loadedButtons = MainToolbarStorage::LoadFromXml(mainToolbarNode.get());
	ASSERT_TRUE(loadedButtons.has_value());

	EXPECT_EQ(*loadedButtons, referenceButtons);
}
