// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class BrowserCommandTarget;
class BrowserCommandTargetManager;

// Intended to be held as a member variable within a view. `TargetFocused` should then be called
// when the view receives focus (i.e. when WM_SETFOCUS is received). When the instance is destroyed,
// it will clear the current target, if the current target matches the provided target.
class ScopedBrowserCommandTarget
{
public:
	ScopedBrowserCommandTarget(BrowserCommandTargetManager *targetManager,
		BrowserCommandTarget *target);
	~ScopedBrowserCommandTarget();

	void TargetFocused();

private:
	BrowserCommandTargetManager *const m_targetManager;
	BrowserCommandTarget *const m_target;
};
