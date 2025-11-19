// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class DirectoryWatcherFactory;
class PidlUpdater;
class ShellEnumerator;

// Makes available objects that allow interaction with the shell namespace.
class ShellContext
{
public:
	virtual ~ShellContext() = default;

	virtual ShellEnumerator *GetShellEnumerator() = 0;
	virtual DirectoryWatcherFactory *GetDirectoryWatcherFactory() = 0;
	virtual PidlUpdater *GetPidlUpdater() = 0;
};
