// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>
#include <vector>

struct RebarBandStorageData;

namespace RebarXmlStorage
{

std::vector<RebarBandStorageData> Load(IXMLDOMNode *rebarNode);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *rebarNode,
	const std::vector<RebarBandStorageData> &rebarStorageInfo);

}
