// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellChangeWatcher.h"

std::unique_ptr<ShellChangeWatcher> ShellChangeWatcher::MaybeCreate(ShellChangeManager *manager,
	const PidlAbsolute &pidl, LONG events, ShellChangeManager::Callback callback, bool recursive)
{
	auto id = manager->StartWatching(pidl, events, callback, recursive);

	if (!id)
	{
		return nullptr;
	}

	return std::make_unique<ShellChangeWatcher>(manager, *id, PassKey());
}

ShellChangeWatcher::ShellChangeWatcher(ShellChangeManager *manager, UINT id, PassKey) :
	m_manager(manager),
	m_id(id)
{
}

ShellChangeWatcher::~ShellChangeWatcher()
{
	m_manager->StopWatching(m_id);
}
