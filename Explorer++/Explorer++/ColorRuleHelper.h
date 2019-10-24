// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <objbase.h>
#include <MsXml2.h>

namespace NColorRuleHelper
{
	struct ColorRule_t
	{
		std::wstring	strDescription;

		/* Filename and attribute filtering. */
		std::wstring	strFilterPattern;
		BOOL			caseInsensitive;
		DWORD			dwFilterAttributes;

		COLORREF		rgbColour;
	};

	std::vector<ColorRule_t> GetDefaultColorRules();

	void	LoadColorRulesFromRegistry(std::vector<ColorRule_t> &ColorRules);
	void	SaveColorRulesToRegistry(const std::vector<ColorRule_t> &ColorRules);

	void	LoadColorRulesFromXML(IXMLDOMDocument *pXMLDom,std::vector<ColorRule_t> &ColorRules);
	void	SaveColorRulesToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot,const std::vector<ColorRule_t> &ColorRules);
}