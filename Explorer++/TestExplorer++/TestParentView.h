// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ViewBase.h"
#include <wil/resource.h>

class TestParentView : public ViewBase
{
public:
	TestParentView();

private:
	static HWND CreateTestWindow();

	wil::unique_hwnd m_ownedWindow;
};
