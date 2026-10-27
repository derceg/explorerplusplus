// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarView.h"

namespace Applications
{

class ApplicationToolbarView : public ToolbarView
{
public:
	ApplicationToolbarView(HWND parent, MenuHelpTextHost *menuHelpTextHost, const Config *config);
};

}
