// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PlatformContextImpl.h"

SystemClock *PlatformContextImpl::GetSystemClock()
{
	return &m_systemClock;
}

KeyboardState *PlatformContextImpl::GetKeyboardState()
{
	return &m_keyboardState;
}

ClipboardStore *PlatformContextImpl::GetClipboardStore()
{
	return &m_clipboardStore;
}
