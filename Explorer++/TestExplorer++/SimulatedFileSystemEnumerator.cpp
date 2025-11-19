// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedFileSystemEnumerator.h"
#include "SimulatedFileSystem.h"

SimulatedFileSystemEnumerator::SimulatedFileSystemEnumerator(SimulatedFileSystem *fileSystem) :
	m_fileSystem(fileSystem)
{
}

HRESULT SimulatedFileSystemEnumerator::EnumerateDirectory(PCIDLIST_ABSOLUTE pidlDirectory,
	std::vector<PidlChild> &outputItems, std::stop_token stopToken) const
{
	UNREFERENCED_PARAMETER(stopToken);

	std::vector<PidlAbsolute> fullPidls;
	HRESULT hr = m_fileSystem->EnumerateFolder(pidlDirectory, fullPidls);

	if (FAILED(hr))
	{
		return hr;
	}

	for (const auto &fullPidl : fullPidls)
	{
		outputItems.push_back(fullPidl.GetLastItem());
	}

	return hr;
}
