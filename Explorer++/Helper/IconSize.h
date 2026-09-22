// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

enum class IconSize
{
	// Indicates that the icon size should match SM_CXSMICON/SM_CYSMICON.
	Small,

	// Indicates that the icon size should match SM_CXICON/SM_CYICON.
	Large
};

int GetIconPixelSizeAtDefaultDpi(IconSize size);
