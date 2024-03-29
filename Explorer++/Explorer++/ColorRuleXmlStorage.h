// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>

class ColorRuleModel;

namespace ColorRuleXmlStorage
{

void Load(IXMLDOMDocument *xmlDocument, ColorRuleModel *model);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *rootNode, const ColorRuleModel *model);

}
