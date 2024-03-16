// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BetterEnumsWrapper.h"

// Note that these values are used to save the toolbar state and should not be changed.
// clang-format off
BETTER_ENUM(MainToolbarButton, int,
	Separator = 45001,
	Back = 45002,
	Forward = 45003,
	Up = 45004,
	Folders = 45005,
	CopyTo = 45006,
	MoveTo = 45007,
	NewFolder = 45008,
	Copy = 45009,
	Cut = 45010,
	Paste = 45011,
	Delete = 45012,
	Views = 45013,
	Search = 45014,
	Properties = 45015,
	Refresh = 45017,
	AddBookmark = 45018,
	NewTab = 45019,
	OpenCommandPrompt = 45020,
	Bookmarks = 45021,
	DeletePermanently = 45022,
	SplitFile = 45023,
	MergeFiles = 45024,
	CloseTab = 45025
)
// clang-format on
