// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ViewBase.h"

class AddressBarView;

class BrowserView : public ViewBase
{
public:
	enum class FocusChangeDirection
	{
		Previous,
		Next
	};

	BrowserView(HWND hwnd) : ViewBase(hwnd)
	{
	}

	virtual AddressBarView *GetAddressBarView() const = 0;

	virtual void FocusNextWindow(FocusChangeDirection direction) = 0;
};
