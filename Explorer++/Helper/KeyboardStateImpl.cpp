// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "KeyboardStateImpl.h"
#include "Helper.h"

bool KeyboardStateImpl::IsCtrlDown() const
{
	return IsKeyDown(VK_CONTROL);
}

bool KeyboardStateImpl::IsShiftDown() const
{
	return IsKeyDown(VK_SHIFT);
}

bool KeyboardStateImpl::IsAltDown() const
{
	return IsKeyDown(VK_MENU);
}
