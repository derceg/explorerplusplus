// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabView.h"

class MainTabView : public TabView
{
public:
	static MainTabView *Create(HWND parent, const Config *config);

private:
	MainTabView(HWND parent, const Config *config);
};
