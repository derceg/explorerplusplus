// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedFileSystemWatcherFactory.h"
#include "SimulatedFileSystemWatcher.h"

SimulatedFileSystemWatcherFactory::SimulatedFileSystemWatcherFactory(
	SimulatedFileSystem *fileSystem) :
	m_fileSystem(fileSystem)
{
}

std::unique_ptr<DirectoryWatcher> SimulatedFileSystemWatcherFactory::MaybeCreate(
	const PidlAbsolute &pidl, DirectoryWatcher::Filters filters,
	DirectoryWatcher::Callback callback, DirectoryWatcher::Behavior behavior)
{
	UNREFERENCED_PARAMETER(filters);

	return std::make_unique<SimulatedFileSystemWatcher>(m_fileSystem, pidl, callback, behavior);
}
