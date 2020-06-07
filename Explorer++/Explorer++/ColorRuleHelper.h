// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <objbase.h>
#include <MsXml2.h>

namespace NColorRuleHelper
{
	struct ColorRule
	{
		std::wstring	strDescription;

		/* Filename and attribute filtering. */
		std::wstring	strFilterPattern;
		BOOL			caseInsensitive;
		DWORD			dwFilterAttributes;

		COLORREF		rgbColour;
	};

	std::vector<ColorRule> GetDefaultColorRules();

	void	LoadColorRulesFromRegistry(std::vector<ColorRule> &ColorRules);
	void	SaveColorRulesToRegistry(const std::vector<ColorRule> &ColorRules);

	void	LoadColorRulesFromXML(IXMLDOMDocument *pXMLDom,std::vector<ColorRule> &ColorRules);
	void	SaveColorRulesToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot,const std::vector<ColorRule> &ColorRules);
}