// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>
#include <optional>
#include <string>

class MainToolbarLayout;

namespace MainToolbarStorage
{

std::optional<MainToolbarLayout> LoadFromRegistry(HKEY parentKey, const std::wstring &valueName);
void SaveToRegistry(HKEY parentKey, const std::wstring &valueName, const MainToolbarLayout &layout);
std::optional<MainToolbarLayout> LoadFromXml(IXMLDOMNode *toolbarNode);
void SaveToXml(IXMLDOMDocument *xmlDocument, IXMLDOMElement *toolbarNode,
	const MainToolbarLayout &layout);

}
