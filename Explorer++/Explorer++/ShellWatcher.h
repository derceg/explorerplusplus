// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DirectoryWatcher.h"
#include "../Helper/PassKey.h"
#include "../Helper/Pidl.h"

class ShellWatcherManager;

// Watches a shell item for changes, via SHChangeNotifyRegister().
class ShellWatcher : public DirectoryWatcher
{
private:
	using PassKey = PassKey<ShellWatcher>;

public:
	static std::unique_ptr<ShellWatcher> MaybeCreate(ShellWatcherManager *manager,
		const PidlAbsolute &pidl, Filters filters, Callback callback, Behavior behavior);

	ShellWatcher(ShellWatcherManager *manager, UINT id, PassKey);
	~ShellWatcher();

private:
	ShellWatcherManager *const m_manager;
	const UINT m_id;
};
