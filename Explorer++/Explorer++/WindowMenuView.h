// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RootMenuView.h"

class WindowMenuView : public RootMenuView
{
public:
	WindowMenuView(HWND ownerWindow, MenuHelpTextHost *menuHelpTextHost);
};
