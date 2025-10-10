// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "KeyboardStateFake.h"
#include "PlatformContext.h"
#include "SimulatedClipboardStore.h"
#include "SystemClockFake.h"

class PlatformContextFake : public PlatformContext
{
public:
	SystemClock *GetSystemClock() override;
	KeyboardStateFake *GetKeyboardState() override;
	ClipboardStore *GetClipboardStore() override;

private:
	SystemClockFake m_systemClock;
	KeyboardStateFake m_keyboardState;
	SimulatedClipboardStore m_clipboardStore;
};
