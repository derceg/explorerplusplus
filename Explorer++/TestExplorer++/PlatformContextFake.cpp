// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "PlatformContextFake.h"

SystemClock *PlatformContextFake::GetSystemClock()
{
	return &m_systemClock;
}

KeyboardStateFake *PlatformContextFake::GetKeyboardState()
{
	return &m_keyboardState;
}

ClipboardStore *PlatformContextFake::GetClipboardStore()
{
	return &m_clipboardStore;
}
