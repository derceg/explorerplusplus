// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkXmlStorage.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/BookmarkStorage.h"
#include "Bookmarks/BookmarkTree.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>

namespace
{

namespace V2
{

const TCHAR bookmarksKeyNodeName[] = _T("Bookmarksv2");

void Load(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree);
void LoadPermanentFolder(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree,
	BookmarkItem *bookmarkItem, const std::wstring &name);
void LoadBookmarkChildren(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree,
	BookmarkItem *parentBookmarkItem);
std::unique_ptr<BookmarkItem> LoadBookmarkItem(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree);

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkTree *bookmarkTree, int indent);
void SavePermanentFolder(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *bookmarkItem, const std::wstring &name, int indent);
void SaveBookmarkChildren(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *parentBookmarkItem, int indent);
void SaveBookmarkItem(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *bookmarkItem, int indent);

}

namespace V1
{

const TCHAR bookmarksKeyNodeName[] = _T("Bookmarks");

void Load(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree);
void LoadBookmarkChildren(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree,
	BookmarkItem *parentBookmarkItem);
std::unique_ptr<BookmarkItem> LoadBookmarkItem(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree,
	bool &showOnToolbarOutput);

}

}

namespace BookmarkXmlStorage
{

void Load(IXMLDOMDocument *xmlDocument, BookmarkTree *bookmarkTree)
{
	wil::com_ptr_nothrow<IXMLDOMNode> bookmarksNode;
	auto queryString = wil::make_bstr_nothrow(
		(std::wstring(L"/ExplorerPlusPlus/") + std::wstring(V2::bookmarksKeyNodeName)).c_str());
	HRESULT hr = xmlDocument->selectSingleNode(queryString.get(), &bookmarksNode);

	if (hr == S_OK)
	{
		V2::Load(bookmarksNode.get(), bookmarkTree);
		return;
	}

	queryString = wil::make_bstr_nothrow(
		(std::wstring(L"/ExplorerPlusPlus/") + std::wstring(V1::bookmarksKeyNodeName)).c_str());
	hr = xmlDocument->selectSingleNode(queryString.get(), &bookmarksNode);

	if (hr == S_OK)
	{
		V1::Load(bookmarksNode.get(), bookmarkTree);
		return;
	}
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const BookmarkTree *bookmarkTree,
	int indent)
{
	auto newline =
		wil::make_bstr_nothrow((std::wstring(L"\n") + std::wstring(indent, '\t')).c_str());
	XMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), rootNode);

	wil::com_ptr_nothrow<IXMLDOMElement> bookmarksNode;
	auto bookmarksKeyNodeName = wil::make_bstr_nothrow(V2::bookmarksKeyNodeName);
	HRESULT hr = xmlDocument->createElement(bookmarksKeyNodeName.get(), &bookmarksNode);

	if (hr == S_OK)
	{
		V2::Save(xmlDocument, bookmarksNode.get(), bookmarkTree, indent + 1);

		XMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), bookmarksNode.get());
		XMLSettings::AppendChildToParent(bookmarksNode.get(), rootNode);
	}
}

}

namespace
{

namespace V2
{

void Load(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree)
{
	LoadPermanentFolder(parentNode, bookmarkTree, bookmarkTree->GetBookmarksToolbarFolder(),
		BookmarkStorage::BOOKMARKS_TOOLBAR_NODE_NAME);
	LoadPermanentFolder(parentNode, bookmarkTree, bookmarkTree->GetBookmarksMenuFolder(),
		BookmarkStorage::BOOKMARKS_MENU_NODE_NAME);
	LoadPermanentFolder(parentNode, bookmarkTree, bookmarkTree->GetOtherBookmarksFolder(),
		BookmarkStorage::OTHER_BOOKMARKS_NODE_NAME);
}

void LoadPermanentFolder(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree,
	BookmarkItem *bookmarkItem, const std::wstring &name)
{
	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	auto queryString = wil::make_bstr_nothrow((L"./PermanentItem[@name='" + name + L"']").c_str());
	HRESULT hr = parentNode->selectSingleNode(queryString.get(), &childNode);

	if (hr == S_OK)
	{
		wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
		childNode->get_attributes(&attributeMap);

		FILETIME dateCreated;
		XMLSettings::ReadDateTime(attributeMap.get(), _T("DateCreated"), dateCreated);
		bookmarkItem->SetDateCreated(dateCreated);

		FILETIME dateModified;
		XMLSettings::ReadDateTime(attributeMap.get(), _T("DateModified"), dateModified);
		bookmarkItem->SetDateModified(dateModified);

		LoadBookmarkChildren(childNode.get(), bookmarkTree, bookmarkItem);
	}
}

void LoadBookmarkChildren(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree,
	BookmarkItem *parentBookmarkItem)
{
	auto makeQueryString = [](int index)
	{
		return wil::make_bstr_nothrow(
			(L"./Bookmark[@name='" + std::to_wstring(index) + L"']").c_str());
	};

	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	int index = 0;
	auto queryString = makeQueryString(index);

	while (parentNode->selectSingleNode(queryString.get(), &childNode) == S_OK)
	{
		auto childBookmarkItem = LoadBookmarkItem(childNode.get(), bookmarkTree);
		bookmarkTree->AddBookmarkItem(parentBookmarkItem, std::move(childBookmarkItem), index);

		index++;
		queryString = makeQueryString(index);
	}
}

std::unique_ptr<BookmarkItem> LoadBookmarkItem(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	parentNode->get_attributes(&attributeMap);

	int type;
	XMLSettings::GetIntFromMap(attributeMap.get(), L"Type", type);

	std::wstring guid;
	XMLSettings::GetStringFromMap(attributeMap.get(), L"GUID", guid);

	std::wstring name;
	XMLSettings::GetStringFromMap(attributeMap.get(), L"ItemName", name);

	std::optional<std::wstring> locationOptional;

	if (type == static_cast<int>(BookmarkItem::Type::Bookmark))
	{
		std::wstring location;
		XMLSettings::GetStringFromMap(attributeMap.get(), L"Location", location);

		locationOptional = location;
	}

	auto bookmarkItem = std::make_unique<BookmarkItem>(guid, name, locationOptional);

	FILETIME dateCreated;
	XMLSettings::ReadDateTime(attributeMap.get(), _T("DateCreated"), dateCreated);
	bookmarkItem->SetDateCreated(dateCreated);

	FILETIME dateModified;
	XMLSettings::ReadDateTime(attributeMap.get(), _T("DateModified"), dateModified);
	bookmarkItem->SetDateModified(dateModified);

	if (type == static_cast<int>(BookmarkItem::Type::Folder))
	{
		LoadBookmarkChildren(parentNode, bookmarkTree, bookmarkItem.get());
	}

	return bookmarkItem;
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkTree *bookmarkTree, int indent)
{
	SavePermanentFolder(xmlDocument, parentNode, bookmarkTree->GetBookmarksToolbarFolder(),
		BookmarkStorage::BOOKMARKS_TOOLBAR_NODE_NAME, indent);
	SavePermanentFolder(xmlDocument, parentNode, bookmarkTree->GetBookmarksMenuFolder(),
		BookmarkStorage::BOOKMARKS_MENU_NODE_NAME, indent);
	SavePermanentFolder(xmlDocument, parentNode, bookmarkTree->GetOtherBookmarksFolder(),
		BookmarkStorage::OTHER_BOOKMARKS_NODE_NAME, indent);
}

void SavePermanentFolder(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *bookmarkItem, const std::wstring &name, int indent)
{
	auto newline =
		wil::make_bstr_nothrow((std::wstring(L"\n") + std::wstring(indent, '\t')).c_str());
	XMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), parentNode);

	wil::com_ptr_nothrow<IXMLDOMElement> childNode;
	XMLSettings::CreateElementNode(xmlDocument, &childNode, parentNode, L"PermanentItem",
		name.c_str());

	XMLSettings::SaveDateTime(xmlDocument, childNode.get(), _T("DateCreated"),
		bookmarkItem->GetDateCreated());
	XMLSettings::SaveDateTime(xmlDocument, childNode.get(), _T("DateModified"),
		bookmarkItem->GetDateModified());

	SaveBookmarkChildren(xmlDocument, childNode.get(), bookmarkItem, indent + 1);

	XMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), childNode.get());
}

void SaveBookmarkChildren(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *parentBookmarkItem, int indent)
{
	int index = 0;

	for (auto &child : parentBookmarkItem->GetChildren())
	{
		auto newline =
			wil::make_bstr_nothrow((std::wstring(L"\n") + std::wstring(indent, '\t')).c_str());
		XMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), parentNode);

		wil::com_ptr_nothrow<IXMLDOMElement> childNode;
		XMLSettings::CreateElementNode(xmlDocument, &childNode, parentNode, _T("Bookmark"),
			std::to_wstring(index).c_str());

		SaveBookmarkItem(xmlDocument, childNode.get(), child.get(), indent);

		XMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), childNode.get());

		index++;
	}
}

void SaveBookmarkItem(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *bookmarkItem, int indent)
{
	XMLSettings::AddAttributeToNode(xmlDocument, parentNode, _T("Type"),
		XMLSettings::EncodeIntValue(static_cast<int>(bookmarkItem->GetType())));
	XMLSettings::AddAttributeToNode(xmlDocument, parentNode, _T("GUID"),
		bookmarkItem->GetGUID().c_str());
	XMLSettings::AddAttributeToNode(xmlDocument, parentNode, _T("ItemName"),
		bookmarkItem->GetName().c_str());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Bookmark)
	{
		XMLSettings::AddAttributeToNode(xmlDocument, parentNode, _T("Location"),
			bookmarkItem->GetLocation().c_str());
	}

	XMLSettings::SaveDateTime(xmlDocument, parentNode, _T("DateCreated"),
		bookmarkItem->GetDateCreated());
	XMLSettings::SaveDateTime(xmlDocument, parentNode, _T("DateModified"),
		bookmarkItem->GetDateModified());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Folder)
	{
		SaveBookmarkChildren(xmlDocument, parentNode, bookmarkItem, indent + 1);
	}
}

}

namespace V1
{

void Load(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree)
{
	LoadBookmarkChildren(parentNode, bookmarkTree, nullptr);
}

void LoadBookmarkChildren(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree,
	BookmarkItem *parentBookmarkItem)
{
	wil::com_ptr_nothrow<IXMLDOMNodeList> children;
	auto queryString = wil::make_bstr_nothrow(L"./Bookmark");
	HRESULT hr = parentNode->selectNodes(queryString.get(), &children);

	if (hr != S_OK)
	{
		return;
	}

	wil::com_ptr_nothrow<IXMLDOMNode> childNode;

	while (children->nextNode(&childNode) == S_OK)
	{
		bool showOnToolbar;
		auto childBookmarkItem = LoadBookmarkItem(childNode.get(), bookmarkTree, showOnToolbar);

		if (!parentBookmarkItem)
		{
			if (showOnToolbar)
			{
				bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(),
					std::move(childBookmarkItem),
					bookmarkTree->GetBookmarksToolbarFolder()->GetChildren().size());
			}
			else
			{
				bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(),
					std::move(childBookmarkItem),
					bookmarkTree->GetBookmarksMenuFolder()->GetChildren().size());
			}
		}
		else
		{
			bookmarkTree->AddBookmarkItem(parentBookmarkItem, std::move(childBookmarkItem),
				parentBookmarkItem->GetChildren().size());
		}
	}
}

std::unique_ptr<BookmarkItem> LoadBookmarkItem(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree,
	bool &showOnToolbarOutput)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	parentNode->get_attributes(&attributeMap);

	int type;
	XMLSettings::GetIntFromMap(attributeMap.get(), L"Type", type);

	std::wstring name;
	XMLSettings::GetStringFromMap(attributeMap.get(), L"name", name);

	std::wstring showOnToolbar;
	XMLSettings::GetStringFromMap(attributeMap.get(), L"ShowOnBookmarksToolbar", showOnToolbar);

	showOnToolbarOutput = XMLSettings::DecodeBoolValue(showOnToolbar.c_str());

	std::optional<std::wstring> locationOptional;

	if (type == static_cast<int>(BookmarkStorage::BookmarkTypeV1::Bookmark))
	{
		std::wstring location;
		XMLSettings::GetStringFromMap(attributeMap.get(), L"Location", location);

		locationOptional = location;
	}

	auto bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt, name, locationOptional);

	if (type == static_cast<int>(BookmarkStorage::BookmarkTypeV1::Folder))
	{
		wil::com_ptr_nothrow<IXMLDOMNode> bookmarksNode;
		auto queryString = wil::make_bstr_nothrow(L"./Bookmarks");
		HRESULT hr = parentNode->selectSingleNode(queryString.get(), &bookmarksNode);

		if (hr == S_OK)
		{
			LoadBookmarkChildren(bookmarksNode.get(), bookmarkTree, bookmarkItem.get());
		}
	}

	return bookmarkItem;
}

}

}
