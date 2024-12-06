// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>

namespace Applications
{

class ApplicationModel;

namespace ApplicationToolbarXmlStorage
{

void Load(IXMLDOMNode *rootNode, ApplicationModel *model);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const ApplicationModel *model);

}

}
