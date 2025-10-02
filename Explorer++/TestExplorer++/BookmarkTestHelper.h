// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gmock/gmock.h>

// Verifies that a new bookmark item is a folder and is contained within the specified parent
// folder.
MATCHER_P(CreateFolderMatcher, expectedParentFolder, "")
{
	return arg.IsFolder() && arg.GetParent() == expectedParentFolder;
}
