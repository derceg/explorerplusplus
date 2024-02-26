// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>

struct FolderColumns;

namespace ColumnXmlStorage
{

void LoadAllColumnSets(IXMLDOMNode *parentNode, FolderColumns &folderColumns);
void SaveAllColumnSets(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const FolderColumns &folderColumns);

}
