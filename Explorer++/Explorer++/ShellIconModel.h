// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellIconLoader.h"
#include "../Helper/PidlHelper.h"

// Represents a shell icon (i.e. the icon for a shell item). Using this class, it's possible to
// signify that a shell icon should be shown, without needing to know the exact details of how that
// icon is loaded.
class ShellIconModel
{
public:
	ShellIconModel() = default;
	ShellIconModel(ShellIconLoader *shellIconLoader, PCIDLIST_ABSOLUTE pidl,
		ShellIconSize size = ShellIconSize::Small);

	wil::unique_hbitmap GetBitmap(ShellIconUpdateCallback updateCallback) const;
	bool IsEmpty() const;

private:
	ShellIconLoader *m_shellIconLoader = nullptr;
	const PidlAbsolute m_pidl;
	const ShellIconSize m_size = ShellIconSize::Small;
};
