// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// By implementing this interface, a view can both indicate whether a particular command should be
// enabled as well as execute a command.
//
// The advantage of this is that it allows generic commands (e.g. cut, copy, paste, etc) to be
// handled in whatever way the active view deems best.
class BrowserCommandTarget
{
public:
	virtual ~BrowserCommandTarget() = default;

	virtual bool IsCommandEnabled(int command) const = 0;
	virtual void ExecuteCommand(int command) = 0;
};
