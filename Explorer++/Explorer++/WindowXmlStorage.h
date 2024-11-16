// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>
#include <vector>

struct WindowStorageData;

namespace WindowXmlStorage
{

std::vector<WindowStorageData> Load(IXMLDOMNode *rootNode);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode,
	const std::vector<WindowStorageData> &windows);

}
