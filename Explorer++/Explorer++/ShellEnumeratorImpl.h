// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellEnumerator.h"
#include <atomic>

class ShellEnumeratorImpl : public ShellEnumerator
{
public:
	enum class EnumerationScope
	{
		FoldersOnly,
		FoldersAndFiles
	};

	// The values here indicate whether hidden and hidden system items will be included in the
	// enumeration, or excluded.
	enum class HiddenItemsPolicy
	{
		IncludeHidden,
		ExcludeHidden
	};

	ShellEnumeratorImpl(HWND embedder, EnumerationScope enumerationScope,
		HiddenItemsPolicy hiddenItemsPolicy);

	HRESULT EnumerateDirectory(PCIDLIST_ABSOLUTE pidlDirectory, std::vector<PidlChild> &outputItems,
		std::stop_token stopToken) const override;

	// It's safe to call this method on one thread while `EnumerateDirectory` is being run on a
	// different thread.
	void SetHiddenItemsPolicy(HiddenItemsPolicy hiddenItemsPolicy);

private:
	const HWND m_embedder;
	const EnumerationScope m_enumerationScope;
	std::atomic<HiddenItemsPolicy> m_hiddenItemsPolicy = HiddenItemsPolicy::IncludeHidden;
};
