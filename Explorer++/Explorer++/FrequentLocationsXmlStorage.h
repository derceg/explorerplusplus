// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>

class FrequentLocationsModel;

namespace FrequentLocationsXmlStorage
{

void Load(IXMLDOMNode *rootNode, FrequentLocationsModel *model);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const FrequentLocationsModel *model);

}
