// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "KeyboardState.h"

class KeyboardStateImpl : public KeyboardState
{
public:
	bool IsCtrlDown() const override;
	bool IsShiftDown() const override;
	bool IsAltDown() const override;
};
