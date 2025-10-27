// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

enum class ChangeNotifyMode
{
	// Indicates that `ShellWatcher` will be used to watch directories for changes. Internally, that
	// class uses `SHChangeNotifyRegister` to perform the watch.
	Shell,

	// Indicates that `FileSystemWatcher` will be used to watch directories for changes. Internally,
	// that class uses `ReadDirectoryChangesW` to perform the watch.
	Filesystem
};
