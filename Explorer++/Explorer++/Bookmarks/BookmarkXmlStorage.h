// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <MsXml2.h>

class BookmarkTree;

namespace BookmarkXmlStorage
{

void Load(IXMLDOMNode *rootNode, BookmarkTree *bookmarkTree);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const BookmarkTree *bookmarkTree);

}
