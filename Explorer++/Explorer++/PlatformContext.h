// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class ClipboardStore;
class KeyboardState;
class SystemClock;

// Makes available objects that represent the host environment.
class PlatformContext
{
public:
	virtual ~PlatformContext() = default;

	virtual SystemClock *GetSystemClock() = 0;
	virtual KeyboardState *GetKeyboardState() = 0;
	virtual ClipboardStore *GetClipboardStore() = 0;
};
