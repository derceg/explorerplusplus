// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellContextFake.h"

ShellContextFake::ShellContextFake() :
	m_shellEnumerator(&m_fileSystem),
	m_directoryWatcherFactory(&m_fileSystem),
	m_pidlUpdater(&m_fileSystem)
{
}

ShellEnumerator *ShellContextFake::GetShellEnumerator()
{
	return &m_shellEnumerator;
}

DirectoryWatcherFactory *ShellContextFake::GetDirectoryWatcherFactory()
{
	return &m_directoryWatcherFactory;
}

PidlUpdater *ShellContextFake::GetPidlUpdater()
{
	return &m_pidlUpdater;
}

SimulatedFileSystem *ShellContextFake::GetFileSystem()
{
	return &m_fileSystem;
}
