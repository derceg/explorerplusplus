// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellEnumerator.h"

class ShellEnumeratorImpl : public ShellEnumerator
{
public:
	ShellEnumeratorImpl(HWND embedder);

	HRESULT EnumerateDirectory(PCIDLIST_ABSOLUTE pidlDirectory, ShellItemFilter::ItemType itemType,
		ShellItemFilter::HiddenItemPolicy hiddenItemPolicy, std::vector<PidlChild> &outputItems,
		std::stop_token stopToken) const override;

private:
	const HWND m_embedder;
};
