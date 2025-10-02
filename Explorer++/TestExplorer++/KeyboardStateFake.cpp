// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "KeyboardStateFake.h"

bool KeyboardStateFake::IsCtrlDown() const
{
	return m_ctrlDown;
}

bool KeyboardStateFake::IsShiftDown() const
{
	return m_shiftDown;
}

bool KeyboardStateFake::IsAltDown() const
{
	return m_altDown;
}

void KeyboardStateFake::SetCtrlDown(bool down)
{
	m_ctrlDown = down;
}

void KeyboardStateFake::SetShiftDown(bool down)
{
	m_shiftDown = down;
}

void KeyboardStateFake::SetAltDown(bool down)
{
	m_altDown = down;
}
