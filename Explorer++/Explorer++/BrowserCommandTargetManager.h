// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserCommandTarget.h"
#include "../Helper/SignalWrapper.h"

class BrowserWindow;

// Manages the active target view for an individual browser window.
class BrowserCommandTargetManager
{
public:
	// Signals
	SignalWrapper<BrowserCommandTargetManager, void()> targetChangedSignal;

	BrowserCommandTargetManager(const BrowserWindow *browser);

	BrowserCommandTarget *GetCurrentTarget() const;

	// Clients should use `ScopedBrowserCommandTarget`, rather than invoking these methods directly.
	void SetCurrentTarget(BrowserCommandTarget *currentTarget);
	void ClearCurrentTarget();

private:
	// This represents the default state, in which no command is enabled and executing a command has
	// no effect.
	class DefaultTarget : public BrowserCommandTarget
	{
	public:
		bool IsCommandEnabled(int command) const override;
		void ExecuteCommand(int command) override;
	};

	void UpdateCurrentTarget(BrowserCommandTarget *currentTarget);

	const BrowserWindow *const m_browser;
	BrowserCommandTarget *m_currentTarget = nullptr;
	static inline DefaultTarget m_defaultTarget;
};
