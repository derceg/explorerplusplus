// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>

struct FolderColumns;

namespace DefaultColumnXmlStorage
{

void Load(IXMLDOMDocument *xmlDocument, FolderColumns &defaultColumns);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *rootNode,
	const FolderColumns &defaultColumns);

}
