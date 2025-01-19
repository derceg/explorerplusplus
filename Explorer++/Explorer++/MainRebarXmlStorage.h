// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>
#include <vector>

struct RebarBandStorageInfo;

namespace MainRebarXmlStorage
{

std::vector<RebarBandStorageInfo> Load(IXMLDOMNode *mainRebarNode);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *mainRebarNode,
	const std::vector<RebarBandStorageInfo> &rebarStorageInfo);

}
