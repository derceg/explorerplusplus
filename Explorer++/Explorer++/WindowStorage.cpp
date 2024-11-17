// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowStorage.h"
#include "MainRebarStorage.h"
#include "TabStorage.h"
#include "../Helper/Helper.h"
#include "../Helper/WindowHelper.h"

namespace
{

const auto g_showStateMappings = MakeBimap<int, WindowShowState>(
	{ { SW_SHOWNORMAL, WindowShowState::Normal }, { SW_SHOWMINIMIZED, WindowShowState::Minimized },
		{ SW_SHOWMAXIMIZED, WindowShowState::Maximized } });

}

bool WindowStorageData::operator==(const WindowStorageData &other) const = default;

WindowShowState NativeShowStateToShowState(int nativeShowState)
{
	auto itr = g_showStateMappings.left.find(nativeShowState);

	if (itr != g_showStateMappings.left.end())
	{
		return itr->second;
	}

	return WindowShowState::Normal;
}

int ShowStateToNativeShowState(WindowShowState showState)
{
	auto itr = g_showStateMappings.right.find(showState);

	if (itr != g_showStateMappings.right.end())
	{
		return itr->second;
	}

	return SW_SHOWNORMAL;
}
