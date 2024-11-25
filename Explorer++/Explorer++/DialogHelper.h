// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>

namespace DialogHelper
{

void LoadDialogStatesFromRegistry(HKEY applicationKey);
void SaveDialogStatesToRegistry(HKEY applicationKey);

void LoadDialogStatesFromXML(IXMLDOMDocument *xmlDocument);
void SaveDialogStatesToXML(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode);

}
