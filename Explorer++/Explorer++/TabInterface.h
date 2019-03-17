// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"

__interface TabInterface
{
	void			SetTabName(Tab &tab, const std::wstring strName);
	void			ClearTabName(Tab &tab);
	HRESULT			RefreshTab(Tab &tab);
};