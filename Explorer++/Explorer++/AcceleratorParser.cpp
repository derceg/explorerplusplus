// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AcceleratorParser.h"
#include <boost/algorithm/string.hpp>

const std::wstring KEY_CTRL = L"Ctrl";
const std::wstring KEY_SHIFT = L"Shift";
const std::wstring KEY_ALT = L"Alt";

std::optional<Accelerator> Plugins::parseAccelerator(const std::wstring &acceleratorString)
{
	std::vector<std::wstring> tokens;
	boost::split(tokens, acceleratorString, boost::is_any_of(L"+"));

	// There must at least be a modifier (e.g. ctrl/shift/alt), along
	// with a key. If any of these are missing, the accelerator
	// combination isn't valid.
	if (tokens.size() < 2)
	{
		return std::nullopt;
	}

	BYTE modifiers = FVIRTKEY;
	WORD key = 0;

	for (auto token : tokens)
	{
		boost::trim(token);

		if (token == KEY_CTRL)
		{
			modifiers |= FCONTROL;
		}
		else if (token == KEY_SHIFT)
		{
			modifiers |= FSHIFT;
		}
		else if (token == KEY_ALT)
		{
			modifiers |= FALT;
		}
		else if (token.size() == 1)
		{
			if (token[0] >= 'A' && token[0] <= 'Z')
			{
				key = token[0];
			}
			else if (token[0] >= '0' && token[0] <= '9')
			{
				key = token[0];
			}
		}
		else
		{
			return std::nullopt;
		}
	}

	bool ctrl = (modifiers & FCONTROL) == FCONTROL;
	bool shift = (modifiers & FSHIFT) == FSHIFT;
	bool alt = (modifiers & FALT) == FALT;

	// See https://blogs.msdn.microsoft.com/oldnewthing/20040329-00/?p=40003
	// for an explanation of why Ctrl+Alt shouldn't be used as a
	// shortcut modifier.
	if (key == 0 || (ctrl && alt) || (!ctrl && !shift && !alt) || (!ctrl && shift && !alt))
	{
		return std::nullopt;
	}

	Accelerator accelerator;
	accelerator.modifiers = modifiers;
	accelerator.key = key;

	return accelerator;
}