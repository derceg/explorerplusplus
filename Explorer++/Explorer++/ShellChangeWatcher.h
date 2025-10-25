// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellChangeManager.h"
#include "../Helper/PassKey.h"
#include "../Helper/PidlHelper.h"

class ShellChangeWatcher
{
private:
	using PassKey = PassKey<ShellChangeWatcher>;

public:
	static std::unique_ptr<ShellChangeWatcher> MaybeCreate(ShellChangeManager *manager,
		const PidlAbsolute &pidl, LONG events, ShellChangeManager::Callback callback,
		bool recursive = false);

	ShellChangeWatcher(ShellChangeManager *manager, UINT id, PassKey);
	~ShellChangeWatcher();

private:
	ShellChangeManager *const m_manager;
	const UINT m_id;
};
