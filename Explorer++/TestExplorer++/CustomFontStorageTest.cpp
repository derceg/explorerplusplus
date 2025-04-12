// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "CustomFontStorage.h"
#include "CustomFont.h"
#include "RegistryStorageTestHelper.h"
#include "ResourceTestHelper.h"
#include "XmlStorageTestHelper.h"
#include "../Helper/XMLSettings.h"
#include <gtest/gtest.h>
#include <wil/registry.h>
#include <format>

using namespace testing;

CustomFont BuildLoadSaveReferenceFont()
{
	return { L"Agency FB", 20 };
}

class CustomFontRegistryStorageTest : public RegistryStorageTest
{
protected:
	static inline const wchar_t MAIN_FONT_KEY_NAME[] = L"MainFont";
};

TEST_F(CustomFontRegistryStorageTest, Load)
{
	CustomFont referenceFont = BuildLoadSaveReferenceFont();

	ImportRegistryResource(L"custom-font.reg");

	wil::unique_hkey mainFontKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(m_applicationTestKey.get(), MAIN_FONT_KEY_NAME,
		mainFontKey, wil::reg::key_access::read);
	ASSERT_HRESULT_SUCCEEDED(hr);

	auto loadedFont = CustomFontStorage::LoadFromRegistry(mainFontKey.get());
	ASSERT_NE(loadedFont, nullptr);

	EXPECT_EQ(*loadedFont, referenceFont);
}

TEST_F(CustomFontRegistryStorageTest, Save)
{
	CustomFont referenceFont = BuildLoadSaveReferenceFont();

	wil::unique_hkey mainFontKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(m_applicationTestKey.get(), MAIN_FONT_KEY_NAME,
		mainFontKey, wil::reg::key_access::readwrite);
	ASSERT_HRESULT_SUCCEEDED(hr);

	CustomFontStorage::SaveToRegistry(mainFontKey.get(), referenceFont);

	auto loadedFont = CustomFontStorage::LoadFromRegistry(mainFontKey.get());
	ASSERT_NE(loadedFont, nullptr);

	EXPECT_EQ(*loadedFont, referenceFont);
}

class CustomFontXmlStorageTest : public XmlStorageTest
{
protected:
	static inline const WCHAR MAIN_FONT_NODE_NAME[] = L"MainFont";
};

TEST_F(CustomFontXmlStorageTest, Load)
{
	CustomFont referenceFont = BuildLoadSaveReferenceFont();

	std::wstring xmlFilePath = GetResourcePath(L"custom-font.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	wil::com_ptr_nothrow<IXMLDOMNode> mainFontNode;
	auto queryString = wil::make_bstr_nothrow(
		std::format(L"Settings/Setting[@name='{}']", MAIN_FONT_NODE_NAME).c_str());
	HRESULT hr = xmlDocumentData.rootNode->selectSingleNode(queryString.get(), &mainFontNode);
	ASSERT_EQ(hr, S_OK);

	auto loadedFont = CustomFontStorage::LoadFromXml(mainFontNode.get());
	ASSERT_NE(loadedFont, nullptr);

	EXPECT_EQ(*loadedFont, referenceFont);
}

TEST_F(CustomFontXmlStorageTest, Save)
{
	CustomFont referenceFont = BuildLoadSaveReferenceFont();

	auto xmlDocumentData = CreateXmlDocument();

	wil::com_ptr_nothrow<IXMLDOMElement> settingsNode;
	auto bstr = wil::make_bstr_nothrow(L"Settings");
	HRESULT hr = xmlDocumentData.xmlDocument->createElement(bstr.get(), &settingsNode);
	ASSERT_HRESULT_SUCCEEDED(hr);

	wil::com_ptr_nothrow<IXMLDOMElement> mainFontNode;
	XMLSettings::CreateElementNode(xmlDocumentData.xmlDocument.get(), &mainFontNode,
		settingsNode.get(), L"Setting", MAIN_FONT_NODE_NAME);
	CustomFontStorage::SaveToXml(xmlDocumentData.xmlDocument.get(), mainFontNode.get(),
		referenceFont);

	auto loadedFont = CustomFontStorage::LoadFromXml(mainFontNode.get());
	ASSERT_NE(loadedFont, nullptr);

	EXPECT_EQ(*loadedFont, referenceFont);
}
