// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CustomizableToolbarView.h"

class MainToolbarView : public CustomizableToolbarView
{
public:
	MainToolbarView(HWND parent, MenuHelpTextHost *menuHelpTextHost, const Config *config);
};
