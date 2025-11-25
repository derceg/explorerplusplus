// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellEnumerator.h"

class PidlAbsolute;
class SimulatedFileSystem;

class SimulatedFileSystemEnumerator : public ShellEnumerator
{
public:
	SimulatedFileSystemEnumerator(SimulatedFileSystem *fileSystem);

	HRESULT EnumerateDirectory(PCIDLIST_ABSOLUTE pidlDirectory, ShellItemFilter::ItemType itemType,
		ShellItemFilter::HiddenItemPolicy hiddenItemPolicy, std::vector<PidlChild> &outputItems,
		std::stop_token stopToken) const override;

private:
	static bool ShouldIncludeItem(const PidlAbsolute &pidl, ShellItemFilter::ItemType itemType,
		ShellItemFilter::HiddenItemPolicy hiddenItemPolicy);

	SimulatedFileSystem *const m_fileSystem;
};
