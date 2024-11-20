// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>
#include <memory>

class CustomFont;

namespace CustomFontStorage
{

std::unique_ptr<CustomFont> LoadFromRegistry(HKEY fontKey);
void SaveToRegistry(HKEY fontKey, const CustomFont &customFont);

std::unique_ptr<CustomFont> LoadFromXml(IXMLDOMNode *fontNode);
void SaveToXml(IXMLDOMDocument *xmlDocument, IXMLDOMElement *fontNode,
	const CustomFont &customFont);

}
