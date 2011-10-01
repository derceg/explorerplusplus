/******************************************************************
 *
 * Project: Explorer++
 * File: ColorRuleHelper.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides helper functionality for the color rules (which are
 * used to color files, based on their filename an/or
 * attributes).
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <vector>
#include "Explorer++_internal.h"
#include "ColorRuleHelper.h"
#include "MainResource.h"
#include "../Helper/Macros.h"


namespace
{
	const COLORREF CF_COMPRESSED = RGB(0,0,255);
	const COLORREF CF_ENCRYPTED = RGB(0,128,0);
}

std::vector<NColorRuleHelper::ColorRule_t> NColorRuleHelper::GetDefaultColorRules()
{
	std::vector<ColorRule_t> ColorRules;
	ColorRule_t ColorRule;
	TCHAR szTemp[64];

	LoadString(g_hLanguageModule,IDS_GENERAL_COLOR_RULE_COMPRESSED,szTemp,SIZEOF_ARRAY(szTemp));
	ColorRule.strDescription		= szTemp;
	ColorRule.rgbColour				= CF_COMPRESSED;
	ColorRule.dwFilterAttributes	= FILE_ATTRIBUTE_COMPRESSED;
	ColorRules.push_back(ColorRule);

	LoadString(g_hLanguageModule,IDS_GENERAL_COLOR_RULE_ENCRYPTED,szTemp,SIZEOF_ARRAY(szTemp));
	ColorRule.strDescription		= szTemp;
	ColorRule.rgbColour				= CF_ENCRYPTED;
	ColorRule.dwFilterAttributes	= FILE_ATTRIBUTE_ENCRYPTED;
	ColorRules.push_back(ColorRule);

	return ColorRules;
}