// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>

class CustomFont;

std::unique_ptr<CustomFont> LoadCustomFontFromRegistry(const std::wstring &keyPath);
void SaveCustomFontToRegistry(const std::wstring &keyPath, const CustomFont &customFont);

std::unique_ptr<CustomFont> LoadCustomFontFromXml(IXMLDOMNode *parentNode);
void SaveCustomFontToXml(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const CustomFont &customFont);
