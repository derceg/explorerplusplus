// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "UiApi.h"
#include "UiTheming.h"
#include "../Helper/Rgb.h"

Plugins::UiApi::UiApi(UiTheming *uiTheming) :
	m_uiTheming(uiTheming)
{

}

bool Plugins::UiApi::setListViewColors(const std::wstring &backgroundColorString, const std::wstring &textColorString)
{
	auto backgroundColor = parseRGBString(backgroundColorString);
	auto textColor = parseRGBString(textColorString);

	if (!backgroundColor || !textColor)
	{
		return false;
	}

	bool res = m_uiTheming->SetListViewColors(*backgroundColor, *textColor);

	return res;
}

bool Plugins::UiApi::setTreeViewColors(const std::wstring &backgroundColorString, const std::wstring &textColorString)
{
	auto backgroundColor = parseRGBString(backgroundColorString);
	auto textColor = parseRGBString(textColorString);

	if (!backgroundColor || !textColor)
	{
		return false;
	}

	m_uiTheming->SetTreeViewColors(*backgroundColor, *textColor);

	return true;
}