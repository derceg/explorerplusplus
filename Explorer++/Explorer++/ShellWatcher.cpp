// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellWatcher.h"
#include "ShellWatcherManager.h"

std::unique_ptr<ShellWatcher> ShellWatcher::MaybeCreate(ShellWatcherManager *manager,
	const PidlAbsolute &pidl, Filters filters, Callback callback, Behavior behavior)
{
	auto id = manager->StartWatching(pidl, filters, callback, behavior);

	if (!id)
	{
		return nullptr;
	}

	return std::make_unique<ShellWatcher>(manager, *id, PassKey());
}

ShellWatcher::ShellWatcher(ShellWatcherManager *manager, UINT id, PassKey) :
	m_manager(manager),
	m_id(id)
{
}

ShellWatcher::~ShellWatcher()
{
	m_manager->StopWatching(m_id);
}
