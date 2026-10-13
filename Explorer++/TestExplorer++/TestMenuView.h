// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "NoOpMenuHelpTextHost.h"
#include "RootMenuView.h"

class TestMenuView : public RootMenuView
{
public:
	TestMenuView(HWND ownerWindow = nullptr,
		MenuHelpTextHost *menuHelpTextHost = NoOpMenuHelpTextHost::GetInstance());
};
