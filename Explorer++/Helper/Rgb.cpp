// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Rgb.h"
#include <regex>

unsigned int convertHexStrToUint(const std::wstring &str);

std::optional<COLORREF> parseRGBString(const std::wstring &color)
{
	std::wregex regex(L"#[a-f\\d]{6}", std::regex_constants::icase);

	bool match = std::regex_match(color, regex);

	if (!match)
	{
		return std::nullopt;
	}

	unsigned int r;
	unsigned int g;
	unsigned int b;

	r = convertHexStrToUint(color.substr(1, 2));
	g = convertHexStrToUint(color.substr(3, 2));
	b = convertHexStrToUint(color.substr(5, 2));

	return RGB(r, g, b);
}

unsigned int convertHexStrToUint(const std::wstring &str)
{
	unsigned int value;

	std::wstringstream ss;
	ss << std::hex << str;
	ss >> value;

	return value;
}