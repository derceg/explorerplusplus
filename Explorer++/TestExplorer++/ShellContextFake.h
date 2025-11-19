// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellContext.h"
#include "SimulatedFileSystem.h"
#include "SimulatedFileSystemEnumerator.h"
#include "SimulatedFileSystemPidlUpdater.h"
#include "SimulatedFileSystemWatcherFactory.h"

class ShellContextFake : public ShellContext
{
public:
	ShellContextFake();

	ShellEnumerator *GetShellEnumerator() override;
	DirectoryWatcherFactory *GetDirectoryWatcherFactory() override;
	PidlUpdater *GetPidlUpdater() override;

	SimulatedFileSystem *GetFileSystem();

private:
	SimulatedFileSystem m_fileSystem;
	SimulatedFileSystemEnumerator m_shellEnumerator;
	SimulatedFileSystemWatcherFactory m_directoryWatcherFactory;
	SimulatedFileSystemPidlUpdater m_pidlUpdater;
};
