// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <msxml.h>
#include <string>
#include <vector>

namespace StartupFoldersXmlStorage
{

std::vector<std::wstring> Load(IXMLDOMNode *startupFoldersNode);
void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *startupFoldersNode,
	const std::vector<std::wstring> &startupFolders);

}
