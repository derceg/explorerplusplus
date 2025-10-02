// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// This class exists to make it easier to fake keyboard modifiers (ctrl/shift/alt) being down in
// unit tests.
class KeyboardState
{
public:
	virtual ~KeyboardState() = default;

	virtual bool IsCtrlDown() const = 0;
	virtual bool IsShiftDown() const = 0;
	virtual bool IsAltDown() const = 0;
};
