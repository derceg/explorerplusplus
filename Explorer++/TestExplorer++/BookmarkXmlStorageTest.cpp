// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "Bookmarks/BookmarkXmlStorage.h"
#include "Bookmarks/BookmarkTree.h"
#include "ResourceHelper.h"
#include "../Helper/XMLSettings.h"
#include <gtest/gtest.h>
#include <wil/com.h>

using namespace testing;

void CompareBookmarkTrees(const BookmarkTree *firstTree, const BookmarkTree *secondTree);
void CompareFolders(const BookmarkItem *firstFolder, const BookmarkItem *secondFolder);
void CompareBookmarks(const BookmarkItem *firstBookmark, const BookmarkItem *secondBookmark);
wil::com_ptr<IXMLDOMDocument> InitializeXmlDocument(const std::wstring &filePath);

class BookmarkXmlStorageTest : public Test
{
protected:
	BookmarkXmlStorageTest()
	{
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	}
};

TEST_F(BookmarkXmlStorageTest, V1BasicLoad)
{
	BookmarkTree referenceBookmarkTree;

	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Root", L"C:\\");
	referenceBookmarkTree.AddBookmarkItem(
		referenceBookmarkTree.GetBookmarksToolbarFolder(), std::move(bookmark), 0);

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt);
	auto *rawFolder = referenceBookmarkTree.AddBookmarkItem(
		referenceBookmarkTree.GetBookmarksToolbarFolder(), std::move(folder), 1);
	ASSERT_TRUE(rawFolder);

	bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Windows", L"C:\\Windows");
	referenceBookmarkTree.AddBookmarkItem(rawFolder, std::move(bookmark), 0);

	folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt);
	referenceBookmarkTree.AddBookmarkItem(
		referenceBookmarkTree.GetBookmarksMenuFolder(), std::move(folder), 0);

	std::wstring xmlFilePath = GetResourcePath(L"bookmarks-v1-config.xml");
	auto xmlDocument = InitializeXmlDocument(xmlFilePath);
	ASSERT_TRUE(xmlDocument);

	BookmarkTree loadedBookmarkTree;
	BookmarkXmlStorage::Load(xmlDocument.get(), &loadedBookmarkTree);

	CompareBookmarkTrees(&loadedBookmarkTree, &referenceBookmarkTree);
}

TEST_F(BookmarkXmlStorageTest, V1NestedShowOnToolbarLoad)
{
	BookmarkTree referenceBookmarkTree;

	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Root", L"C:\\");
	referenceBookmarkTree.AddBookmarkItem(
		referenceBookmarkTree.GetBookmarksToolbarFolder(), std::move(bookmark), 0);

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt);
	auto *rawFolder = referenceBookmarkTree.AddBookmarkItem(
		referenceBookmarkTree.GetBookmarksMenuFolder(), std::move(folder), 0);
	ASSERT_TRUE(rawFolder);

	bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Windows", L"C:\\Windows");
	referenceBookmarkTree.AddBookmarkItem(rawFolder, std::move(bookmark), 0);

	folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt);
	referenceBookmarkTree.AddBookmarkItem(
		referenceBookmarkTree.GetBookmarksMenuFolder(), std::move(folder), 1);

	std::wstring xmlFilePath = GetResourcePath(L"bookmarks-v1-config-nested-show-on-toolbar.xml");
	auto xmlDocument = InitializeXmlDocument(xmlFilePath);
	ASSERT_TRUE(xmlDocument);

	BookmarkTree loadedBookmarkTree;
	BookmarkXmlStorage::Load(xmlDocument.get(), &loadedBookmarkTree);

	CompareBookmarkTrees(&loadedBookmarkTree, &referenceBookmarkTree);
}

void CompareBookmarkTrees(const BookmarkTree *firstTree, const BookmarkTree *secondTree)
{
	CompareFolders(firstTree->GetBookmarksMenuFolder(), secondTree->GetBookmarksMenuFolder());
	CompareFolders(firstTree->GetBookmarksToolbarFolder(), secondTree->GetBookmarksToolbarFolder());
	CompareFolders(firstTree->GetOtherBookmarksFolder(), secondTree->GetOtherBookmarksFolder());
}

void CompareFolders(const BookmarkItem *firstFolder, const BookmarkItem *secondFolder)
{
	EXPECT_EQ(firstFolder->GetName(), secondFolder->GetName());

	auto &firstChildren = firstFolder->GetChildren();
	auto &secondChildren = secondFolder->GetChildren();
	ASSERT_EQ(firstChildren.size(), secondChildren.size());

	for (int i = 0; i < firstChildren.size(); i++)
	{
		auto *firstCurrent = firstChildren[i].get();
		auto *secondCurrent = secondChildren[i].get();

		ASSERT_EQ(firstCurrent->GetType(), secondCurrent->GetType());

		if (firstCurrent->IsFolder())
		{
			CompareFolders(firstCurrent, secondCurrent);
		}
		else
		{
			CompareBookmarks(firstCurrent, secondCurrent);
		}
	}
}

void CompareBookmarks(const BookmarkItem *firstBookmark, const BookmarkItem *secondBookmark)
{
	EXPECT_EQ(firstBookmark->GetName(), secondBookmark->GetName());
	EXPECT_EQ(firstBookmark->GetLocation(), secondBookmark->GetLocation());
}

wil::com_ptr<IXMLDOMDocument> InitializeXmlDocument(const std::wstring &filePath)
{
	wil::com_ptr<IXMLDOMDocument> xmlDocument(NXMLSettings::DomFromCOM());

	if (!xmlDocument)
	{
		return nullptr;
	}

	VARIANT_BOOL status;
	VARIANT variantFilePath = NXMLSettings::VariantString(filePath.c_str());
	xmlDocument->load(variantFilePath, &status);

	if (status != VARIANT_TRUE)
	{
		return nullptr;
	}

	return xmlDocument;
}