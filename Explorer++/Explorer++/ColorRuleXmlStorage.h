// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>

class ColorRuleModel;

namespace ColorRuleXmlStorage
{

void Load(IXMLDOMNode *rootNode, ColorRuleModel *model);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const ColorRuleModel *model);

}
