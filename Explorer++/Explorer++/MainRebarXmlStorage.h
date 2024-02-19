// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>
#include <vector>

struct RebarBandStorageInfo;

namespace MainRebarXmlStorage
{

std::vector<RebarBandStorageInfo> Load(IXMLDOMDocument *xmlDocument);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *rootNode,
	const std::vector<RebarBandStorageInfo> &rebarStorageInfo);

}
