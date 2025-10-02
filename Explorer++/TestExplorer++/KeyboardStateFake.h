// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/KeyboardState.h"

class KeyboardStateFake : public KeyboardState
{
public:
	bool IsCtrlDown() const override;
	bool IsShiftDown() const override;
	bool IsAltDown() const override;

	void SetCtrlDown(bool down);
	void SetShiftDown(bool down);
	void SetAltDown(bool down);

private:
	bool m_ctrlDown = false;
	bool m_shiftDown = false;
	bool m_altDown = false;
};
