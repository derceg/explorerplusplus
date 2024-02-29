// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>
#include <vector>

struct TabStorageData;

namespace TabXmlStorage
{

std::vector<TabStorageData> Load(IXMLDOMNode *tabsNode);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *tabsNode,
	const std::vector<TabStorageData> &tabs);

}
