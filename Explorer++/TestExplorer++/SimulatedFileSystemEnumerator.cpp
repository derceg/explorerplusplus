// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedFileSystemEnumerator.h"
#include "SimulatedFileSystem.h"
#include <ranges>

SimulatedFileSystemEnumerator::SimulatedFileSystemEnumerator(SimulatedFileSystem *fileSystem) :
	m_fileSystem(fileSystem)
{
}

HRESULT SimulatedFileSystemEnumerator::EnumerateDirectory(PCIDLIST_ABSOLUTE pidlDirectory,
	ShellItemFilter::ItemType itemType, ShellItemFilter::HiddenItemPolicy hiddenItemPolicy,
	std::vector<PidlChild> &outputItems, std::stop_token stopToken) const
{
	UNREFERENCED_PARAMETER(stopToken);

	std::vector<PidlAbsolute> fullPidls;
	HRESULT hr = m_fileSystem->EnumerateFolder(pidlDirectory, fullPidls);

	if (FAILED(hr))
	{
		return hr;
	}

	for (const auto &fullPidl : fullPidls
			| std::views::filter([itemType, hiddenItemPolicy](const PidlAbsolute &pidl)
				{ return ShouldIncludeItem(pidl, itemType, hiddenItemPolicy); }))
	{
		outputItems.push_back(fullPidl.GetLastItem());
	}

	return hr;
}

bool SimulatedFileSystemEnumerator::ShouldIncludeItem(const PidlAbsolute &pidl,
	ShellItemFilter::ItemType itemType, ShellItemFilter::HiddenItemPolicy hiddenItemPolicy)
{
	if (itemType == ShellItemFilter::ItemType::FoldersOnly
		&& !DoesItemHaveAttributes(pidl.Raw(), SFGAO_FOLDER))
	{
		return false;
	}

	if (hiddenItemPolicy == ShellItemFilter::HiddenItemPolicy::Exclude
		&& DoesItemHaveAttributes(pidl.Raw(), SFGAO_HIDDEN))
	{
		return false;
	}

	return true;
}
