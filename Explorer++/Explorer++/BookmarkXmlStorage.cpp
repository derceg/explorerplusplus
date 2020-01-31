// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkXmlStorage.h"
#include "BookmarkItem.h"
#include "BookmarkStorage.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>

void LoadPermanentFolder(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem,
	const std::wstring &name);
void LoadBookmarkChildren(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree, BookmarkItem *parentBookmarkItem);
std::unique_ptr<BookmarkItem> LoadBookmarkItem(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree);

void SavePermanentFolder(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *bookmarkItem, const std::wstring &name, int indent);
void SaveBookmarkChildren(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *parentBookmarkItem, int indent);
void SaveBookmarkItem(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *bookmarkItem, int indent);

void BookmarkXmlStorage::Load(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree)
{
	LoadPermanentFolder(parentNode, bookmarkTree, bookmarkTree->GetBookmarksToolbarFolder(),
		BookmarkStorage::BOOKMARKS_TOOLBAR_NODE_NAME);
	LoadPermanentFolder(parentNode, bookmarkTree, bookmarkTree->GetBookmarksMenuFolder(),
		BookmarkStorage::BOOKMARKS_MENU_NODE_NAME);
	LoadPermanentFolder(parentNode, bookmarkTree, bookmarkTree->GetOtherBookmarksFolder(),
		BookmarkStorage::OTHER_BOOKMARKS_NODE_NAME);
}

void LoadPermanentFolder(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem,
	const std::wstring &name)
{
	wil::com_ptr<IXMLDOMNode> childNode;
	auto queryString = wil::make_bstr((L".//PermanentItem[@name='" + name + L"']").c_str());
	HRESULT hr = parentNode->selectSingleNode(queryString.get(), &childNode);

	if (hr == S_OK)
	{
		wil::com_ptr<IXMLDOMNamedNodeMap> attributeMap;
		childNode->get_attributes(&attributeMap);

		FILETIME dateCreated;
		NXMLSettings::ReadDateTime(attributeMap.get(), _T("DateCreated"), dateCreated);
		bookmarkItem->SetDateCreated(dateCreated);

		FILETIME dateModified;
		NXMLSettings::ReadDateTime(attributeMap.get(), _T("DateModified"), dateModified);
		bookmarkItem->SetDateModified(dateModified);

		LoadBookmarkChildren(childNode.get(), bookmarkTree, bookmarkItem);
	}
}

void LoadBookmarkChildren(IXMLDOMNode *parentNode, BookmarkTree *bookmarkTree, BookmarkItem *parentBookmarkItem)
{
	auto makeQueryString = [] (int index) {
		return wil::make_bstr((L".//Bookmark[@name='" + std::to_wstring(index) + L"']").c_str());
	};

	wil::com_ptr<IXMLDOMNode> childNode;
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
	wil::com_ptr<IXMLDOMNamedNodeMap> attributeMap;
	parentNode->get_attributes(&attributeMap);

	int type;
	NXMLSettings::GetIntFromMap(attributeMap.get(), L"Type", type);

	std::wstring guid;
	NXMLSettings::GetStringFromMap(attributeMap.get(), L"GUID", guid);

	std::wstring name;
	NXMLSettings::GetStringFromMap(attributeMap.get(), L"ItemName", name);

	std::optional<std::wstring> locationOptional;

	if (type == static_cast<int>(BookmarkItem::Type::Bookmark))
	{
		std::wstring location;
		NXMLSettings::GetStringFromMap(attributeMap.get(), L"Location", location);

		locationOptional = location;
	}

	auto bookmarkItem = std::make_unique<BookmarkItem>(guid, name, locationOptional);

	FILETIME dateCreated;
	NXMLSettings::ReadDateTime(attributeMap.get(), _T("DateCreated"), dateCreated);
	bookmarkItem->SetDateCreated(dateCreated);

	FILETIME dateModified;
	NXMLSettings::ReadDateTime(attributeMap.get(), _T("DateModified"), dateModified);
	bookmarkItem->SetDateModified(dateModified);

	if (type == static_cast<int>(BookmarkItem::Type::Folder))
	{
		LoadBookmarkChildren(parentNode, bookmarkTree, bookmarkItem.get());
	}

	return bookmarkItem;
}

void BookmarkXmlStorage::Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	BookmarkTree *bookmarkTree, int indent)
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
	auto newline = wil::make_bstr((std::wstring(L"\n") + std::wstring(indent, '\t')).c_str());
	NXMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), parentNode);

	wil::com_ptr<IXMLDOMElement> childNode;
	NXMLSettings::CreateElementNode(xmlDocument, &childNode, parentNode, L"PermanentItem", name.c_str());

	NXMLSettings::SaveDateTime(xmlDocument, childNode.get(), _T("DateCreated"), bookmarkItem->GetDateCreated());
	NXMLSettings::SaveDateTime(xmlDocument, childNode.get(), _T("DateModified"), bookmarkItem->GetDateModified());

	SaveBookmarkChildren(xmlDocument, childNode.get(), bookmarkItem, indent + 1);

	NXMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), childNode.get());
}

void SaveBookmarkChildren(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *parentBookmarkItem, int indent)
{
	int index = 0;

	for (auto &child : parentBookmarkItem->GetChildren())
	{
		auto newline = wil::make_bstr((std::wstring(L"\n") + std::wstring(indent, '\t')).c_str());
		NXMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), parentNode);

		wil::com_ptr<IXMLDOMElement> childNode;
		NXMLSettings::CreateElementNode(xmlDocument, &childNode, parentNode, _T("Bookmark"), std::to_wstring(index).c_str());

		SaveBookmarkItem(xmlDocument, childNode.get(), child.get(), indent);

		NXMLSettings::AddWhiteSpaceToNode(xmlDocument, newline.get(), childNode.get());

		index++;
	}
}

void SaveBookmarkItem(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const BookmarkItem *bookmarkItem, int indent)
{
	NXMLSettings::AddAttributeToNode(xmlDocument, parentNode, _T("Type"), NXMLSettings::EncodeIntValue(static_cast<int>(bookmarkItem->GetType())));
	NXMLSettings::AddAttributeToNode(xmlDocument, parentNode, _T("GUID"), bookmarkItem->GetGUID().c_str());
	NXMLSettings::AddAttributeToNode(xmlDocument, parentNode, _T("ItemName"), bookmarkItem->GetName().c_str());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Bookmark)
	{
		NXMLSettings::AddAttributeToNode(xmlDocument, parentNode, _T("Location"), bookmarkItem->GetLocation().c_str());
	}

	NXMLSettings::SaveDateTime(xmlDocument, parentNode, _T("DateCreated"), bookmarkItem->GetDateCreated());
	NXMLSettings::SaveDateTime(xmlDocument, parentNode, _T("DateModified"), bookmarkItem->GetDateModified());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Folder)
	{
		SaveBookmarkChildren(xmlDocument, parentNode, bookmarkItem, indent + 1);
	}
}