// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DirectoryWatcherFactory.h"

class SimulatedFileSystem;

class SimulatedFileSystemWatcherFactory : public DirectoryWatcherFactory
{
public:
	SimulatedFileSystemWatcherFactory(SimulatedFileSystem *fileSystem);

	std::unique_ptr<DirectoryWatcher> MaybeCreate(const PidlAbsolute &pidl,
		DirectoryWatcher::Filters filters, DirectoryWatcher::Callback callback,
		DirectoryWatcher::Behavior behavior) override;

private:
	SimulatedFileSystem *const m_fileSystem;
};
