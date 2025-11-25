// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconModel.h"
#include "ShellIconLoader.h"
#include "../Helper/Pidl.h"

// Represents a shell icon (i.e. the icon for a shell item). Using this class, it's possible to
// signify that a shell icon should be shown, without needing to know the exact details of how that
// icon is loaded.
class ShellIconModel : public IconModel
{
public:
	ShellIconModel(ShellIconLoader *shellIconLoader, PCIDLIST_ABSOLUTE pidl,
		ShellIconSize size = ShellIconSize::Small);

	wil::unique_hbitmap GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const override;

private:
	ShellIconLoader *const m_shellIconLoader;
	const PidlAbsolute m_pidl;
	const ShellIconSize m_size;
};
