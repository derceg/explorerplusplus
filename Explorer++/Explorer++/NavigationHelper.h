// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

enum class OpenFolderDisposition
{
	CurrentTab,
	BackgroundTab,
	ForegroundTab,
	NewWindow,

	// It's up to higher level code to interpret these.
	// NewTabDefault indicates that the calling code wants to open a folder in a new tab, with the
	// tab taking a background/foreground position, depending on whichever is the default.
	// NewTabAlternate indicates that the calling code wants to open a folder in a new tab, with the
	// tab taking a background/foreground position, depending on whichever is *not* the default.
	NewTabDefault,
	NewTabAlternate
};

// Determines the open disposition, using the state of the ctrl and shift keys as retrieved by
// GetKeyState(). That is, the state of the keys at the time the current window message was
// generated.
OpenFolderDisposition DetermineOpenDisposition(bool isMiddleButtonDown);

OpenFolderDisposition DetermineOpenDisposition(bool isMiddleButtonDown, bool isCtrlKeyDown,
	bool isShiftKeyDown);
